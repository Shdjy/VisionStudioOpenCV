#include "GeneralOps.h"
#include "../PipelineContext.h"
#include <algorithm>
#include <cmath>
#include <iomanip>
#include <sstream>

namespace
{
OpParameter MakeCombo(const std::string& name,
                      const std::string& displayName,
                      int value,
                      const std::vector<std::string>& options)
{
    OpParameter parameter;
    parameter.Name = name;
    parameter.DisplayName = displayName;
    parameter.Type = ParamType::ComboBox;
    parameter.Value = value;
    parameter.EnumOptions = options;
    return parameter;
}

OpParameter MakeInt(const std::string& name,
                    const std::string& displayName,
                    int value,
                    int minValue,
                    int maxValue,
                    int step = 1)
{
    OpParameter parameter;
    parameter.Name = name;
    parameter.DisplayName = displayName;
    parameter.Type = ParamType::IntSlider;
    parameter.Value = value;
    parameter.MinValue = minValue;
    parameter.MaxValue = maxValue;
    parameter.Step = step;
    return parameter;
}

OpParameter MakeDouble(const std::string& name,
                       const std::string& displayName,
                       double value,
                       double minValue,
                       double maxValue,
                       double step)
{
    OpParameter parameter;
    parameter.Name = name;
    parameter.DisplayName = displayName;
    parameter.Type = ParamType::DoubleSlider;
    parameter.Value = value;
    parameter.MinValue = minValue;
    parameter.MaxValue = maxValue;
    parameter.Step = step;
    return parameter;
}

OpParameter MakeBool(const std::string& name, const std::string& displayName, bool value)
{
    OpParameter parameter;
    parameter.Name = name;
    parameter.DisplayName = displayName;
    parameter.Type = ParamType::CheckBox;
    parameter.Value = value;
    return parameter;
}

cv::Mat ToGray(const cv::Mat& input)
{
    if (input.channels() == 1)
    {
        return input;
    }

    cv::Mat gray;
    if (input.channels() == 4)
    {
        cv::cvtColor(input, gray, cv::COLOR_BGRA2GRAY);
    }
    else
    {
        cv::cvtColor(input, gray, cv::COLOR_BGR2GRAY);
    }
    return gray;
}

cv::Mat ToBgr(const cv::Mat& input)
{
    if (input.channels() == 3)
    {
        return input;
    }

    cv::Mat bgr;
    if (input.channels() == 4)
    {
        cv::cvtColor(input, bgr, cv::COLOR_BGRA2BGR);
    }
    else
    {
        cv::cvtColor(input, bgr, cv::COLOR_GRAY2BGR);
    }
    return bgr;
}

cv::Mat MatchHistogramChannel(const cv::Mat& input, const cv::Mat& reference)
{
    int histSize = 256;
    float range[] = { 0.0f, 256.0f };
    const float* ranges[] = { range };
    cv::Mat inputHist;
    cv::Mat referenceHist;
    cv::calcHist(&input, 1, nullptr, cv::Mat(), inputHist, 1, &histSize, ranges, true, false);
    cv::calcHist(&reference, 1, nullptr, cv::Mat(), referenceHist, 1, &histSize, ranges, true, false);

    inputHist /= static_cast<float>(input.total());
    referenceHist /= static_cast<float>(reference.total());
    for (int index = 1; index < histSize; ++index)
    {
        inputHist.at<float>(index) += inputHist.at<float>(index - 1);
        referenceHist.at<float>(index) += referenceHist.at<float>(index - 1);
    }

    cv::Mat lookup(1, histSize, CV_8U);
    int referenceIndex = 0;
    for (int inputIndex = 0; inputIndex < histSize; ++inputIndex)
    {
        const float inputCdf = inputHist.at<float>(inputIndex);
        while (referenceIndex < histSize - 1 &&
               referenceHist.at<float>(referenceIndex) < inputCdf)
        {
            ++referenceIndex;
        }
        lookup.at<unsigned char>(inputIndex) = static_cast<unsigned char>(referenceIndex);
    }

    cv::Mat matched;
    cv::LUT(input, lookup, matched);
    return matched;
}
}

ChannelMergeOp::ChannelMergeOp()
{
    const std::vector<std::string> sources = {
        "蓝通道 B", "绿通道 G", "红通道 R", "灰度通道", "常量 0"
    };
    _params.push_back(MakeCombo("blueSource", "输出 B 通道来源", 0, sources));
    _params.push_back(MakeCombo("greenSource", "输出 G 通道来源", 1, sources));
    _params.push_back(MakeCombo("redSource", "输出 R 通道来源", 2, sources));
}

std::string ChannelMergeOp::GetPrincipleDoc() const
{
    return "### 通道重组\n\n`cv::merge` 将多个等尺寸、等深度的单通道矩阵合成为多通道矩阵。"
           "本算子允许重复、交换或清零 B/G/R 通道，也可把灰度通道复制到任意输出通道。";
}

bool ChannelMergeOp::Process(const cv::Mat& input, cv::Mat& output, std::string& errorMsg)
{
    ClearResults();
    if (input.empty())
    {
        errorMsg = "输入图像为空";
        return false;
    }

    try
    {
        cv::Mat bgr = ToBgr(input);
        cv::Mat gray = ToGray(input);
        std::vector<cv::Mat> channels;
        cv::split(bgr, channels);
        cv::Mat zero = cv::Mat::zeros(input.size(), input.depth());

        auto selectChannel = [&](int sourceIndex) -> cv::Mat {
            if (sourceIndex >= 0 && sourceIndex <= 2)
            {
                return channels[sourceIndex];
            }
            return sourceIndex == 3 ? gray : zero;
        };

        std::vector<cv::Mat> mergedChannels = {
            selectChannel(_params[0].GetInt()),
            selectChannel(_params[1].GetInt()),
            selectChannel(_params[2].GetInt())
        };
        cv::merge(mergedChannels, output);
        return true;
    }
    catch (const cv::Exception& ex)
    {
        errorMsg = ex.what();
        return false;
    }
}

std::string ChannelMergeOp::GenerateCppCode(const std::string& inVar, const std::string& outVar) const
{
    std::ostringstream stream;
    stream << "cv::Mat mergeBgr, mergeGray;\n"
           << "if (" << inVar << ".channels() == 1) cv::cvtColor(" << inVar << ", mergeBgr, cv::COLOR_GRAY2BGR);\n"
           << "else if (" << inVar << ".channels() == 4) cv::cvtColor(" << inVar << ", mergeBgr, cv::COLOR_BGRA2BGR);\n"
           << "else mergeBgr = " << inVar << ";\n"
           << "if (" << inVar << ".channels() == 1) mergeGray = " << inVar << ";\n"
           << "else cv::cvtColor(mergeBgr, mergeGray, cv::COLOR_BGR2GRAY);\n"
           << "std::vector<cv::Mat> mergeChannels; cv::split(mergeBgr, mergeChannels);\n"
           << "cv::Mat mergeZero = cv::Mat::zeros(" << inVar << ".size(), " << inVar << ".depth());\n"
           << "auto selectMergeChannel = [&](int index) { return index <= 2 ? mergeChannels[index] : (index == 3 ? mergeGray : mergeZero); };\n"
           << "std::vector<cv::Mat> selectedChannels = { selectMergeChannel(" << _params[0].GetInt()
           << "), selectMergeChannel(" << _params[1].GetInt() << "), selectMergeChannel(" << _params[2].GetInt() << ") };\n"
           << "cv::merge(selectedChannels, " << outVar << ");";
    return stream.str();
}

std::string ChannelMergeOp::GeneratePythonCode(const std::string& inVar, const std::string& outVar) const
{
    std::ostringstream stream;
    stream << "merge_bgr = cv2.cvtColor(" << inVar << ", cv2.COLOR_GRAY2BGR) if len(" << inVar << ".shape) == 2 else " << inVar << "[:, :, :3]\n"
           << "merge_gray = cv2.cvtColor(merge_bgr, cv2.COLOR_BGR2GRAY)\n"
           << "merge_channels = list(cv2.split(merge_bgr)) + [merge_gray, np.zeros_like(merge_gray)]\n"
           << outVar << " = cv2.merge([merge_channels[" << _params[0].GetInt() << "], merge_channels["
           << _params[1].GetInt() << "], merge_channels[" << _params[2].GetInt() << "]])";
    return stream.str();
}

NormalizeOp::NormalizeOp()
{
    _params.push_back(MakeCombo("normType", "归一化类型", 0,
        { "MINMAX", "L1", "L2", "INF" }));
    _params.push_back(MakeDouble("alpha", "Alpha / 范数目标值", 0.0, 0.0, 255.0, 1.0));
    _params.push_back(MakeDouble("beta", "Beta (MINMAX 上限)", 255.0, 0.0, 255.0, 1.0));
    _params.push_back(MakeCombo("outputDepth", "输出深度", 0,
        { "保持输入深度", "CV_8U", "CV_32F" }));
}

std::string NormalizeOp::GetPrincipleDoc() const
{
    return "### 矩阵归一化\n\nMINMAX 把数值线性映射到指定区间；L1、L2 和 INF 则把矩阵对应范数缩放到目标值。";
}

bool NormalizeOp::Process(const cv::Mat& input, cv::Mat& output, std::string& errorMsg)
{
    ClearResults();
    if (input.empty())
    {
        errorMsg = "输入图像为空";
        return false;
    }

    const int typeIndex = _params[0].GetInt();
    const int normTypes[] = { cv::NORM_MINMAX, cv::NORM_L1, cv::NORM_L2, cv::NORM_INF };
    const int depthIndex = _params[3].GetInt();
    const int depth = depthIndex == 1 ? CV_8U : (depthIndex == 2 ? CV_32F : -1);
    double alpha = _params[1].GetDouble();
    const double beta = _params[2].GetDouble();
    if (typeIndex != 0 && alpha <= 0.0)
    {
        alpha = 1.0;
    }

    try
    {
        cv::normalize(input, output, alpha, beta, normTypes[typeIndex], depth);
        return true;
    }
    catch (const cv::Exception& ex)
    {
        errorMsg = ex.what();
        return false;
    }
}

std::string NormalizeOp::GenerateCppCode(const std::string& inVar, const std::string& outVar) const
{
    const char* normTypes[] = { "cv::NORM_MINMAX", "cv::NORM_L1", "cv::NORM_L2", "cv::NORM_INF" };
    const char* depths[] = { "-1", "CV_8U", "CV_32F" };
    double alpha = _params[1].GetDouble();
    if (_params[0].GetInt() != 0 && alpha <= 0.0)
    {
        alpha = 1.0;
    }
    std::ostringstream stream;
    stream << "cv::normalize(" << inVar << ", " << outVar << ", " << alpha << ", "
           << _params[2].GetDouble() << ", " << normTypes[_params[0].GetInt()] << ", "
           << depths[_params[3].GetInt()] << ");";
    return stream.str();
}

std::string NormalizeOp::GeneratePythonCode(const std::string& inVar, const std::string& outVar) const
{
    const char* normTypes[] = { "cv2.NORM_MINMAX", "cv2.NORM_L1", "cv2.NORM_L2", "cv2.NORM_INF" };
    const char* depths[] = { "-1", "cv2.CV_8U", "cv2.CV_32F" };
    double alpha = _params[1].GetDouble();
    if (_params[0].GetInt() != 0 && alpha <= 0.0)
    {
        alpha = 1.0;
    }
    std::ostringstream stream;
    stream << outVar << " = cv2.normalize(" << inVar << ", None, " << alpha << ", "
           << _params[2].GetDouble() << ", " << normTypes[_params[0].GetInt()] << ", dtype="
           << depths[_params[3].GetInt()] << ")";
    return stream.str();
}

SeparableFilterOp::SeparableFilterOp()
{
    _params.push_back(MakeCombo("kernelType", "一维核类型", 0,
        { "高斯核", "Sobel X", "Sobel Y" }));
    _params.push_back(MakeInt("ksize", "核大小", 5, 1, 31, 2));
    _params.push_back(MakeDouble("sigma", "高斯 Sigma", 1.2, 0.1, 10.0, 0.1));
}

std::string SeparableFilterOp::GetPrincipleDoc() const
{
    return "### 可分离滤波\n\n当二维卷积核可写为列向量与行向量的外积时，可用两次一维卷积把复杂度从 O(K^2) 降到 O(2K)。";
}

bool SeparableFilterOp::Process(const cv::Mat& input, cv::Mat& output, std::string& errorMsg)
{
    ClearResults();
    if (input.empty())
    {
        errorMsg = "输入图像为空";
        return false;
    }

    int kernelSize = _params[1].GetInt();
    if (kernelSize % 2 == 0)
    {
        ++kernelSize;
    }
    const int kernelType = _params[0].GetInt();

    try
    {
        cv::Mat kernelX;
        cv::Mat kernelY;
        if (kernelType == 0)
        {
            kernelX = cv::getGaussianKernel(kernelSize, _params[2].GetDouble(), CV_64F);
            kernelY = kernelX;
            cv::sepFilter2D(input, output, -1, kernelX, kernelY);
        }
        else
        {
            const int dx = kernelType == 1 ? 1 : 0;
            const int dy = kernelType == 2 ? 1 : 0;
            cv::getDerivKernels(kernelX, kernelY, dx, dy, kernelSize, true, CV_64F);
            cv::Mat signedOutput;
            cv::sepFilter2D(input, signedOutput, CV_16S, kernelX, kernelY);
            cv::convertScaleAbs(signedOutput, output);
        }
        return true;
    }
    catch (const cv::Exception& ex)
    {
        errorMsg = ex.what();
        return false;
    }
}

std::string SeparableFilterOp::GenerateCppCode(const std::string& inVar, const std::string& outVar) const
{
    int kernelSize = _params[1].GetInt();
    if (kernelSize % 2 == 0)
    {
        ++kernelSize;
    }
    const int kernelType = _params[0].GetInt();
    std::ostringstream stream;
    stream << "cv::Mat kernelX, kernelY;\n";
    if (kernelType == 0)
    {
        stream << "kernelX = cv::getGaussianKernel(" << kernelSize << ", " << _params[2].GetDouble() << ", CV_64F);\n"
               << "kernelY = kernelX;\n"
               << "cv::sepFilter2D(" << inVar << ", " << outVar << ", -1, kernelX, kernelY);";
    }
    else
    {
        stream << "cv::getDerivKernels(kernelX, kernelY, " << (kernelType == 1 ? 1 : 0) << ", "
               << (kernelType == 2 ? 1 : 0) << ", " << kernelSize << ", true, CV_64F);\n"
               << "cv::Mat separableSigned;\n"
               << "cv::sepFilter2D(" << inVar << ", separableSigned, CV_16S, kernelX, kernelY);\n"
               << "cv::convertScaleAbs(separableSigned, " << outVar << ");";
    }
    return stream.str();
}

std::string SeparableFilterOp::GeneratePythonCode(const std::string& inVar, const std::string& outVar) const
{
    int kernelSize = _params[1].GetInt();
    if (kernelSize % 2 == 0)
    {
        ++kernelSize;
    }
    const int kernelType = _params[0].GetInt();
    std::ostringstream stream;
    if (kernelType == 0)
    {
        stream << "kernel_x = cv2.getGaussianKernel(" << kernelSize << ", " << _params[2].GetDouble() << ")\n"
               << outVar << " = cv2.sepFilter2D(" << inVar << ", -1, kernel_x, kernel_x)";
    }
    else
    {
        stream << "kernel_x, kernel_y = cv2.getDerivKernels(" << (kernelType == 1 ? 1 : 0) << ", "
               << (kernelType == 2 ? 1 : 0) << ", " << kernelSize << ", normalize=True)\n"
               << "separable_signed = cv2.sepFilter2D(" << inVar << ", cv2.CV_16S, kernel_x, kernel_y)\n"
               << outVar << " = cv2.convertScaleAbs(separable_signed)";
    }
    return stream.str();
}

AffineTransformOp::AffineTransformOp()
{
    _params.push_back(MakeDouble("angle", "旋转角度", 0.0, -180.0, 180.0, 1.0));
    _params.push_back(MakeDouble("scale", "缩放比例", 1.0, 0.1, 4.0, 0.05));
    _params.push_back(MakeDouble("translateX", "水平平移像素", 0.0, -2000.0, 2000.0, 1.0));
    _params.push_back(MakeDouble("translateY", "垂直平移像素", 0.0, -2000.0, 2000.0, 1.0));
    _params.push_back(MakeDouble("shearX", "水平错切角度", 0.0, -60.0, 60.0, 1.0));
    _params.push_back(MakeDouble("shearY", "垂直错切角度", 0.0, -60.0, 60.0, 1.0));
    _params.push_back(MakeCombo("interpolation", "插值方式", 1,
        { "最近邻", "双线性", "双三次" }));
}

std::string AffineTransformOp::GetPrincipleDoc() const
{
    return "### 二维仿射变换\n\n仿射矩阵保持直线和平行关系，可统一表达旋转、缩放、平移和错切。`warpAffine` 使用 2x3 矩阵完成反向采样。";
}

bool AffineTransformOp::Process(const cv::Mat& input, cv::Mat& output, std::string& errorMsg)
{
    ClearResults();
    if (input.empty())
    {
        errorMsg = "输入图像为空";
        return false;
    }

    try
    {
        const cv::Point2f center(input.cols / 2.0f, input.rows / 2.0f);
        cv::Mat matrix = cv::getRotationMatrix2D(center, _params[0].GetDouble(), _params[1].GetDouble());
        matrix.at<double>(0, 2) += _params[2].GetDouble();
        matrix.at<double>(1, 2) += _params[3].GetDouble();
        constexpr double DegreesToRadians = CV_PI / 180.0;
        matrix.at<double>(0, 1) += std::tan(_params[4].GetDouble() * DegreesToRadians);
        matrix.at<double>(1, 0) += std::tan(_params[5].GetDouble() * DegreesToRadians);
        const int interpolations[] = { cv::INTER_NEAREST, cv::INTER_LINEAR, cv::INTER_CUBIC };
        cv::warpAffine(input, output, matrix, input.size(), interpolations[_params[6].GetInt()], cv::BORDER_CONSTANT);
        return true;
    }
    catch (const cv::Exception& ex)
    {
        errorMsg = ex.what();
        return false;
    }
}

std::string AffineTransformOp::GenerateCppCode(const std::string& inVar, const std::string& outVar) const
{
    const char* interpolations[] = { "cv::INTER_NEAREST", "cv::INTER_LINEAR", "cv::INTER_CUBIC" };
    std::ostringstream stream;
    stream << "cv::Point2f affineCenter(" << inVar << ".cols / 2.0f, " << inVar << ".rows / 2.0f);\n"
           << "cv::Mat affineMatrix = cv::getRotationMatrix2D(affineCenter, " << _params[0].GetDouble() << ", " << _params[1].GetDouble() << ");\n"
           << "affineMatrix.at<double>(0, 2) += " << _params[2].GetDouble() << ";\n"
           << "affineMatrix.at<double>(1, 2) += " << _params[3].GetDouble() << ";\n"
           << "affineMatrix.at<double>(0, 1) += std::tan(" << _params[4].GetDouble() << " * CV_PI / 180.0);\n"
           << "affineMatrix.at<double>(1, 0) += std::tan(" << _params[5].GetDouble() << " * CV_PI / 180.0);\n"
           << "cv::warpAffine(" << inVar << ", " << outVar << ", affineMatrix, " << inVar << ".size(), "
           << interpolations[_params[6].GetInt()] << ", cv::BORDER_CONSTANT);";
    return stream.str();
}

std::string AffineTransformOp::GeneratePythonCode(const std::string& inVar, const std::string& outVar) const
{
    const char* interpolations[] = { "cv2.INTER_NEAREST", "cv2.INTER_LINEAR", "cv2.INTER_CUBIC" };
    std::ostringstream stream;
    stream << "affine_center = (" << inVar << ".shape[1] / 2.0, " << inVar << ".shape[0] / 2.0)\n"
           << "affine_matrix = cv2.getRotationMatrix2D(affine_center, " << _params[0].GetDouble() << ", " << _params[1].GetDouble() << ")\n"
           << "affine_matrix[0, 2] += " << _params[2].GetDouble() << "\n"
           << "affine_matrix[1, 2] += " << _params[3].GetDouble() << "\n"
           << "affine_matrix[0, 1] += np.tan(np.deg2rad(" << _params[4].GetDouble() << "))\n"
           << "affine_matrix[1, 0] += np.tan(np.deg2rad(" << _params[5].GetDouble() << "))\n"
           << outVar << " = cv2.warpAffine(" << inVar << ", affine_matrix, (" << inVar << ".shape[1], "
           << inVar << ".shape[0]), flags=" << interpolations[_params[6].GetInt()] << ")";
    return stream.str();
}

ImageStatisticsOp::ImageStatisticsOp()
{
    _params.push_back(MakeBool("drawOverlay", "在图像上绘制统计摘要", true));
}

std::string ImageStatisticsOp::GetPrincipleDoc() const
{
    return "### 像素统计\n\n`minMaxLoc` 获取极值，`meanStdDev` 计算各通道均值与标准差。结构化结果可供界面或外部调用读取。";
}

bool ImageStatisticsOp::Process(const cv::Mat& input, cv::Mat& output, std::string& errorMsg)
{
    ClearResults();
    if (input.empty())
    {
        errorMsg = "输入图像为空";
        return false;
    }

    try
    {
        output = input.clone();
        std::vector<cv::Mat> channels;
        cv::split(input, channels);
        for (size_t index = 0; index < channels.size(); ++index)
        {
            double minValue = 0.0;
            double maxValue = 0.0;
            cv::minMaxLoc(channels[index], &minValue, &maxValue);
            cv::Scalar meanValue;
            cv::Scalar stdDeviation;
            cv::meanStdDev(channels[index], meanValue, stdDeviation);
            const std::string suffix = std::to_string(index);
            AddResult("channel" + suffix + ".min", "通道 " + suffix + " 最小值", minValue);
            AddResult("channel" + suffix + ".max", "通道 " + suffix + " 最大值", maxValue);
            AddResult("channel" + suffix + ".mean", "通道 " + suffix + " 均值", meanValue[0]);
            AddResult("channel" + suffix + ".stddev", "通道 " + suffix + " 标准差", stdDeviation[0]);

            if (_params[0].GetBool())
            {
                std::ostringstream line;
                line << "C" << index << " min=" << std::fixed << std::setprecision(1) << minValue
                     << " max=" << maxValue << " mean=" << meanValue[0] << " std=" << stdDeviation[0];
                cv::putText(output, line.str(), cv::Point(10, 24 + static_cast<int>(index) * 20),
                            cv::FONT_HERSHEY_SIMPLEX, 0.45, cv::Scalar::all(255), 1, cv::LINE_AA);
            }
        }
        return true;
    }
    catch (const cv::Exception& ex)
    {
        errorMsg = ex.what();
        return false;
    }
}

std::string ImageStatisticsOp::GenerateCppCode(const std::string& inVar, const std::string& outVar) const
{
    std::ostringstream stream;
    stream << outVar << " = " << inVar << ".clone();\n"
           << "std::vector<cv::Mat> statisticsChannels; cv::split(" << inVar << ", statisticsChannels);\n"
           << "for (size_t channelIndex = 0; channelIndex < statisticsChannels.size(); ++channelIndex)\n"
           << "{\n"
           << "    double minValue, maxValue; cv::minMaxLoc(statisticsChannels[channelIndex], &minValue, &maxValue);\n"
           << "    cv::Scalar meanValue, stdValue; cv::meanStdDev(statisticsChannels[channelIndex], meanValue, stdValue);\n";
    if (_params[0].GetBool())
    {
        stream << "    std::ostringstream statisticsText; statisticsText << \"C\" << channelIndex << \" min=\" << minValue << \" max=\" << maxValue << \" mean=\" << meanValue[0] << \" std=\" << stdValue[0];\n"
               << "    cv::putText(" << outVar << ", statisticsText.str(), cv::Point(10, 24 + (int)channelIndex * 20), cv::FONT_HERSHEY_SIMPLEX, 0.45, cv::Scalar::all(255), 1, cv::LINE_AA);\n";
    }
    stream << "}";
    return stream.str();
}

std::string ImageStatisticsOp::GeneratePythonCode(const std::string& inVar, const std::string& outVar) const
{
    std::ostringstream stream;
    stream << outVar << " = " << inVar << ".copy()\n"
           << "statistics_channels = cv2.split(" << inVar << ") if len(" << inVar << ".shape) == 3 else [" << inVar << "]\n"
           << "for channel_index, statistics_channel in enumerate(statistics_channels):\n"
           << "    min_value, max_value, _, _ = cv2.minMaxLoc(statistics_channel)\n"
           << "    mean_value, std_value = cv2.meanStdDev(statistics_channel)\n";
    if (_params[0].GetBool())
    {
        stream << "    text = f'C{channel_index} min={min_value:.1f} max={max_value:.1f} mean={mean_value[0,0]:.1f} std={std_value[0,0]:.1f}'\n"
               << "    cv2.putText(" << outVar << ", text, (10, 24 + channel_index * 20), cv2.FONT_HERSHEY_SIMPLEX, 0.45, 255, 1, cv2.LINE_AA)";
    }
    else
    {
        stream << "    pass";
    }
    return stream.str();
}

HistogramCompareOp::HistogramCompareOp()
{
    _params.push_back(MakeCombo("method", "比较方法", 0,
        { "相关性 CORREL", "卡方 CHISQR", "交集 INTERSECT", "巴氏距离 BHATTACHARYYA" }));
    _params.push_back(MakeInt("bins", "直方图分箱数", 64, 8, 256, 8));
    _params.push_back(MakeBool("drawOverlay", "在图像上绘制比较得分", true));
}

std::string HistogramCompareOp::GetPrincipleDoc() const
{
    return "### 直方图比较\n\n将当前图和原始图的归一化灰度直方图进行比较。相关性和交集越大越相似，卡方与巴氏距离越小越相似。";
}

bool HistogramCompareOp::Process(const cv::Mat& input, cv::Mat& output, std::string& errorMsg)
{
    ClearResults();
    if (input.empty())
    {
        errorMsg = "输入图像为空";
        return false;
    }
    cv::Mat reference = PipelineContext::Instance().GetSourceImage();
    if (reference.empty())
    {
        errorMsg = "原始参考图像为空";
        return false;
    }

    try
    {
        cv::Mat grayInput = ToGray(input);
        cv::Mat grayReference = ToGray(reference);
        const int bins = _params[1].GetInt();
        float range[] = { 0.0f, 256.0f };
        const float* ranges[] = { range };
        cv::Mat inputHistogram;
        cv::Mat referenceHistogram;
        cv::calcHist(&grayInput, 1, nullptr, cv::Mat(), inputHistogram, 1, &bins, ranges, true, false);
        cv::calcHist(&grayReference, 1, nullptr, cv::Mat(), referenceHistogram, 1, &bins, ranges, true, false);
        cv::normalize(inputHistogram, inputHistogram, 1.0, 0.0, cv::NORM_L1);
        cv::normalize(referenceHistogram, referenceHistogram, 1.0, 0.0, cv::NORM_L1);
        const int methods[] = { cv::HISTCMP_CORREL, cv::HISTCMP_CHISQR, cv::HISTCMP_INTERSECT, cv::HISTCMP_BHATTACHARYYA };
        const double score = cv::compareHist(inputHistogram, referenceHistogram, methods[_params[0].GetInt()]);
        AddResult("histogram.score", "直方图比较得分", score);

        output = ToBgr(input).clone();
        if (_params[2].GetBool())
        {
            std::ostringstream line;
            line << "Histogram score: " << std::fixed << std::setprecision(6) << score;
            cv::putText(output, line.str(), cv::Point(10, 24), cv::FONT_HERSHEY_SIMPLEX, 0.55,
                        cv::Scalar(0, 255, 255), 1, cv::LINE_AA);
        }
        return true;
    }
    catch (const cv::Exception& ex)
    {
        errorMsg = ex.what();
        return false;
    }
}

std::string HistogramCompareOp::GenerateCppCode(const std::string& inVar, const std::string& outVar) const
{
    const char* methods[] = { "cv::HISTCMP_CORREL", "cv::HISTCMP_CHISQR", "cv::HISTCMP_INTERSECT", "cv::HISTCMP_BHATTACHARYYA" };
    std::ostringstream stream;
    stream << "cv::Mat compareGray, compareReferenceGray;\n"
           << "if (" << inVar << ".channels() == 1) compareGray = " << inVar << "; else cv::cvtColor(" << inVar << ", compareGray, cv::COLOR_BGR2GRAY);\n"
           << "if (source_image.channels() == 1) compareReferenceGray = source_image; else cv::cvtColor(source_image, compareReferenceGray, cv::COLOR_BGR2GRAY);\n"
           << "int histogramSize = " << _params[1].GetInt() << "; float histogramRange[] = {0, 256}; const float* histogramRanges[] = {histogramRange};\n"
           << "cv::Mat currentHistogram, referenceHistogram;\n"
           << "cv::calcHist(&compareGray, 1, nullptr, cv::Mat(), currentHistogram, 1, &histogramSize, histogramRanges);\n"
           << "cv::calcHist(&compareReferenceGray, 1, nullptr, cv::Mat(), referenceHistogram, 1, &histogramSize, histogramRanges);\n"
           << "cv::normalize(currentHistogram, currentHistogram, 1.0, 0.0, cv::NORM_L1);\n"
           << "cv::normalize(referenceHistogram, referenceHistogram, 1.0, 0.0, cv::NORM_L1);\n"
           << "double histogramScore = cv::compareHist(currentHistogram, referenceHistogram, " << methods[_params[0].GetInt()] << ");\n"
           << "if (" << inVar << ".channels() == 1) cv::cvtColor(" << inVar << ", " << outVar << ", cv::COLOR_GRAY2BGR); else " << outVar << " = " << inVar << ".clone();\n";
    if (_params[2].GetBool())
    {
        stream << "cv::putText(" << outVar << ", \"Histogram score: \" + std::to_string(histogramScore), cv::Point(10, 24), cv::FONT_HERSHEY_SIMPLEX, 0.55, cv::Scalar(0,255,255), 1, cv::LINE_AA);";
    }
    return stream.str();
}

std::string HistogramCompareOp::GeneratePythonCode(const std::string& inVar, const std::string& outVar) const
{
    const char* methods[] = { "cv2.HISTCMP_CORREL", "cv2.HISTCMP_CHISQR", "cv2.HISTCMP_INTERSECT", "cv2.HISTCMP_BHATTACHARYYA" };
    std::ostringstream stream;
    stream << "compare_gray = " << inVar << " if len(" << inVar << ".shape) == 2 else cv2.cvtColor(" << inVar << ", cv2.COLOR_BGR2GRAY)\n"
           << "compare_reference_gray = source_image if len(source_image.shape) == 2 else cv2.cvtColor(source_image, cv2.COLOR_BGR2GRAY)\n"
           << "current_histogram = cv2.calcHist([compare_gray], [0], None, [" << _params[1].GetInt() << "], [0, 256])\n"
           << "reference_histogram = cv2.calcHist([compare_reference_gray], [0], None, [" << _params[1].GetInt() << "], [0, 256])\n"
           << "current_histogram = cv2.normalize(current_histogram, None, alpha=1, norm_type=cv2.NORM_L1)\n"
           << "reference_histogram = cv2.normalize(reference_histogram, None, alpha=1, norm_type=cv2.NORM_L1)\n"
           << "histogram_score = cv2.compareHist(current_histogram, reference_histogram, " << methods[_params[0].GetInt()] << ")\n"
           << outVar << " = cv2.cvtColor(" << inVar << ", cv2.COLOR_GRAY2BGR) if len(" << inVar << ".shape) == 2 else " << inVar << ".copy()\n";
    if (_params[2].GetBool())
    {
        stream << "cv2.putText(" << outVar << ", f'Histogram score: {histogram_score:.6f}', (10, 24), cv2.FONT_HERSHEY_SIMPLEX, 0.55, (0,255,255), 1, cv2.LINE_AA)";
    }
    return stream.str();
}

HistogramMatchOp::HistogramMatchOp()
{
    _params.push_back(MakeBool("perChannel", "彩色图像逐通道匹配", true));
}

std::string HistogramMatchOp::GetPrincipleDoc() const
{
    return "### 直方图规定化\n\n分别计算输入图与原始参考图的累积分布函数 CDF，并建立最接近 CDF 值之间的灰度查找表。";
}

bool HistogramMatchOp::Process(const cv::Mat& input, cv::Mat& output, std::string& errorMsg)
{
    ClearResults();
    if (input.empty() || input.depth() != CV_8U)
    {
        errorMsg = "直方图匹配需要非空的 8 位图像";
        return false;
    }
    cv::Mat reference = PipelineContext::Instance().GetSourceImage();
    if (reference.empty() || reference.depth() != CV_8U)
    {
        errorMsg = "原始参考图像为空或不是 8 位图像";
        return false;
    }

    try
    {
        if (input.channels() == 1 || !_params[0].GetBool())
        {
            cv::Mat grayInput = ToGray(input);
            cv::Mat grayReference = ToGray(reference);
            output = MatchHistogramChannel(grayInput, grayReference);
        }
        else
        {
            cv::Mat inputBgr = ToBgr(input);
            cv::Mat referenceBgr = ToBgr(reference);
            std::vector<cv::Mat> inputChannels;
            std::vector<cv::Mat> referenceChannels;
            cv::split(inputBgr, inputChannels);
            cv::split(referenceBgr, referenceChannels);
            for (size_t index = 0; index < inputChannels.size(); ++index)
            {
                inputChannels[index] = MatchHistogramChannel(inputChannels[index], referenceChannels[index]);
            }
            cv::merge(inputChannels, output);
        }
        return true;
    }
    catch (const cv::Exception& ex)
    {
        errorMsg = ex.what();
        return false;
    }
}

std::string HistogramMatchOp::GenerateCppCode(const std::string& inVar, const std::string& outVar) const
{
    std::ostringstream stream;
    stream << "auto matchHistogramChannel = [](const cv::Mat& currentChannel, const cv::Mat& referenceChannel)\n"
           << "{\n"
           << "    int histSize = 256; float range[] = {0, 256}; const float* ranges[] = {range}; cv::Mat currentHist, referenceHist;\n"
           << "    cv::calcHist(&currentChannel, 1, nullptr, cv::Mat(), currentHist, 1, &histSize, ranges); cv::calcHist(&referenceChannel, 1, nullptr, cv::Mat(), referenceHist, 1, &histSize, ranges);\n"
           << "    currentHist /= (float)currentChannel.total(); referenceHist /= (float)referenceChannel.total();\n"
           << "    for (int i = 1; i < 256; ++i) { currentHist.at<float>(i) += currentHist.at<float>(i - 1); referenceHist.at<float>(i) += referenceHist.at<float>(i - 1); }\n"
           << "    cv::Mat lookup(1, 256, CV_8U); int j = 0; for (int i = 0; i < 256; ++i) { while (j < 255 && referenceHist.at<float>(j) < currentHist.at<float>(i)) ++j; lookup.at<uchar>(i) = (uchar)j; }\n"
           << "    cv::Mat matched; cv::LUT(currentChannel, lookup, matched); return matched;\n"
           << "};\n";
    if (_params[0].GetBool())
    {
        stream << "cv::Mat matchInput = " << inVar << ", matchReference = source_image;\n"
               << "if (matchInput.channels() == 1) cv::cvtColor(matchInput, matchInput, cv::COLOR_GRAY2BGR);\n"
               << "if (matchReference.channels() == 1) cv::cvtColor(matchReference, matchReference, cv::COLOR_GRAY2BGR);\n"
               << "std::vector<cv::Mat> matchChannels, referenceChannels; cv::split(matchInput, matchChannels); cv::split(matchReference, referenceChannels);\n"
               << "for (size_t i = 0; i < matchChannels.size(); ++i) matchChannels[i] = matchHistogramChannel(matchChannels[i], referenceChannels[i]);\n"
               << "cv::merge(matchChannels, " << outVar << ");";
    }
    else
    {
        stream << "cv::Mat matchGray, referenceGray; if (" << inVar << ".channels() == 1) matchGray = " << inVar << "; else cv::cvtColor(" << inVar << ", matchGray, cv::COLOR_BGR2GRAY);\n"
               << "if (source_image.channels() == 1) referenceGray = source_image; else cv::cvtColor(source_image, referenceGray, cv::COLOR_BGR2GRAY);\n"
               << outVar << " = matchHistogramChannel(matchGray, referenceGray);";
    }
    return stream.str();
}

std::string HistogramMatchOp::GeneratePythonCode(const std::string& inVar, const std::string& outVar) const
{
    std::ostringstream stream;
    stream << "def match_histogram_channel(current_channel, reference_channel):\n"
           << "    current_hist, _ = np.histogram(current_channel.ravel(), 256, [0, 256], density=True)\n"
           << "    reference_hist, _ = np.histogram(reference_channel.ravel(), 256, [0, 256], density=True)\n"
           << "    current_cdf = np.cumsum(current_hist)\n"
           << "    reference_cdf = np.cumsum(reference_hist)\n"
           << "    lookup = np.searchsorted(reference_cdf, current_cdf).clip(0, 255).astype(np.uint8)\n"
           << "    return cv2.LUT(current_channel, lookup)\n";
    if (_params[0].GetBool())
    {
        stream << "match_input = cv2.cvtColor(" << inVar << ", cv2.COLOR_GRAY2BGR) if len(" << inVar << ".shape) == 2 else " << inVar << "[:, :, :3]\n"
               << "match_reference = cv2.cvtColor(source_image, cv2.COLOR_GRAY2BGR) if len(source_image.shape) == 2 else source_image[:, :, :3]\n"
               << "matched_channels = [match_histogram_channel(a, b) for a, b in zip(cv2.split(match_input), cv2.split(match_reference))]\n"
               << outVar << " = cv2.merge(matched_channels)";
    }
    else
    {
        stream << "match_gray = " << inVar << " if len(" << inVar << ".shape) == 2 else cv2.cvtColor(" << inVar << ", cv2.COLOR_BGR2GRAY)\n"
               << "reference_gray = source_image if len(source_image.shape) == 2 else cv2.cvtColor(source_image, cv2.COLOR_BGR2GRAY)\n"
               << outVar << " = match_histogram_channel(match_gray, reference_gray)";
    }
    return stream.str();
}

MomentsHuOp::MomentsHuOp()
{
    _params.push_back(MakeInt("threshold", "二值化阈值 (0=Otsu)", 0, 0, 255));
    _params.push_back(MakeBool("invert", "反相二值化", false));
    _params.push_back(MakeBool("drawOverlay", "绘制最大轮廓与质心", true));
}

std::string MomentsHuOp::GetPrincipleDoc() const
{
    return "### 图像矩与 Hu 矩\n\n空间矩描述区域质量分布，`m10/m00` 和 `m01/m00` 给出质心。七个 Hu 组合矩对平移、尺度和旋转具有不变性。";
}

bool MomentsHuOp::Process(const cv::Mat& input, cv::Mat& output, std::string& errorMsg)
{
    ClearResults();
    if (input.empty())
    {
        errorMsg = "输入图像为空";
        return false;
    }

    try
    {
        cv::Mat gray = ToGray(input);
        cv::Mat binary;
        const int thresholdType = (_params[1].GetBool() ? cv::THRESH_BINARY_INV : cv::THRESH_BINARY) |
                                  (_params[0].GetInt() == 0 ? cv::THRESH_OTSU : 0);
        cv::threshold(gray, binary, _params[0].GetInt(), 255, thresholdType);
        std::vector<std::vector<cv::Point>> contours;
        cv::findContours(binary, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);
        if (contours.empty())
        {
            errorMsg = "未找到可计算矩的轮廓";
            return false;
        }

        const auto largest = std::max_element(contours.begin(), contours.end(), [](const auto& left, const auto& right) {
            return cv::contourArea(left) < cv::contourArea(right);
        });
        const cv::Moments moments = cv::moments(*largest);
        if (std::abs(moments.m00) < 1e-12)
        {
            errorMsg = "最大轮廓面积为零";
            return false;
        }
        const double centerX = moments.m10 / moments.m00;
        const double centerY = moments.m01 / moments.m00;
        double hu[7] = {};
        cv::HuMoments(moments, hu);
        AddResult("moments.area", "轮廓面积", moments.m00, "px^2");
        AddResult("moments.centerX", "质心 X", centerX, "px");
        AddResult("moments.centerY", "质心 Y", centerY, "px");
        for (int index = 0; index < 7; ++index)
        {
            AddResult("hu." + std::to_string(index + 1), "Hu 矩 " + std::to_string(index + 1), hu[index]);
        }

        output = ToBgr(input).clone();
        if (_params[2].GetBool())
        {
            cv::drawContours(output, contours, static_cast<int>(std::distance(contours.begin(), largest)), cv::Scalar(0, 255, 0), 2);
            cv::drawMarker(output, cv::Point2d(centerX, centerY), cv::Scalar(0, 0, 255), cv::MARKER_CROSS, 16, 2);
            std::ostringstream line;
            line << "Area=" << std::fixed << std::setprecision(1) << moments.m00 << " Center=(" << centerX << "," << centerY << ")";
            cv::putText(output, line.str(), cv::Point(10, 24), cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0, 255, 255), 1, cv::LINE_AA);
        }
        return true;
    }
    catch (const cv::Exception& ex)
    {
        errorMsg = ex.what();
        return false;
    }
}

std::string MomentsHuOp::GenerateCppCode(const std::string& inVar, const std::string& outVar) const
{
    std::ostringstream stream;
    stream << "cv::Mat momentsGray, momentsBinary;\n"
           << "if (" << inVar << ".channels() == 1) momentsGray = " << inVar << "; else cv::cvtColor(" << inVar << ", momentsGray, cv::COLOR_BGR2GRAY);\n"
           << "cv::threshold(momentsGray, momentsBinary, " << _params[0].GetInt() << ", 255, "
           << (_params[1].GetBool() ? "cv::THRESH_BINARY_INV" : "cv::THRESH_BINARY")
           << (_params[0].GetInt() == 0 ? " | cv::THRESH_OTSU" : "") << ");\n"
           << "std::vector<std::vector<cv::Point>> momentContours; cv::findContours(momentsBinary, momentContours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);\n"
           << "auto largestMomentContour = std::max_element(momentContours.begin(), momentContours.end(), [](const auto& a, const auto& b) { return cv::contourArea(a) < cv::contourArea(b); });\n"
           << "cv::Moments imageMoments = cv::moments(*largestMomentContour); double huMoments[7]; cv::HuMoments(imageMoments, huMoments);\n"
           << "cv::Point2d momentCenter(imageMoments.m10 / imageMoments.m00, imageMoments.m01 / imageMoments.m00);\n"
           << "if (" << inVar << ".channels() == 1) cv::cvtColor(" << inVar << ", " << outVar << ", cv::COLOR_GRAY2BGR); else " << outVar << " = " << inVar << ".clone();\n";
    if (_params[2].GetBool())
    {
        stream << "cv::drawContours(" << outVar << ", momentContours, (int)std::distance(momentContours.begin(), largestMomentContour), cv::Scalar(0,255,0), 2);\n"
               << "cv::drawMarker(" << outVar << ", momentCenter, cv::Scalar(0,0,255), cv::MARKER_CROSS, 16, 2);";
    }
    return stream.str();
}

std::string MomentsHuOp::GeneratePythonCode(const std::string& inVar, const std::string& outVar) const
{
    std::ostringstream stream;
    stream << "moments_gray = " << inVar << " if len(" << inVar << ".shape) == 2 else cv2.cvtColor(" << inVar << ", cv2.COLOR_BGR2GRAY)\n"
           << "_, moments_binary = cv2.threshold(moments_gray, " << _params[0].GetInt() << ", 255, "
           << (_params[1].GetBool() ? "cv2.THRESH_BINARY_INV" : "cv2.THRESH_BINARY")
           << (_params[0].GetInt() == 0 ? " | cv2.THRESH_OTSU" : "") << ")\n"
           << "moment_contours, _ = cv2.findContours(moments_binary, cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_SIMPLE)\n"
           << "largest_moment_contour = max(moment_contours, key=cv2.contourArea)\n"
           << "image_moments = cv2.moments(largest_moment_contour)\n"
           << "hu_moments = cv2.HuMoments(image_moments).flatten()\n"
           << "moment_center = (int(image_moments['m10'] / image_moments['m00']), int(image_moments['m01'] / image_moments['m00']))\n"
           << outVar << " = cv2.cvtColor(" << inVar << ", cv2.COLOR_GRAY2BGR) if len(" << inVar << ".shape) == 2 else " << inVar << ".copy()\n";
    if (_params[2].GetBool())
    {
        stream << "cv2.drawContours(" << outVar << ", [largest_moment_contour], -1, (0,255,0), 2)\n"
               << "cv2.drawMarker(" << outVar << ", moment_center, (0,0,255), cv2.MARKER_CROSS, 16, 2)";
    }
    return stream.str();
}
