#include "ColorOps.h"
#include <sstream>

CvtColorOp::CvtColorOp()
{
    OpParameter codeParam;
    codeParam.Name = "code";
    codeParam.DisplayName = "转换模式 (Code)";
    codeParam.Tooltip = "选择目标色彩空间类型";
    codeParam.Type = ParamType::ComboBox;
    codeParam.Value = 0; // 默认 BGR2GRAY
    codeParam.EnumOptions = {
        "BGR -> GRAY (灰度化)",
        "GRAY -> BGR (灰度转伪BGR)",
        "BGR -> HSV (色调/饱和度/明度)",
        "HSV -> BGR (HSV还原BGR)",
        "BGR -> RGB (RGB反序)",
        "BGR -> Lab (CIELAB色彩空间)",
        "BGR -> YCrCb (亮度/色度)"
    };
    _params.push_back(codeParam);
}

std::string CvtColorOp::GetPrincipleDoc() const
{
    return "### 📘 算子原理：色彩空间转换 (cvtColor)\n\n"
           "**色彩空间** 是用数值表示颜色的数学模型：\n"
           "* **RGB / BGR**：加色模型，常用于显示器与通用图像采集。\n"
           "* **GRAY**：单通道灰度，公式 $Gray = 0.299R + 0.587G + 0.114B$，减少数据量并加速运算。\n"
           "* **HSV**：H(色调 0~180), S(饱和度 0~255), V(明度 0~255)，极适合用于基于颜色的分割与识别。\n"
           "* **Lab**：感知均匀色彩空间，L 表示明度，a/b 表示色彩分量，适合颜色差异度量。";
}

bool CvtColorOp::Process(const cv::Mat& input, cv::Mat& output, std::string& errorMsg)
{
    if (input.empty())
    {
        errorMsg = "输入图像为空";
        return false;
    }

    int mode = _params[0].GetInt();
    try
    {
        switch (mode)
        {
            case 0: // BGR2GRAY
                if (input.channels() == 1) { output = input.clone(); }
                else if (input.channels() == 3) { cv::cvtColor(input, output, cv::COLOR_BGR2GRAY); }
                else if (input.channels() == 4) { cv::cvtColor(input, output, cv::COLOR_BGRA2GRAY); }
                break;
            case 1: // GRAY2BGR
                if (input.channels() == 1) { cv::cvtColor(input, output, cv::COLOR_GRAY2BGR); }
                else { output = input.clone(); }
                break;
            case 2: // BGR2HSV
                if (input.channels() == 3) { cv::cvtColor(input, output, cv::COLOR_BGR2HSV); }
                else { errorMsg = "BGR -> HSV 要求输入必须为 3 通道图像"; return false; }
                break;
            case 3: // HSV2BGR
                if (input.channels() == 3) { cv::cvtColor(input, output, cv::COLOR_HSV2BGR); }
                else { errorMsg = "HSV -> BGR 要求输入必须为 3 通道图像"; return false; }
                break;
            case 4: // BGR2RGB
                if (input.channels() == 3) { cv::cvtColor(input, output, cv::COLOR_BGR2RGB); }
                else { output = input.clone(); }
                break;
            case 5: // BGR2Lab
                if (input.channels() == 3) { cv::cvtColor(input, output, cv::COLOR_BGR2Lab); }
                else { errorMsg = "BGR -> Lab 要求输入必须为 3 通道图像"; return false; }
                break;
            case 6: // BGR2YCrCb
                if (input.channels() == 3) { cv::cvtColor(input, output, cv::COLOR_BGR2YCrCb); }
                else { errorMsg = "BGR -> YCrCb 要求输入必须为 3 通道图像"; return false; }
                break;
            default:
                output = input.clone();
                break;
        }
        return true;
    }
    catch (const cv::Exception& e)
    {
        errorMsg = e.what();
        return false;
    }
}

std::string CvtColorOp::GenerateCppCode(const std::string& inVar, const std::string& outVar) const
{
    int mode = _params[0].GetInt();
    std::string codeConst = "cv::COLOR_BGR2GRAY";
    if (mode == 1) codeConst = "cv::COLOR_GRAY2BGR";
    else if (mode == 2) codeConst = "cv::COLOR_BGR2HSV";
    else if (mode == 3) codeConst = "cv::COLOR_HSV2BGR";
    else if (mode == 4) codeConst = "cv::COLOR_BGR2RGB";
    else if (mode == 5) codeConst = "cv::COLOR_BGR2Lab";
    else if (mode == 6) codeConst = "cv::COLOR_BGR2YCrCb";

    std::stringstream ss;
    ss << "cv::cvtColor(" << inVar << ", " << outVar << ", " << codeConst << ");";
    return ss.str();
}

std::string CvtColorOp::GeneratePythonCode(const std::string& inVar, const std::string& outVar) const
{
    int mode = _params[0].GetInt();
    std::string codeConst = "cv2.COLOR_BGR2GRAY";
    if (mode == 1) codeConst = "cv2.COLOR_GRAY2BGR";
    else if (mode == 2) codeConst = "cv2.COLOR_BGR2HSV";
    else if (mode == 3) codeConst = "cv2.COLOR_HSV2BGR";
    else if (mode == 4) codeConst = "cv2.COLOR_BGR2RGB";
    else if (mode == 5) codeConst = "cv2.COLOR_BGR2Lab";
    else if (mode == 6) codeConst = "cv2.COLOR_BGR2YCrCb";

    std::stringstream ss;
    ss << outVar << " = cv2.cvtColor(" << inVar << ", " << codeConst << ")";
    return ss.str();
}

// ---------------- ChannelSplitOp ----------------

ChannelSplitOp::ChannelSplitOp()
{
    OpParameter chParam;
    chParam.Name = "channel";
    chParam.DisplayName = "目标通道 (Channel)";
    chParam.Tooltip = "选择要提取的单通道索引";
    chParam.Type = ParamType::ComboBox;
    chParam.Value = 0;
    chParam.EnumOptions = {
        "通道 0 (Blue / H色度)",
        "通道 1 (Green / S饱和度)",
        "通道 2 (Red / V明度)"
    };
    _params.push_back(chParam);
}

std::string ChannelSplitOp::GetPrincipleDoc() const
{
    return "### 📘 算子原理：通道分离 (split)\n\n"
           "将多通道图像（如 BGR 或 HSV）拆分为独立的单通道灰度图，便于对特定通道进行独立阈值分割或特征提取。";
}

bool ChannelSplitOp::Process(const cv::Mat& input, cv::Mat& output, std::string& errorMsg)
{
    if (input.empty())
    {
        errorMsg = "输入图像为空";
        return false;
    }

    if (input.channels() == 1)
    {
        output = input.clone();
        return true;
    }

    int chIdx = _params[0].GetInt();
    std::vector<cv::Mat> channels;
    cv::split(input, channels);

    if (chIdx < 0 || chIdx >= static_cast<int>(channels.size()))
    {
        errorMsg = "通道索引超出当前图像通道数范围";
        return false;
    }

    output = channels[chIdx];
    return true;
}

std::string ChannelSplitOp::GenerateCppCode(const std::string& inVar, const std::string& outVar) const
{
    int chIdx = _params[0].GetInt();
    std::stringstream ss;
    ss << "std::vector<cv::Mat> channels_" << outVar << ";\n"
       << "cv::split(" << inVar << ", channels_" << outVar << ");\n"
       << outVar << " = channels_" << outVar << "[" << chIdx << "];";
    return ss.str();
}

std::string ChannelSplitOp::GeneratePythonCode(const std::string& inVar, const std::string& outVar) const
{
    int chIdx = _params[0].GetInt();
    std::stringstream ss;
    ss << "channels_" << outVar << " = cv2.split(" << inVar << ")\n"
       << outVar << " = channels_" << outVar << "[" << chIdx << "]";
    return ss.str();
}
