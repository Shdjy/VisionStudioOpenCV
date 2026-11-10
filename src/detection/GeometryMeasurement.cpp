#include "GeometryMeasurement.h"
#include "../utils/Logger.h"
#include <chrono>
#include <cmath>

MeasurementConfig MeasurementConfig::FromJson(const QJsonObject& obj)
{
    MeasurementConfig cfg;
    if (obj.contains("mode"))
    {
        cfg.Mode = obj["mode"].toString().toStdString();
    }
    if (obj.contains("pixelToMmRatio"))
    {
        cfg.PixelToMmRatio = obj["pixelToMmRatio"].toDouble(1.0);
    }
    if (obj.contains("binaryThreshold"))
    {
        cfg.BinaryThreshold = obj["binaryThreshold"].toInt(128);
    }
    if (obj.contains("invertBinary"))
    {
        cfg.InvertBinary = obj["invertBinary"].toBool(false);
    }
    if (obj.contains("minRadius"))
    {
        cfg.MinRadius = obj["minRadius"].toDouble(5.0);
    }
    if (obj.contains("maxRadius"))
    {
        cfg.MaxRadius = obj["maxRadius"].toDouble(1000.0);
    }
    if (obj.contains("minContourArea"))
    {
        cfg.MinContourArea = obj["minContourArea"].toDouble(50.0);
    }
    return cfg;
}

QJsonObject MeasurementConfig::ToJson() const
{
    QJsonObject obj;
    obj["mode"] = QString::fromStdString(Mode);
    obj["pixelToMmRatio"] = PixelToMmRatio;
    obj["binaryThreshold"] = BinaryThreshold;
    obj["invertBinary"] = InvertBinary;
    obj["minRadius"] = MinRadius;
    obj["maxRadius"] = MaxRadius;
    obj["minContourArea"] = MinContourArea;
    return obj;
}

bool GeometryMeasurement::Execute(
    const cv::Mat& input,
    const MeasurementConfig& config,
    DetectionResult& result,
    std::string& errorMsg)
{
    if (input.empty())
    {
        errorMsg = "输入图像为空";
        return false;
    }

    auto start = std::chrono::high_resolution_clock::now();

    cv::Mat gray;
    if (input.channels() == 3)
    {
        cv::cvtColor(input, gray, cv::COLOR_BGR2GRAY);
    }
    else
    {
        gray = input.clone();
    }

    cv::Mat canvas = input.clone();
    if (canvas.channels() == 1)
    {
        cv::cvtColor(canvas, canvas, cv::COLOR_GRAY2BGR);
    }

    QJsonObject measurementsObj;

    if (config.Mode == "FindCircles")
    {
        cv::Mat blurred;
        cv::GaussianBlur(gray, blurred, cv::Size(5, 5), 1.5);
        std::vector<cv::Vec3f> circles;
        cv::HoughCircles(blurred, circles, cv::HOUGH_GRADIENT, 1.0,
                         config.MinRadius * 2, 100, 20,
                         static_cast<int>(config.MinRadius),
                         static_cast<int>(config.MaxRadius));

        if (circles.empty())
        {
            // 工业视觉回退：基于二值轮廓最小外接圆精确拟合
            cv::Mat binary;
            int threshType = config.InvertBinary ? cv::THRESH_BINARY_INV : cv::THRESH_BINARY;
            cv::threshold(gray, binary, config.BinaryThreshold, 255, threshType);

            std::vector<std::vector<cv::Point>> contours;
            cv::findContours(binary, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);
            for (const auto& pts : contours)
            {
                if (cv::contourArea(pts) < config.MinContourArea)
                {
                    continue;
                }
                cv::Point2f center;
                float radius = 0.0f;
                cv::minEnclosingCircle(pts, center, radius);
                if (radius >= config.MinRadius && radius <= config.MaxRadius)
                {
                    circles.push_back(cv::Vec3f(center.x, center.y, radius));
                }
            }
        }

        int circleIdx = 0;
        for (const auto& c : circles)
        {
            float cx = c[0];
            float cy = c[1];
            float r = c[2];
            float d = r * 2.0f;

            double dMm = d * config.PixelToMmRatio;
            double rMm = r * config.PixelToMmRatio;

            cv::circle(canvas, cv::Point(static_cast<int>(cx), static_cast<int>(cy)), static_cast<int>(r), cv::Scalar(0, 255, 255), 2);
            cv::drawMarker(canvas, cv::Point(static_cast<int>(cx), static_cast<int>(cy)), cv::Scalar(0, 0, 255), cv::MARKER_CROSS, 8, 2);

            std::string label = "D: " + std::to_string(dMm).substr(0, 5) + " mm";
            cv::putText(canvas, label, cv::Point(static_cast<int>(cx - r), static_cast<int>(cy - r - 5)),
                        cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0, 255, 255), 1, cv::LINE_AA);

            std::string key = "Circle_" + std::to_string(++circleIdx);
            QJsonObject item;
            item["centerX"] = cx;
            item["centerY"] = cy;
            item["radiusPx"] = r;
            item["diameterPx"] = d;
            item["radiusMm"] = rMm;
            item["diameterMm"] = dMm;
            measurementsObj[QString::fromStdString(key)] = item;
        }
    }
    else
    {
        // 轮廓外接矩形测量
        cv::Mat binary;
        int threshType = config.InvertBinary ? cv::THRESH_BINARY_INV : cv::THRESH_BINARY;
        cv::threshold(gray, binary, config.BinaryThreshold, 255, threshType);

        std::vector<std::vector<cv::Point>> contours;
        cv::findContours(binary, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

        int contourIdx = 0;
        for (const auto& pts : contours)
        {
            double area = cv::contourArea(pts);
            if (area < config.MinContourArea)
            {
                continue;
            }

            cv::RotatedRect rrect = cv::minAreaRect(pts);
            float w = std::max(rrect.size.width, rrect.size.height);
            float h = std::min(rrect.size.width, rrect.size.height);

            double wMm = w * config.PixelToMmRatio;
            double hMm = h * config.PixelToMmRatio;
            double areaMm2 = area * config.PixelToMmRatio * config.PixelToMmRatio;

            cv::Point2f vertices[4];
            rrect.points(vertices);
            for (int i = 0; i < 4; ++i)
            {
                cv::line(canvas, vertices[i], vertices[(i + 1) % 4], cv::Scalar(0, 220, 255), 2);
            }

            std::string label = "W:" + std::to_string(wMm).substr(0, 5) + " H:" + std::to_string(hMm).substr(0, 5);
            cv::putText(canvas, label, vertices[0], cv::FONT_HERSHEY_SIMPLEX, 0.45, cv::Scalar(0, 255, 255), 1, cv::LINE_AA);

            std::string key = "Part_" + std::to_string(++contourIdx);
            QJsonObject item;
            item["widthMm"] = wMm;
            item["heightMm"] = hMm;
            item["areaMm2"] = areaMm2;
            item["angle"] = rrect.angle;
            measurementsObj[QString::fromStdString(key)] = item;
        }
    }

    auto end = std::chrono::high_resolution_clock::now();
    double elapsedMs = std::chrono::duration<double, std::milli>(end - start).count();

    result.Objects.clear();
    result.Defects.clear();
    result.Measurements = measurementsObj;
    result.ExecutionTimeMs = elapsedMs;
    result.Visualization = canvas;

    Logger::Info("GeometryMeasurement", "几何测量完成, 耗时 " + std::to_string(elapsedMs) + " ms");
    return true;
}
