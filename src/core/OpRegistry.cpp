#include "OpRegistry.h"
#include "operators/ColorOps.h"
#include "operators/FilterOps.h"
#include "operators/ThresholdOps.h"
#include "operators/MorphologyOps.h"
#include "operators/EdgeOps.h"
#include "operators/TransformOps.h"
#include "operators/EnhanceOps.h"
#include "operators/ContourOps.h"
#include "operators/HoughOps.h"
#include "operators/FeatureOps.h"
#include "operators/ArithmeticOps.h"
#include "operators/BranchOps.h"
#include "operators/InteractiveMaskOp.h"
#include "operators/AdvancedOps.h"
#include "operators/IndustrialOps.h"
#include "operators/GeneralOps.h"
#include <algorithm>

std::unique_ptr<IOpNode> OpRegistry::Create(const std::string& name) const
{
    auto it = _creators.find(name);
    if (it != _creators.end())
    {
        return it->second();
    }
    return nullptr;
}

std::vector<OpMetadata> OpRegistry::GetAllOperators() const
{
    std::vector<OpMetadata> list;
    for (const auto& pair : _metadataMap)
    {
        list.push_back(pair.second);
    }
    return list;
}

std::vector<OpMetadata> OpRegistry::GetOperatorsByCategory(OpCategory category) const
{
    std::vector<OpMetadata> list;
    for (const auto& pair : _metadataMap)
    {
        if (pair.second.Category == category)
        {
            list.push_back(pair.second);
        }
    }
    return list;
}

std::vector<OpMetadata> OpRegistry::SearchOperators(const std::string& keyword) const
{
    std::vector<OpMetadata> list;
    if (keyword.empty())
    {
        return GetAllOperators();
    }

    std::string lowerKeyword = keyword;
    std::transform(lowerKeyword.begin(), lowerKeyword.end(), lowerKeyword.begin(), ::tolower);

    for (const auto& pair : _metadataMap)
    {
        std::string lowerName = pair.second.Name;
        std::string lowerDisplay = pair.second.DisplayName;
        std::string lowerDesc = pair.second.Description;

        std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);
        std::transform(lowerDisplay.begin(), lowerDisplay.end(), lowerDisplay.begin(), ::tolower);
        std::transform(lowerDesc.begin(), lowerDesc.end(), lowerDesc.begin(), ::tolower);

        if (lowerName.find(lowerKeyword) != std::string::npos ||
            lowerDisplay.find(lowerKeyword) != std::string::npos ||
            lowerDesc.find(lowerKeyword) != std::string::npos)
        {
            list.push_back(pair.second);
        }
    }
    return list;
}

void OpRegistry::RegisterAllBuiltinOperators()
{
    // 1. 颜色与通道操作 (Color & Channels)
    Register<CvtColorOp>();
    Register<ChannelSplitOp>();
    Register<LUTOp>();

    // 2. 滤波与图像平滑 (Filtering & Denoise)
    Register<GaussianBlurOp>();
    Register<BlurOp>();
    Register<BoxFilterOp>();
    Register<MedianBlurOp>();
    Register<BilateralFilterOp>();
    Register<Filter2DOp>();

    // 3. 阈值分割与二值化 (Thresholding & Segmentation)
    Register<ThresholdOp>();
    Register<OtsuThresholdOp>();
    Register<AdaptiveThresholdOp>();
    Register<InRangeOp>();
    Register<ThresholdTruncateOp>();

    // 4. 掩码与双图复合运算 (Mask & Branch Arithmetic)
    Register<InteractiveMaskOp>();
    Register<MaskApplyOp>();
    Register<BlendWithSourceOp>();

    // 5. 形态学与结构运算 (Morphology Operations)
    Register<DilateOp>();
    Register<ErodeOp>();
    Register<MorphologyExOp>();
    Register<SkeletonThinningOp>();

    // 6. 几何与空间变换 (Geometry & Transformations)
    Register<ResizeOp>();
    Register<RotateOp>();
    Register<FlipOp>();
    Register<CropRoiOp>();
    Register<PerspectiveTransformOp>();
    Register<PolarTransformOp>();
    Register<CylindricalUnrollOp>();
    Register<ImagePyramidOp>();

    // 7. 边缘与梯度检测 (Edge & Gradient Detection)
    Register<CannyOp>();
    Register<SobelOp>();
    Register<ScharrOp>();
    Register<LaplacianOp>();

    // 8. 轮廓与连通域分析 (Contours & Connected Components)
    Register<FindContoursOp>();
    Register<ConnectedComponentsOp>();
    Register<DistanceTransformOp>();

    // 9. 霍夫特征与几何检测 (Hough Transforms)
    Register<HoughLinesPOp>();
    Register<HoughCirclesOp>();
    Register<FitLineOp>();
    Register<RansacFitCircleOp>();

    // 10. 特征提取与模板匹配 (Features & Matching)
    Register<CornerHarrisOp>();
    Register<GoodFeaturesToTrackOp>();
    Register<MatchTemplateOp>();
    Register<BarcodeQrOp>();

    // 11. 图像增强与修复仿真 (Enhancement & Restoration)
    Register<BrightnessContrastOp>();
    Register<EqualizeHistOp>();
    Register<CLAHEOp>();
    Register<GammaCorrectionOp>();
    Register<InvertOp>();
    Register<SharpenOp>();
    Register<InpaintOp>();
    Register<NoiseInjectionOp>();

    // 12. 通用 OpenCV 算法扩展 (General OpenCV Primitives)
    Register<ChannelMergeOp>();
    Register<NormalizeOp>();
    Register<SeparableFilterOp>();
    Register<AffineTransformOp>();
    Register<ImageStatisticsOp>();
    Register<HistogramCompareOp>();
    Register<HistogramMatchOp>();
    Register<MomentsHuOp>();
}

OperatorMetadata OpRegistry::GetOperatorMetadata(const std::string& name) const
{
    auto it = _creators.find(name);
    if (it != _creators.end())
    {
        auto node = it->second();
        if (node)
        {
            return node->GetMetadata();
        }
    }
    return OperatorMetadata{};
}

QJsonArray OpRegistry::ExportAllMetadataJson() const
{
    QJsonArray array;
    for (const auto& pair : _creators)
    {
        auto node = pair.second();
        if (node)
        {
            array.append(node->GetMetadata().ToJson());
        }
    }
    return array;
}

std::string OpRegistry::ExportAllMetadataPromptString() const
{
    std::string prompt;
    prompt += "系统可用图像处理算子列表与参数规范:\n";
    for (const auto& pair : _creators)
    {
        auto node = pair.second();
        if (!node)
        {
            continue;
        }

        OperatorMetadata meta = node->GetMetadata();
        prompt += "- 算子名: " + meta.Name + " (" + meta.DisplayName + ")\n";
        prompt += "  分类: " + meta.Category + " | 功能: " + meta.Description + "\n";
        if (!meta.Parameters.empty())
        {
            prompt += "  参数:\n";
            for (const auto& p : meta.Parameters)
            {
                prompt += "    * " + p.Name + " (" + p.DisplayName + ", 类型: " + p.Type;
                if (p.Type == "int" || p.Type == "double")
                {
                    prompt += ", 范围: [" + std::to_string(p.MinValue) + ", " + std::to_string(p.MaxValue) + "]";
                }
                else if (p.Type == "enum" && !p.EnumValues.empty())
                {
                    prompt += ", 枚举: [";
                    for (size_t i = 0; i < p.EnumValues.size(); ++i)
                    {
                        if (i > 0)
                        {
                            prompt += ", ";
                        }
                        prompt += std::to_string(i) + ":" + p.EnumValues[i];
                    }
                    prompt += "]";
                }
                prompt += ", 默认: " + p.DefaultValue + ")\n";
            }
        }
    }
    return prompt;
}
