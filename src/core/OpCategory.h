#pragma once

#include <string>
#include <vector>

/**
 * @brief 算子分类细分枚举 (全面对标 OpenCV 工业机器视觉体系)
 */
enum class OpCategory
{
    ColorAndChannels,           // 🎨 颜色与通道操作 (Color & Channels)
    ImageFilter,                 // 🌫️ 滤波与平滑去噪 (Filtering & Denoise)
    Thresholding,                // 🌓 阈值分割与二值化 (Thresholding & Segmentation)
    MaskAndArithmetic,           // 🎭 掩码与双图复合 (Mask & Image Arithmetic)
    Morphology,                  // 🧱 形态学与结构运算 (Morphology Operations)
    GeometricTransform,          // 📐 几何与空间变换 (Geometry & Transformations)
    EdgeDetection,               // 🔍 边缘与梯度检测 (Edge & Gradient Detection)
    ContourAnalysis,             // 📊 轮廓与连通域分析 (Contours & Connected Components)
    HoughTransform,              // 📏 霍夫特征与几何检测 (Hough Transforms)
    FeatureAndMatch,             // 🎯 特征提取与模板匹配 (Features & Matching)
    EnhancementAndRestoration    // ✨ 图像增强与修复仿真 (Enhancement & Restoration)
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
            case OpCategory::ColorAndChannels:
                return "🎨 颜色与通道操作 (Color & Channels)";
            case OpCategory::ImageFilter:
                return "🌫️ 滤波与平滑去噪 (Filtering & Denoise)";
            case OpCategory::Thresholding:
                return "🌓 阈值分割与二值化 (Thresholding & Segmentation)";
            case OpCategory::MaskAndArithmetic:
                return "🎭 掩码与双图复合 (Mask & Image Arithmetic)";
            case OpCategory::Morphology:
                return "🧱 形态学与结构运算 (Morphology Operations)";
            case OpCategory::GeometricTransform:
                return "📐 几何与空间变换 (Geometry & Transformations)";
            case OpCategory::EdgeDetection:
                return "🔍 边缘与梯度检测 (Edge & Gradient Detection)";
            case OpCategory::ContourAnalysis:
                return "📊 轮廓与连通域分析 (Contours & Connected Components)";
            case OpCategory::HoughTransform:
                return "📏 霍夫特征与几何检测 (Hough Transforms)";
            case OpCategory::FeatureAndMatch:
                return "🎯 特征提取与模板匹配 (Features & Matching)";
            case OpCategory::EnhancementAndRestoration:
                return "✨ 图像增强与修复仿真 (Enhancement & Restoration)";
            default:
                return "📦 其他算子 (Other Operations)";
        }
    }

    static std::vector<OpCategory> GetAllCategories()
    {
        return {
            OpCategory::ColorAndChannels,
            OpCategory::ImageFilter,
            OpCategory::Thresholding,
            OpCategory::MaskAndArithmetic,
            OpCategory::Morphology,
            OpCategory::GeometricTransform,
            OpCategory::EdgeDetection,
            OpCategory::ContourAnalysis,
            OpCategory::HoughTransform,
            OpCategory::FeatureAndMatch,
            OpCategory::EnhancementAndRestoration
        };
    }
};
