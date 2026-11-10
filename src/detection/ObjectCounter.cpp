#include "ObjectCounter.h"
#include "../utils/Logger.h"
#include <chrono>
#include <algorithm>
#include <cmath>

ObjectCountConfig ObjectCountConfig::FromJson(const QJsonObject& obj)
{
    ObjectCountConfig cfg;
    if (obj.contains("method"))
    {
        cfg.Method = obj["method"].toString().toStdString();
    }
    if (obj.contains("colorChannel"))
    {
        cfg.ColorChannel = obj["colorChannel"].toString().toStdString();
    }
    if (obj.contains("thresholdMethod"))
    {
        cfg.ThresholdMethod = obj["thresholdMethod"].toString().toStdString();
    }
    if (obj.contains("manualThreshold"))
    {
        cfg.ManualThreshold = obj["manualThreshold"].toInt(128);
    }
    if (obj.contains("invertThreshold"))
    {
        cfg.InvertThreshold = obj["invertThreshold"].toBool(false);
    }
    if (obj.contains("morphKernelSize"))
    {
        cfg.MorphKernelSize = obj["morphKernelSize"].toInt(3);
    }
    if (obj.contains("morphType"))
    {
        cfg.MorphType = obj["morphType"].toString("Open").toStdString();
    }
    if (obj.contains("minArea"))
    {
        cfg.MinArea = obj["minArea"].toDouble(30.0);
    }
    if (obj.contains("maxArea"))
    {
        cfg.MaxArea = obj["maxArea"].toDouble(500000.0);
    }
    if (obj.contains("minWidth"))
    {
        cfg.MinWidth = obj["minWidth"].toDouble(5.0);
    }
    if (obj.contains("minHeight"))
    {
        cfg.MinHeight = obj["minHeight"].toDouble(5.0);
    }
    if (obj.contains("minCircularity"))
    {
        cfg.MinCircularity = obj["minCircularity"].toDouble(0.0);
    }
    if (obj.contains("iouThreshold"))
    {
        cfg.IouThreshold = obj["iouThreshold"].toDouble(0.3);
    }
    if (obj.contains("targetClass"))
    {
        cfg.TargetClass = obj["targetClass"].toString("part").toStdString();
    }
    if (obj.contains("targetDisplayName"))
    {
        cfg.TargetDisplayName = obj["targetDisplayName"].toString("零件").toStdString();
    }
    if (obj.contains("roi"))
    {
        QJsonObject roiObj = obj["roi"].toObject();
        cfg.Roi = cv::Rect(
            roiObj["x"].toInt(0),
            roiObj["y"].toInt(0),
            roiObj["width"].toInt(0),
            roiObj["height"].toInt(0)
        );
    }
    return cfg;
}

QJsonObject ObjectCountConfig::ToJson() const
{
    QJsonObject obj;
    obj["method"] = QString::fromStdString(Method);
    obj["colorChannel"] = QString::fromStdString(ColorChannel);
    obj["thresholdMethod"] = QString::fromStdString(ThresholdMethod);
    obj["manualThreshold"] = ManualThreshold;
    obj["invertThreshold"] = InvertThreshold;
    obj["morphKernelSize"] = MorphKernelSize;
    obj["morphType"] = QString::fromStdString(MorphType);
    obj["minArea"] = MinArea;
    obj["maxArea"] = MaxArea;
    obj["minWidth"] = MinWidth;
    obj["minHeight"] = MinHeight;
    obj["minCircularity"] = MinCircularity;
    obj["iouThreshold"] = IouThreshold;
    obj["targetClass"] = QString::fromStdString(TargetClass);
    obj["targetDisplayName"] = QString::fromStdString(TargetDisplayName);

    if (Roi.width > 0 && Roi.height > 0)
    {
        QJsonObject roiObj;
        roiObj["x"] = Roi.x;
        roiObj["y"] = Roi.y;
        roiObj["width"] = Roi.width;
        roiObj["height"] = Roi.height;
        obj["roi"] = roiObj;
    }
    return obj;
}

cv::Mat ObjectCounter::Preprocess(const cv::Mat& input, const ObjectCountConfig& config)
{
    cv::Mat gray;
    if (input.channels() == 3)
    {
        if (config.ColorChannel == "Red")
        {
            std::vector<cv::Mat> channels;
            cv::split(input, channels);
            gray = channels[2];
        }
        else if (config.ColorChannel == "Green")
        {
            std::vector<cv::Mat> channels;
            cv::split(input, channels);
            gray = channels[1];
        }
        else if (config.ColorChannel == "Blue")
        {
            std::vector<cv::Mat> channels;
            cv::split(input, channels);
            gray = channels[0];
        }
        else
        {
            cv::cvtColor(input, gray, cv::COLOR_BGR2GRAY);
        }
    }
    else
    {
        gray = input.clone();
    }

    // 二值化
    cv::Mat binary;
    int threshType = config.InvertThreshold ? cv::THRESH_BINARY_INV : cv::THRESH_BINARY;

    if (config.ThresholdMethod == "Otsu")
    {
        cv::threshold(gray, binary, 0, 255, threshType | cv::THRESH_OTSU);
    }
    else if (config.ThresholdMethod == "Adaptive")
    {
        int adaptType = config.InvertThreshold ? cv::THRESH_BINARY_INV : cv::THRESH_BINARY;
        cv::adaptiveThreshold(gray, binary, 255, cv::ADAPTIVE_THRESH_GAUSSIAN_C, adaptType, 15, 3);
    }
    else
    {
        cv::threshold(gray, binary, config.ManualThreshold, 255, threshType);
    }

    // 形态学滤波
    if (config.MorphKernelSize > 1 && config.MorphType != "None")
    {
        int k = (config.MorphKernelSize % 2 == 0) ? config.MorphKernelSize + 1 : config.MorphKernelSize;
        cv::Mat element = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(k, k));
        if (config.MorphType == "Open")
        {
            cv::morphologyEx(binary, binary, cv::MORPH_OPEN, element);
        }
        else if (config.MorphType == "Close")
        {
            cv::morphologyEx(binary, binary, cv::MORPH_CLOSE, element);
        }
    }

    return binary;
}

void ObjectCounter::ApplyNms(std::vector<BoundingBox>& boxes, double iouThreshold)
{
    if (boxes.empty())
    {
        return;
    }

    // 按面积从大到小排序
    std::sort(boxes.begin(), boxes.end(), [](const BoundingBox& a, const BoundingBox& b) {
        return a.Area > b.Area;
    });

    std::vector<BoundingBox> retained;
    std::vector<bool> suppressed(boxes.size(), false);

    for (size_t i = 0; i < boxes.size(); ++i)
    {
        if (suppressed[i])
        {
            continue;
        }

        retained.push_back(boxes[i]);
        for (size_t j = i + 1; j < boxes.size(); ++j)
        {
            if (suppressed[j])
            {
                continue;
            }

            cv::Rect2f intersection = boxes[i].Rect & boxes[j].Rect;
            float interArea = intersection.area();
            float unionArea = boxes[i].Rect.area() + boxes[j].Rect.area() - interArea;
            float iou = (unionArea > 0.0f) ? (interArea / unionArea) : 0.0f;

            if (iou > iouThreshold)
            {
                suppressed[j] = true;
            }
        }
    }

    boxes = std::move(retained);
}

void ObjectCounter::RenderVisualization(
    cv::Mat& canvas,
    const std::vector<BoundingBox>& objects,
    const ObjectCountConfig& config)
{
    if (canvas.channels() == 1)
    {
        cv::cvtColor(canvas, canvas, cv::COLOR_GRAY2BGR);
    }

    // 绘制 ROI 边界 (若存在)
    if (config.Roi.width > 0 && config.Roi.height > 0)
    {
        cv::rectangle(canvas, config.Roi, cv::Scalar(255, 200, 0), 2, cv::LINE_AA);
        cv::putText(canvas, "ROI", cv::Point(config.Roi.x + 5, config.Roi.y + 20),
                    cv::FONT_HERSHEY_SIMPLEX, 0.6, cv::Scalar(255, 200, 0), 2);
    }

    // 绘制目标边框、中心点与序号
    for (size_t i = 0; i < objects.size(); ++i)
    {
        const auto& obj = objects[i];
        cv::Scalar boxColor(0, 220, 0); // 翠绿色边框

        cv::rectangle(canvas, obj.Rect, boxColor, 2, cv::LINE_AA);

        // 中心十字标记
        int cx = static_cast<int>(obj.Center.x);
        int cy = static_cast<int>(obj.Center.y);
        cv::drawMarker(canvas, cv::Point(cx, cy), cv::Scalar(0, 0, 255), cv::MARKER_CROSS, 8, 2);

        // 标签序号
        std::string tag = "#" + std::to_string(i + 1);
        int baseLine = 0;
        cv::Size textSize = cv::getTextSize(tag, cv::FONT_HERSHEY_SIMPLEX, 0.5, 1, &baseLine);
        cv::Point textPos(static_cast<int>(obj.Rect.x), std::max(18, static_cast<int>(obj.Rect.y) - 4));
        cv::rectangle(canvas,
                      cv::Point(textPos.x, textPos.y - textSize.height - 2),
                      cv::Point(textPos.x + textSize.width + 4, textPos.y + 2),
                      boxColor, -1);
        cv::putText(canvas, tag, cv::Point(textPos.x + 2, textPos.y),
                    cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0, 0, 0), 1, cv::LINE_AA);
    }

    // 绘制左上角统计看板
    std::string countText = "Count: " + std::to_string(objects.size()) + " (" + config.TargetDisplayName + ")";
    int baseLine = 0;
    cv::Size textSize = cv::getTextSize(countText, cv::FONT_HERSHEY_SIMPLEX, 0.75, 2, &baseLine);

    cv::rectangle(canvas, cv::Point(10, 10),
                  cv::Point(20 + textSize.width, 20 + textSize.height + baseLine),
                  cv::Scalar(30, 30, 30), -1);
    cv::rectangle(canvas, cv::Point(10, 10),
                  cv::Point(20 + textSize.width, 20 + textSize.height + baseLine),
                  cv::Scalar(0, 220, 0), 2);
    cv::putText(canvas, countText, cv::Point(15, 15 + textSize.height),
                cv::FONT_HERSHEY_SIMPLEX, 0.75, cv::Scalar(0, 255, 0), 2, cv::LINE_AA);
}

bool ObjectCounter::Execute(
    const cv::Mat& input,
    const ObjectCountConfig& config,
    DetectionResult& result,
    std::string& errorMsg)
{
    if (input.empty())
    {
        errorMsg = "输入图像为空";
        return false;
    }

    auto start = std::chrono::high_resolution_clock::now();

    cv::Mat roiMat = input;
    cv::Point roiOffset(0, 0);

    if (config.Roi.width > 0 && config.Roi.height > 0)
    {
        cv::Rect validRoi = config.Roi & cv::Rect(0, 0, input.cols, input.rows);
        if (validRoi.area() <= 0)
        {
            errorMsg = "指定的 ROI 区域越界或面积为零";
            return false;
        }
        roiMat = input(validRoi);
        roiOffset = validRoi.tl();
    }

    cv::Mat binary = Preprocess(roiMat, config);

    std::vector<BoundingBox> detectedObjects;

    if (config.Method == "Contours")
    {
        std::vector<std::vector<cv::Point>> contours;
        std::vector<cv::Vec4i> hierarchy;
        cv::findContours(binary, contours, hierarchy, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

        for (const auto& pts : contours)
        {
            double area = cv::contourArea(pts);
            if (area < config.MinArea || area > config.MaxArea)
            {
                continue;
            }

            cv::Rect br = cv::boundingRect(pts);
            if (br.width < config.MinWidth || br.height < config.MinHeight)
            {
                continue;
            }

            double perimeter = cv::arcLength(pts, true);
            double circularity = (perimeter > 0.0) ? (4.0 * CV_PI * area / (perimeter * perimeter)) : 0.0;
            if (circularity < config.MinCircularity)
            {
                continue;
            }

            cv::Moments m = cv::moments(pts);
            cv::Point2f center = (m.m00 != 0.0)
                ? cv::Point2f(static_cast<float>(m.m10 / m.m00), static_cast<float>(m.m01 / m.m00))
                : cv::Point2f(br.x + br.width * 0.5f, br.y + br.height * 0.5f);

            BoundingBox obj;
            obj.Rect = cv::Rect2f(
                static_cast<float>(br.x + roiOffset.x),
                static_cast<float>(br.y + roiOffset.y),
                static_cast<float>(br.width),
                static_cast<float>(br.height)
            );
            obj.Center = cv::Point2f(center.x + roiOffset.x, center.y + roiOffset.y);
            obj.Area = area;
            obj.AspectRatio = (br.height > 0) ? (static_cast<double>(br.width) / br.height) : 1.0;
            obj.Circularity = circularity;
            obj.Label = config.TargetClass;
            obj.Confidence = 1.0f;
            detectedObjects.push_back(obj);
        }
    }
    else
    {
        // 连通域分析
        cv::Mat labels, stats, centroids;
        int nLabels = cv::connectedComponentsWithStats(binary, labels, stats, centroids, 8, CV_32S);

        for (int i = 1; i < nLabels; ++i) // 跳过背景标签 0
        {
            int area = stats.at<int>(i, cv::CC_STAT_AREA);
            if (area < config.MinArea || area > config.MaxArea)
            {
                continue;
            }

            int x = stats.at<int>(i, cv::CC_STAT_LEFT);
            int y = stats.at<int>(i, cv::CC_STAT_TOP);
            int w = stats.at<int>(i, cv::CC_STAT_WIDTH);
            int h = stats.at<int>(i, cv::CC_STAT_HEIGHT);

            if (w < config.MinWidth || h < config.MinHeight)
            {
                continue;
            }

            double cx = centroids.at<double>(i, 0);
            double cy = centroids.at<double>(i, 1);

            double aspectRatio = (h > 0) ? (static_cast<double>(w) / h) : 1.0;

            BoundingBox obj;
            obj.Rect = cv::Rect2f(
                static_cast<float>(x + roiOffset.x),
                static_cast<float>(y + roiOffset.y),
                static_cast<float>(w),
                static_cast<float>(h)
            );
            obj.Center = cv::Point2f(
                static_cast<float>(cx + roiOffset.x),
                static_cast<float>(cy + roiOffset.y)
            );
            obj.Area = static_cast<double>(area);
            obj.AspectRatio = aspectRatio;
            obj.Circularity = 1.0;
            obj.Label = config.TargetClass;
            obj.Confidence = 1.0f;
            detectedObjects.push_back(obj);
        }
    }

    // NMS 去重
    if (config.IouThreshold > 0.0)
    {
        ApplyNms(detectedObjects, config.IouThreshold);
    }

    auto end = std::chrono::high_resolution_clock::now();
    double elapsedMs = std::chrono::duration<double, std::milli>(end - start).count();

    result.Objects = detectedObjects;
    result.ObjectCount = static_cast<int>(detectedObjects.size());
    result.Defects.clear();
    result.DefectCount = 0;
    result.ExecutionTimeMs = elapsedMs;

    // 可视化叠加画布
    cv::Mat canvas = input.clone();
    RenderVisualization(canvas, detectedObjects, config);
    result.Visualization = canvas;

    QJsonObject statsObj;
    statsObj["detectedCount"] = result.ObjectCount;
    statsObj["algorithm"] = QString::fromStdString(config.Method);
    statsObj["timeMs"] = elapsedMs;
    result.Statistics = statsObj;

    Logger::Info("ObjectCounter", "目标计数完成: 检测到 " + std::to_string(result.ObjectCount) +
                 " 个目标, 耗时 " + std::to_string(elapsedMs) + " ms");

    return true;
}
