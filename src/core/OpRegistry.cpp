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

    // 4. 掩码与双图复合 (Mask & Image Arithmetic)
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
