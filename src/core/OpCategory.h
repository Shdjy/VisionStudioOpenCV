#pragma once

#include <string>
#include <vector>

/**
 * @brief 算子分类枚举
 */
enum class OpCategory
{
    ColorConversion,     // 色彩空间与通道
    ImageFilter,         // 滤波与平滑
    Thresholding,        // 阈值与二值化
    Morphology,          // 形态学运算
    EdgeDetection,       // 边缘与梯度
    GeometricTransform,  // 几何变换与校正
    ImageEnhancement,    // 图像增强与算术
    ContourAnalysis,     // 轮廓与几何分析
    HoughTransform,      // 霍夫直线与圆检测
    FeatureAndMatch      // 特征与模式匹配
};

/**
 * @brief 算子分类辅助工具类
 */
class OpCategoryUtils
{
public:
    static std::string ToString(OpCategory category)
    {
        switch (category)
        {
            case OpCategory::ColorConversion:
                return "色彩空间与通道 (Color)";
            case OpCategory::ImageFilter:
                return "滤波与平滑 (Filter)";
            case OpCategory::Thresholding:
                return "阈值与二值化 (Threshold)";
            case OpCategory::Morphology:
                return "形态学运算 (Morphology)";
            case OpCategory::EdgeDetection:
                return "边缘与梯度 (Edge)";
            case OpCategory::GeometricTransform:
                return "几何变换 (Transform)";
            case OpCategory::ImageEnhancement:
                return "图像增强 (Enhance)";
            case OpCategory::ContourAnalysis:
                return "轮廓与分析 (Contour)";
            case OpCategory::HoughTransform:
                return "霍夫检测 (Hough)";
            case OpCategory::FeatureAndMatch:
                return "特征与匹配 (Feature)";
            default:
                return "其他算子 (Other)";
        }
    }

    static std::vector<OpCategory> GetAllCategories()
    {
        return {
            OpCategory::ColorConversion,
            OpCategory::ImageFilter,
            OpCategory::Thresholding,
            OpCategory::Morphology,
            OpCategory::EdgeDetection,
            OpCategory::GeometricTransform,
            OpCategory::ImageEnhancement,
            OpCategory::ContourAnalysis,
            OpCategory::HoughTransform,
            OpCategory::FeatureAndMatch
        };
    }
};
