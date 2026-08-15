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
    // 1. 色彩空间
    Register<CvtColorOp>();
    Register<ChannelSplitOp>();

    // 2. 滤波与平滑
    Register<GaussianBlurOp>();
    Register<BlurOp>();
    Register<MedianBlurOp>();
    Register<BilateralFilterOp>();
    Register<Filter2DOp>();

    // 3. 阈值与二值化
    Register<ThresholdOp>();
    Register<OtsuThresholdOp>();
    Register<AdaptiveThresholdOp>();
    Register<InRangeOp>();

    // 4. 形态学
    Register<DilateOp>();
    Register<ErodeOp>();
    Register<MorphologyExOp>();

    // 5. 边缘与梯度
    Register<CannyOp>();
    Register<SobelOp>();
    Register<ScharrOp>();
    Register<LaplacianOp>();

    // 6. 几何变换
    Register<ResizeOp>();
    Register<RotateOp>();
    Register<FlipOp>();
    Register<CropRoiOp>();

    // 7. 图像增强
    Register<BrightnessContrastOp>();
    Register<EqualizeHistOp>();
    Register<CLAHEOp>();
    Register<GammaCorrectionOp>();
    Register<InvertOp>();
    Register<SharpenOp>();

    // 8. 轮廓分析
    Register<FindContoursOp>();

    // 9. 霍夫检测
    Register<HoughLinesPOp>();
    Register<HoughCirclesOp>();

    // 10. 特征与匹配
    Register<CornerHarrisOp>();
    Register<GoodFeaturesToTrackOp>();
    Register<MatchTemplateOp>();
}
