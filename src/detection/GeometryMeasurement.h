#pragma once

#include "DetectionTask.h"
#include <opencv2/opencv.hpp>
#include <string>

/**
 * @brief 几何测量配置
 */
struct MeasurementConfig
{
    std::string Mode = "FindCircles";           // "FindCircles", "FindContours", "MinAreaRect"
    double PixelToMmRatio = 1.0;                // 像素到物理毫米标定系数 (mm/px)
    int BinaryThreshold = 128;                  // 二值化阈值
    bool InvertBinary = false;                  // 是否反转二值化
    double MinRadius = 5.0;                     // 拟合圆最小半径 (px)
    double MaxRadius = 1000.0;                  // 拟合圆最大半径 (px)
    double MinContourArea = 50.0;               // 最小轮廓面积

    static MeasurementConfig FromJson(const QJsonObject& obj);
    QJsonObject ToJson() const;
};

/**
 * @brief 工业几何尺寸与外形测量引擎
 */
class GeometryMeasurement
{
public:
    static bool Execute(
        const cv::Mat& input,
        const MeasurementConfig& config,
        DetectionResult& result,
        std::string& errorMsg);
};
