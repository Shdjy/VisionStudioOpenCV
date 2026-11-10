#include "DefectDetector.h"
#include "../utils/Logger.h"
#include <chrono>
#include <algorithm>
#include <cmath>

DefectDetectConfig DefectDetectConfig::FromJson(const QJsonObject& obj)
{
    DefectDetectConfig cfg;
    if (obj.contains("method"))
    {
        cfg.Method = obj["method"].toString().toStdString();
    }
    if (obj.contains("backgroundBlurKernel"))
    {
        cfg.BackgroundBlurKernel = obj["backgroundBlurKernel"].toInt(31);
    }
    if (obj.contains("diffThreshold"))
    {
        cfg.DiffThreshold = obj["diffThreshold"].toInt(25);
    }
    if (obj.contains("detectBrightDefects"))
    {
        cfg.DetectBrightDefects = obj["detectBrightDefects"].toBool(true);
    }
    if (obj.contains("detectDarkDefects"))
    {
        cfg.DetectDarkDefects = obj["detectDarkDefects"].toBool(true);
    }
    if (obj.contains("topHatKernelSize"))
    {
        cfg.TopHatKernelSize = obj["topHatKernelSize"].toInt(15);
    }
    if (obj.contains("morphCleanKernel"))
    {
        cfg.MorphCleanKernel = obj["morphCleanKernel"].toInt(3);
    }
    if (obj.contains("minDefectArea"))
    {
        cfg.MinDefectArea = obj["minDefectArea"].toDouble(15.0);
    }
    if (obj.contains("maxDefectArea"))
    {
        cfg.MaxDefectArea = obj["maxDefectArea"].toDouble(50000.0);
    }
    if (obj.contains("minDefectLength"))
    {
        cfg.MinDefectLength = obj["minDefectLength"].toDouble(4.0);
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

QJsonObject DefectDetectConfig::ToJson() const
{
    QJsonObject obj;
    obj["method"] = QString::fromStdString(Method);
    obj["backgroundBlurKernel"] = BackgroundBlurKernel;
    obj["diffThreshold"] = DiffThreshold;
    obj["detectBrightDefects"] = DetectBrightDefects;
    obj["detectDarkDefects"] = DetectDarkDefects;
    obj["topHatKernelSize"] = TopHatKernelSize;
    obj["morphCleanKernel"] = MorphCleanKernel;
    obj["minDefectArea"] = MinDefectArea;
    obj["maxDefectArea"] = MaxDefectArea;
    obj["minDefectLength"] = MinDefectLength;

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

cv::Mat DefectDetector::ExtractDefectMask(const cv::Mat& gray, const DefectDetectConfig& config)
{
    cv::Mat defectMask = cv::Mat::zeros(gray.size(), CV_8UC1);

    if (config.Method == "TopHat")
    {
        int k = (config.TopHatKernelSize % 2 == 0) ? config.TopHatKernelSize + 1 : config.TopHatKernelSize;
        cv::Mat element = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(k, k));

        if (config.DetectBrightDefects)
        {
            cv::Mat topHat;
            cv::morphologyEx(gray, topHat, cv::MORPH_TOPHAT, element);
            cv::Mat brightMask;
            cv::threshold(topHat, brightMask, config.DiffThreshold, 255, cv::THRESH_BINARY);
            cv::bitwise_or(defectMask, brightMask, defectMask);
        }

        if (config.DetectDarkDefects)
        {
            cv::Mat blackHat;
            cv::morphologyEx(gray, blackHat, cv::MORPH_BLACKHAT, element);
            cv::Mat darkMask;
            cv::threshold(blackHat, darkMask, config.DiffThreshold, 255, cv::THRESH_BINARY);
            cv::bitwise_or(defectMask, darkMask, defectMask);
        }
    }
    else if (config.Method == "AdaptiveThreshold")
    {
        cv::Mat adapt;
        cv::adaptiveThreshold(gray, adapt, 255, cv::ADAPTIVE_THRESH_GAUSSIAN_C, cv::THRESH_BINARY_INV, 25, config.DiffThreshold / 5.0);
        defectMask = adapt;
    }
    else
    {
        // 默认背景差分消除光照不均
        int k = (config.BackgroundBlurKernel % 2 == 0) ? config.BackgroundBlurKernel + 1 : config.BackgroundBlurKernel;
        cv::Mat background;
        cv::GaussianBlur(gray, background, cv::Size(k, k), 0);

        if (config.DetectBrightDefects && config.DetectDarkDefects)
        {
            cv::Mat diff;
            cv::absdiff(gray, background, diff);
            cv::threshold(diff, defectMask, config.DiffThreshold, 255, cv::THRESH_BINARY);
        }
        else if (config.DetectBrightDefects)
        {
            cv::Mat brightDiff;
            cv::subtract(gray, background, brightDiff);
            cv::threshold(brightDiff, defectMask, config.DiffThreshold, 255, cv::THRESH_BINARY);
        }
        else if (config.DetectDarkDefects)
        {
            cv::Mat darkDiff;
            cv::subtract(background, gray, darkDiff);
            cv::threshold(darkDiff, defectMask, config.DiffThreshold, 255, cv::THRESH_BINARY);
        }
    }

    // 形态学开运算去除微小孤立噪点
    if (config.MorphCleanKernel > 1)
    {
        int ck = (config.MorphCleanKernel % 2 == 0) ? config.MorphCleanKernel + 1 : config.MorphCleanKernel;
        cv::Mat cleanElement = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(ck, ck));
        cv::morphologyEx(defectMask, defectMask, cv::MORPH_OPEN, cleanElement);
    }

    return defectMask;
}

void DefectDetector::AnalyzeDefectCandidates(
    const cv::Mat& defectMask,
    const cv::Point& roiOffset,
    const DefectDetectConfig& config,
    std::vector<DefectInfo>& defects)
{
    std::vector<std::vector<cv::Point>> contours;
    std::vector<cv::Vec4i> hierarchy;
    cv::findContours(defectMask, contours, hierarchy, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

    for (const auto& pts : contours)
    {
        double area = cv::contourArea(pts);
        if (area < config.MinDefectArea || area > config.MaxDefectArea)
        {
            continue;
        }

        cv::Rect br = cv::boundingRect(pts);

        // 使用最小外接旋转矩形计算精确主长轴、短轴及倾斜角度
        double length = std::max(br.width, br.height);
        double width = std::min(br.width, br.height);
        double angle = 0.0;

        if (pts.size() >= 5)
        {
            cv::RotatedRect minRect = cv::minAreaRect(pts);
            length = std::max(minRect.size.width, minRect.size.height);
            width = std::min(minRect.size.width, minRect.size.height);
            angle = minRect.angle;
        }

        if (length < config.MinDefectLength)
        {
            continue;
        }

        double aspectRatio = (width > 0.0) ? (length / width) : 1.0;
        double perimeter = cv::arcLength(pts, true);
        double circularity = (perimeter > 0.0) ? (4.0 * CV_PI * area / (perimeter * perimeter)) : 0.0;

        // 智能分类缺陷大类
        std::string defectType;
        std::string defectDisplayName;

        if (aspectRatio >= 2.5)
        {
            defectType = "scratch";
            defectDisplayName = "划痕 (Scratch)";
        }
        else if (circularity >= 0.55)
        {
            defectType = "pit";
            defectDisplayName = "凹坑/气孔 (Pit/Dent)";
        }
        else
        {
            defectType = "stain";
            defectDisplayName = "污渍/斑点 (Stain)";
        }

        DefectInfo info;
        info.DefectType = defectType;
        info.DefectDisplayName = defectDisplayName;
        info.Confidence = 0.95f;
        info.BoundingBox = cv::Rect(br.x + roiOffset.x, br.y + roiOffset.y, br.width, br.height);
        info.Area = area;
        info.Length = length;
        info.Width = width;
        info.AspectRatio = aspectRatio;
        info.Angle = angle;
        info.IsNG = false;

        defects.push_back(info);
    }
}

void DefectDetector::RenderVisualization(
    cv::Mat& canvas,
    const cv::Mat& defectMask,
    const cv::Point& roiOffset,
    const std::vector<DefectInfo>& defects,
    const DefectDetectConfig& config)
{
    if (canvas.channels() == 1)
    {
        cv::cvtColor(canvas, canvas, cv::COLOR_GRAY2BGR);
    }

    // 绘制半透明红色缺陷掩膜叠加
    if (!defectMask.empty())
    {
        cv::Mat colorMask = cv::Mat::zeros(canvas.size(), CV_8UC3);
        cv::Rect targetRoi(roiOffset.x, roiOffset.y, defectMask.cols, defectMask.rows);
        cv::Rect validTarget = targetRoi & cv::Rect(0, 0, canvas.cols, canvas.rows);

        if (validTarget.area() > 0)
        {
            cv::Rect srcRoi(validTarget.x - roiOffset.x, validTarget.y - roiOffset.y,
                            validTarget.width, validTarget.height);
            cv::Mat croppedMask = defectMask(srcRoi);
            cv::Mat maskBgr(validTarget.size(), CV_8UC3, cv::Scalar(0, 0, 255)); // 红色掩膜
            maskBgr.copyTo(colorMask(validTarget), croppedMask);

            cv::addWeighted(canvas, 1.0, colorMask, 0.45, 0.0, canvas);
        }
    }

    // 绘制 ROI 边界
    if (config.Roi.width > 0 && config.Roi.height > 0)
    {
        cv::rectangle(canvas, config.Roi, cv::Scalar(255, 180, 0), 2, cv::LINE_AA);
        cv::putText(canvas, "Workpiece ROI", cv::Point(config.Roi.x + 5, config.Roi.y + 20),
                    cv::FONT_HERSHEY_SIMPLEX, 0.55, cv::Scalar(255, 180, 0), 2);
    }

    // 绘制每个缺陷外接矩形与标签
    for (size_t i = 0; i < defects.size(); ++i)
    {
        const auto& d = defects[i];
        cv::Scalar boxColor = (d.DefectType == "scratch")
            ? cv::Scalar(0, 0, 255)       // 划痕鲜红
            : cv::Scalar(0, 140, 255);    // 凹坑橙黄

        cv::rectangle(canvas, d.BoundingBox, boxColor, 2, cv::LINE_AA);

        std::string tag = d.DefectDisplayName + " " + std::to_string(static_cast<int>(d.Area)) + "px²";
        int baseLine = 0;
        cv::Size textSize = cv::getTextSize(tag, cv::FONT_HERSHEY_SIMPLEX, 0.45, 1, &baseLine);
        cv::Point textPos(d.BoundingBox.x, std::max(16, d.BoundingBox.y - 3));

        cv::rectangle(canvas,
                      cv::Point(textPos.x, textPos.y - textSize.height - 2),
                      cv::Point(textPos.x + textSize.width + 4, textPos.y + 2),
                      boxColor, -1);
        cv::putText(canvas, tag, cv::Point(textPos.x + 2, textPos.y),
                    cv::FONT_HERSHEY_SIMPLEX, 0.45, cv::Scalar(255, 255, 255), 1, cv::LINE_AA);
    }

    // 绘制左上角统计看板
    std::string defectText = "Defects Detected: " + std::to_string(defects.size());
    int baseLine = 0;
    cv::Size textSize = cv::getTextSize(defectText, cv::FONT_HERSHEY_SIMPLEX, 0.7, 2, &baseLine);

    cv::Scalar panelBorder = defects.empty() ? cv::Scalar(0, 220, 0) : cv::Scalar(0, 0, 255);
    cv::rectangle(canvas, cv::Point(10, 10),
                  cv::Point(20 + textSize.width, 20 + textSize.height + baseLine),
                  cv::Scalar(25, 25, 25), -1);
    cv::rectangle(canvas, cv::Point(10, 10),
                  cv::Point(20 + textSize.width, 20 + textSize.height + baseLine),
                  panelBorder, 2);
    cv::putText(canvas, defectText, cv::Point(15, 15 + textSize.height),
                cv::FONT_HERSHEY_SIMPLEX, 0.7, panelBorder, 2, cv::LINE_AA);
}

bool DefectDetector::Execute(
    const cv::Mat& input,
    const DefectDetectConfig& config,
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
            errorMsg = "缺陷检测 ROI 区域越界或为空";
            return false;
        }
        roiMat = input(validRoi);
        roiOffset = validRoi.tl();
    }

    cv::Mat gray;
    if (roiMat.channels() == 3)
    {
        cv::cvtColor(roiMat, gray, cv::COLOR_BGR2GRAY);
    }
    else
    {
        gray = roiMat.clone();
    }

    // 提取缺陷候选区域掩膜
    cv::Mat defectMask = ExtractDefectMask(gray, config);

    // 分析候选缺陷几何特征与分类
    std::vector<DefectInfo> defects;
    AnalyzeDefectCandidates(defectMask, roiOffset, config, defects);

    auto end = std::chrono::high_resolution_clock::now();
    double elapsedMs = std::chrono::duration<double, std::milli>(end - start).count();

    result.Objects.clear();
    result.ObjectCount = 0;
    result.Defects = defects;
    result.DefectCount = static_cast<int>(defects.size());
    result.ExecutionTimeMs = elapsedMs;

    // 可视化叠加画布
    cv::Mat canvas = input.clone();
    RenderVisualization(canvas, defectMask, roiOffset, defects, config);
    result.Visualization = canvas;

    QJsonObject statsObj;
    statsObj["defectCount"] = result.DefectCount;
    statsObj["method"] = QString::fromStdString(config.Method);
    statsObj["timeMs"] = elapsedMs;

    int scratchCount = 0;
    int pitCount = 0;
    int stainCount = 0;
    for (const auto& d : defects)
    {
        if (d.DefectType == "scratch")
        {
            scratchCount++;
        }
        else if (d.DefectType == "pit")
        {
            pitCount++;
        }
        else
        {
            stainCount++;
        }
    }
    statsObj["scratchCount"] = scratchCount;
    statsObj["pitCount"] = pitCount;
    statsObj["stainCount"] = stainCount;
    result.Statistics = statsObj;

    Logger::Info("DefectDetector", "缺陷检测完成: 检出 " + std::to_string(result.DefectCount) +
                 " 处缺陷 (划痕: " + std::to_string(scratchCount) +
                 ", 凹坑: " + std::to_string(pitCount) +
                 ", 污渍: " + std::to_string(stainCount) + "), 耗时 " +
                 std::to_string(elapsedMs) + " ms");

    return true;
}
