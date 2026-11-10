#pragma once

#include "DetectionTask.h"
#include <opencv2/opencv.hpp>
#include <string>

/**
 * @brief 表面缺陷检测参数配置
 */
struct DefectDetectConfig
{
    std::string Method = "BackgroundSubtraction"; // "BackgroundSubtraction", "TopHat", "AdaptiveThreshold", "DualThreshold"
    int BackgroundBlurKernel = 31;                // 背景光照估计高斯核大小 (奇数)
    int DiffThreshold = 25;                       // 亮暗差分阈值 (0-255)
    bool DetectBrightDefects = true;              // 是否检测亮斑/高反光缺陷
    bool DetectDarkDefects = true;                // 是否检测暗痕/黑斑/划痕
    int TopHatKernelSize = 15;                    // Top-Hat 算子核大小
    int MorphCleanKernel = 3;                     // 杂讯消除形态学核大小
    double MinDefectArea = 15.0;                  // 缺陷最小判定面积 (像素)
    double MaxDefectArea = 50000.0;               // 缺陷最大面积 (像素)
    double MinDefectLength = 4.0;                 // 缺陷最小长度 (像素)
    cv::Rect Roi = cv::Rect(0, 0, 0, 0);          // 工件检测 ROI 区域

    static DefectDetectConfig FromJson(const QJsonObject& obj);
    QJsonObject ToJson() const;
};

/**
 * @brief 工业工件表面缺陷检测引擎 (DefectDetector)
 * 
 * 适用于金属、玻璃、塑料、注塑件等表面缺陷检测：
 * 支持光照不均自适应校正、差分/形态学亮暗缺陷候选提取、几何特征计算与智能缺陷分类 (划痕/凹坑/污渍)。
 */
class DefectDetector
{
public:
    /**
     * @brief 执行工件表面缺陷检测
     * @param input 输入图像
     * @param config 检测配置
     * @param result 输出结构化检测结果
     * @param errorMsg 错误输出
     * @return 成功返回 true，失败返回 false
     */
    static bool Execute(
        const cv::Mat& input,
        const DefectDetectConfig& config,
        DetectionResult& result,
        std::string& errorMsg);

private:
    static cv::Mat ExtractDefectMask(const cv::Mat& gray, const DefectDetectConfig& config);
    static void AnalyzeDefectCandidates(
        const cv::Mat& defectMask,
        const cv::Point& roiOffset,
        const DefectDetectConfig& config,
        std::vector<DefectInfo>& defects);
    static void RenderVisualization(
        cv::Mat& canvas,
        const cv::Mat& defectMask,
        const cv::Point& roiOffset,
        const std::vector<DefectInfo>& defects,
        const DefectDetectConfig& config);
};
