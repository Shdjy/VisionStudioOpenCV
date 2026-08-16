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
        "BGR -> Lab (CIELAB感知均匀色彩空间)",
        "BGR -> YCrCb (亮度与色度空间)"
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
           "* **Lab**：感知均匀色彩空间，L 表示明度，a/b 表示色彩分量，适合颜色差异度量。\n"
           "* **YCrCb**：Y(亮度), Cr(红色差), Cb(蓝色差)，广泛用于视频编解码与人脸肤色检测。";
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
    std::string desc = "BGR -> GRAY (灰度化)";
    if (mode == 1) { codeConst = "cv::COLOR_GRAY2BGR"; desc = "GRAY -> BGR (灰度转伪BGR)"; }
    else if (mode == 2) { codeConst = "cv::COLOR_BGR2HSV"; desc = "BGR -> HSV (色调/饱和度/明度)"; }
    else if (mode == 3) { codeConst = "cv::COLOR_HSV2BGR"; desc = "HSV -> BGR"; }
    else if (mode == 4) { codeConst = "cv::COLOR_BGR2RGB"; desc = "BGR -> RGB (反序)"; }
    else if (mode == 5) { codeConst = "cv::COLOR_BGR2Lab"; desc = "BGR -> Lab (均匀色彩空间)"; }
    else if (mode == 6) { codeConst = "cv::COLOR_BGR2YCrCb"; desc = "BGR -> YCrCb"; }

    std::stringstream ss;
    ss << "// 色彩转换参数: " << desc << "\n"
       << "int colorCode = " << codeConst << ";\n"
       << "cv::cvtColor(" << inVar << ", " << outVar << ", colorCode);";
    return ss.str();
}

std::string CvtColorOp::GeneratePythonCode(const std::string& inVar, const std::string& outVar) const
{
    int mode = _params[0].GetInt();
    std::string codeConst = "cv2.COLOR_BGR2GRAY";
    std::string desc = "BGR -> GRAY (灰度化)";
    if (mode == 1) { codeConst = "cv2.COLOR_GRAY2BGR"; desc = "GRAY -> BGR"; }
    else if (mode == 2) { codeConst = "cv2.COLOR_BGR2HSV"; desc = "BGR -> HSV"; }
    else if (mode == 3) { codeConst = "cv2.COLOR_HSV2BGR"; desc = "HSV -> BGR"; }
    else if (mode == 4) { codeConst = "cv2.COLOR_BGR2RGB"; desc = "BGR -> RGB"; }
    else if (mode == 5) { codeConst = "cv2.COLOR_BGR2Lab"; desc = "BGR -> Lab"; }
    else if (mode == 6) { codeConst = "cv2.COLOR_BGR2YCrCb"; desc = "BGR -> YCrCb"; }

    std::stringstream ss;
    ss << "# 色彩转换参数: " << desc << "\n"
       << "color_code = " << codeConst << "\n"
       << outVar << " = cv2.cvtColor(" << inVar << ", color_code)";
    return ss.str();
}

// ---------------- ChannelSplitOp ----------------

ChannelSplitOp::ChannelSplitOp()
{
    OpParameter chParam;
    chParam.Name = "channel";
    chParam.DisplayName = "目标提取通道 (Target Channel)";
    chParam.Tooltip = "选择要从图像中提取的独立分量通道";
    chParam.Type = ParamType::ComboBox;
    chParam.Value = 0;
    chParam.EnumOptions = {
        "BGR - 蓝色通道 (Blue)",
        "BGR - 绿色通道 (Green)",
        "BGR - 红色通道 (Red)",
        "HSV - 色调通道 (Hue 0~180)",
        "HSV - 饱和度通道 (Saturation 0~255)",
        "HSV - 亮度通道 (Value 0~255)",
        "YCrCb - 亮度分量 (Y 0~255)",
        "YCrCb - 红色偏度 (Cr)",
        "YCrCb - 蓝色偏度 (Cb)",
        "Lab - 明度分量 (L 0~255)",
        "Lab - 红绿分量 (a)",
        "Lab - 黄蓝分量 (b)"
    };
    _params.push_back(chParam);

    OpParameter normParam;
    normParam.Name = "normalize";
    normParam.DisplayName = "动态范围归一化拉伸 (Normalize 0~255)";
    normParam.Tooltip = "自动将单通道最小/最大值线性拉伸至 0~255，增强视觉反差";
    normParam.Type = ParamType::CheckBox;
    normParam.Value = false;
    _params.push_back(normParam);
}

std::string ChannelSplitOp::GetPrincipleDoc() const
{
    return "### 📘 算子原理：多色彩空间通道分离提取 (Channel Split)\n\n"
           "工业机器视觉中，单个通道往往包含比全彩图更关键的对比度特征：\n"
           "* **HSV.S 通道**：极易去除金属反光；\n"
           "* **HSV.H 通道**：对光照强度变化具有天然鲁棒性；\n"
           "* **Lab.L 通道 / YCrCb.Y 通道**：无损剥离色度后的纯净照度信息；\n"
           "* **BGR 单色通道**：提取特定波长光源（如红光/蓝光打光）下的缺陷特征。";
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

    int choice = _params[0].GetInt();
    bool doNorm = _params[1].GetBool();

    cv::Mat converted;
    int chIdx = 0;

    if (choice >= 0 && choice <= 2) // BGR
    {
        converted = input;
        chIdx = choice;
    }
    else if (choice >= 3 && choice <= 5) // HSV
    {
        cv::cvtColor(input, converted, cv::COLOR_BGR2HSV);
        chIdx = choice - 3;
    }
    else if (choice >= 6 && choice <= 8) // YCrCb
    {
        cv::cvtColor(input, converted, cv::COLOR_BGR2YCrCb);
        chIdx = choice - 6;
    }
    else if (choice >= 9 && choice <= 11) // Lab
    {
        cv::cvtColor(input, converted, cv::COLOR_BGR2Lab);
        chIdx = choice - 9;
    }

    std::vector<cv::Mat> channels;
    cv::split(converted, channels);

    if (chIdx < 0 || chIdx >= static_cast<int>(channels.size()))
    {
        errorMsg = "通道索引超出范围";
        return false;
    }

    if (doNorm)
    {
        cv::normalize(channels[chIdx], output, 0, 255, cv::NORM_MINMAX);
    }
    else
    {
        output = channels[chIdx];
    }
    return true;
}

std::string ChannelSplitOp::GenerateCppCode(const std::string& inVar, const std::string& outVar) const
{
    int choice = _params[0].GetInt();
    bool doNorm = _params[1].GetBool();

    std::string convCode = "";
    int chIdx = 0;
    std::string chName = "BGR Blue 通道";

    if (choice == 0) { chIdx = 0; chName = "BGR Blue 通道"; }
    else if (choice == 1) { chIdx = 1; chName = "BGR Green 通道"; }
    else if (choice == 2) { chIdx = 2; chName = "BGR Red 通道"; }
    else if (choice == 3) { convCode = "cv::Mat hsv;\ncv::cvtColor(" + inVar + ", hsv, cv::COLOR_BGR2HSV);\n"; chIdx = 0; chName = "HSV H(色调) 通道"; }
    else if (choice == 4) { convCode = "cv::Mat hsv;\ncv::cvtColor(" + inVar + ", hsv, cv::COLOR_BGR2HSV);\n"; chIdx = 1; chName = "HSV S(饱和度) 通道"; }
    else if (choice == 5) { convCode = "cv::Mat hsv;\ncv::cvtColor(" + inVar + ", hsv, cv::COLOR_BGR2HSV);\n"; chIdx = 2; chName = "HSV V(明度) 通道"; }
    else if (choice == 6) { convCode = "cv::Mat ycrcb;\ncv::cvtColor(" + inVar + ", ycrcb, cv::COLOR_BGR2YCrCb);\n"; chIdx = 0; chName = "YCrCb Y(亮度) 通道"; }
    else if (choice == 7) { convCode = "cv::Mat ycrcb;\ncv::cvtColor(" + inVar + ", ycrcb, cv::COLOR_BGR2YCrCb);\n"; chIdx = 1; chName = "YCrCb Cr 通道"; }
    else if (choice == 8) { convCode = "cv::Mat ycrcb;\ncv::cvtColor(" + inVar + ", ycrcb, cv::COLOR_BGR2YCrCb);\n"; chIdx = 2; chName = "YCrCb Cb 通道"; }
    else if (choice == 9) { convCode = "cv::Mat lab;\ncv::cvtColor(" + inVar + ", lab, cv::COLOR_BGR2Lab);\n"; chIdx = 0; chName = "Lab L(明度) 通道"; }
    else if (choice == 10) { convCode = "cv::Mat lab;\ncv::cvtColor(" + inVar + ", lab, cv::COLOR_BGR2Lab);\n"; chIdx = 1; chName = "Lab a 通道"; }
    else if (choice == 11) { convCode = "cv::Mat lab;\ncv::cvtColor(" + inVar + ", lab, cv::COLOR_BGR2Lab);\n"; chIdx = 2; chName = "Lab b 通道"; }

    std::string srcObj = convCode.empty() ? inVar : (choice <= 5 ? "hsv" : (choice <= 8 ? "ycrcb" : "lab"));

    std::stringstream ss;
    ss << "// 通道分离提取: " << chName << "\n"
       << convCode
       << "std::vector<cv::Mat> channels_" << outVar << ";\n"
       << "cv::split(" << srcObj << ", channels_" << outVar << ");\n"
       << "int targetChannel = " << chIdx << ";\n";
    if (doNorm)
    {
        ss << "cv::normalize(channels_" << outVar << "[targetChannel], " << outVar << ", 0, 255, cv::NORM_MINMAX);";
    }
    else
    {
        ss << outVar << " = channels_" << outVar << "[targetChannel];";
    }
    return ss.str();
}

std::string ChannelSplitOp::GeneratePythonCode(const std::string& inVar, const std::string& outVar) const
{
    int choice = _params[0].GetInt();
    bool doNorm = _params[1].GetBool();

    std::string convCode = "";
    int chIdx = 0;
    std::string chName = "BGR Blue 通道";

    if (choice == 0) { chIdx = 0; chName = "BGR Blue 通道"; }
    else if (choice == 1) { chIdx = 1; chName = "BGR Green 通道"; }
    else if (choice == 2) { chIdx = 2; chName = "BGR Red 通道"; }
    else if (choice == 3) { convCode = "converted = cv2.cvtColor(" + inVar + ", cv2.COLOR_BGR2HSV)\n"; chIdx = 0; chName = "HSV H(色调) 通道"; }
    else if (choice == 4) { convCode = "converted = cv2.cvtColor(" + inVar + ", cv2.COLOR_BGR2HSV)\n"; chIdx = 1; chName = "HSV S(饱和度) 通道"; }
    else if (choice == 5) { convCode = "converted = cv2.cvtColor(" + inVar + ", cv2.COLOR_BGR2HSV)\n"; chIdx = 2; chName = "HSV V(明度) 通道"; }
    else if (choice == 6) { convCode = "converted = cv2.cvtColor(" + inVar + ", cv2.COLOR_BGR2YCrCb)\n"; chIdx = 0; chName = "YCrCb Y(亮度) 通道"; }
    else if (choice == 7) { convCode = "converted = cv2.cvtColor(" + inVar + ", cv2.COLOR_BGR2YCrCb)\n"; chIdx = 1; chName = "YCrCb Cr 通道"; }
    else if (choice == 8) { convCode = "converted = cv2.cvtColor(" + inVar + ", cv2.COLOR_BGR2YCrCb)\n"; chIdx = 2; chName = "YCrCb Cb 通道"; }
    else if (choice == 9) { convCode = "converted = cv2.cvtColor(" + inVar + ", cv2.COLOR_BGR2Lab)\n"; chIdx = 0; chName = "Lab L(明度) 通道"; }
    else if (choice == 10) { convCode = "converted = cv2.cvtColor(" + inVar + ", cv2.COLOR_BGR2Lab)\n"; chIdx = 1; chName = "Lab a 通道"; }
    else if (choice == 11) { convCode = "converted = cv2.cvtColor(" + inVar + ", cv2.COLOR_BGR2Lab)\n"; chIdx = 2; chName = "Lab b 通道"; }

    std::string srcObj = convCode.empty() ? inVar : "converted";

    std::stringstream ss;
    ss << "# 通道分离提取: " << chName << "\n"
       << convCode
       << "channels_" << outVar << " = cv2.split(" << srcObj << ")\n"
       << "target_ch = " << chIdx << "\n";
    if (doNorm)
    {
        ss << outVar << " = cv2.normalize(channels_" << outVar << "[target_ch], None, 0, 255, cv2.NORM_MINMAX)";
    }
    else
    {
        ss << outVar << " = channels_" << outVar << "[target_ch]";
    }
    return ss.str();
}
