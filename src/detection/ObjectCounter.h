#pragma once

#include "DetectionTask.h"
#include <opencv2/opencv.hpp>
#include <string>

/**
 * @brief 目标计数配置结构
 */
struct ObjectCountConfig
{
    std::string Method = "ConnectedComponents"; // "ConnectedComponents" 或 "Contours"
    std::string ColorChannel = "Gray";          // "Gray", "Red", "Green", "Blue", "Hue"
    std::string ThresholdMethod = "Otsu";       // "Otsu", "Adaptive", "Manual"
    int ManualThreshold = 128;                  // 手动阈值 (0-255)
    bool InvertThreshold = false;               // 是否反转阈值
    int MorphKernelSize = 3;                    // 滤波形态学校正核大小 (奇数)
    std::string MorphType = "Open";             // "Open", "Close", "None"
    double MinArea = 30.0;                      // 最小有效面积 (像素)
    double MaxArea = 500000.0;                  // 最大有效面积 (像素)
    double MinWidth = 5.0;                      // 最小有效宽度
    double MinHeight = 5.0;                     // 最小有效高度
    double MinCircularity = 0.0;                // 最小圆度 (0.0 ~ 1.0，用于过滤瓶盖等圆形零件)
    double IouThreshold = 0.3;                  // 去重 NMS IoU 阈值
    cv::Rect Roi = cv::Rect(0, 0, 0, 0);        // ROI 感兴趣区域
    std::string TargetClass = "bottle_cap";     // 目标类别标识名
    std::string TargetDisplayName = "瓶盖";     // 目标中文显示名

    static ObjectCountConfig FromJson(const QJsonObject& obj);
    QJsonObject ToJson() const;
};

/**
 * @brief 工业通用目标检测与计数引擎 (ObjectCounter)
 * 
 * 遵循工业软件模块化与 SRP 原则：
 * 负责目标提取 (连通域/轮廓) -> 几何特征计算 -> 面积/尺寸/圆度筛选 -> NMS 去重 -> 结果可视化渲染。
 */
class ObjectCounter
{
public:
    /**
     * @brief 执行目标计数分析
     * @param input 输入图像 (BGR 彩色或灰度)
     * @param config 计数参数配置
     * @param result 输出结构化检测结果与可视化图
     * @param errorMsg 错误输出
     * @return 成功返回 true，失败返回 false
     */
    static bool Execute(
        const cv::Mat& input,
        const ObjectCountConfig& config,
        DetectionResult& result,
        std::string& errorMsg);

private:
    static cv::Mat Preprocess(const cv::Mat& input, const ObjectCountConfig& config);
    static void ApplyNms(std::vector<BoundingBox>& boxes, double iouThreshold);
    static void RenderVisualization(cv::Mat& canvas, const std::vector<BoundingBox>& objects, const ObjectCountConfig& config);
};
