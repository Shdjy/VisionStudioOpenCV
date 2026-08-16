#include "EnhanceOps.h"
#include <sstream>

// ---------------- BrightnessContrastOp ----------------
BrightnessContrastOp::BrightnessContrastOp()
{
    OpParameter alphaParam;
    alphaParam.Name = "alpha";
    alphaParam.DisplayName = "对比度倍数 (Contrast Alpha)";
    alphaParam.Tooltip = "图像对比度放大倍数 (<1降低对比度, >1增强反差)";
    alphaParam.Type = ParamType::DoubleSlider;
    alphaParam.Value = 1.2;
    alphaParam.MinValue = 0.1;
    alphaParam.MaxValue = 5.0;
    alphaParam.Step = 0.05;
    _params.push_back(alphaParam);

    OpParameter betaParam;
    betaParam.Name = "beta";
    betaParam.DisplayName = "亮度偏移 (Brightness Beta ±)";
    betaParam.Tooltip = "全局像素亮度增减数值 (-255~+255)";
    betaParam.Type = ParamType::IntSlider;
    betaParam.Value = 15;
    betaParam.MinValue = -255;
    betaParam.MaxValue = 255;
    betaParam.Step = 1;
    _params.push_back(betaParam);

    OpParameter gammaParam;
    gammaParam.Name = "gamma";
    gammaParam.DisplayName = "非线性伽马微调 (Gamma γ)";
    gammaParam.Tooltip = "非线性曲线微调 (<1提亮阴影暗部, >1压暗)";
    gammaParam.Type = ParamType::DoubleSlider;
    gammaParam.Value = 1.0;
    gammaParam.MinValue = 0.2;
    gammaParam.MaxValue = 3.0;
    gammaParam.Step = 0.05;
    _params.push_back(gammaParam);
}

std::string BrightnessContrastOp::GetPrincipleDoc() const
{
    return "### 📘 算子原理：亮度和对比度线性/非线性复合调节\n\n"
           "复合点变换模型：\n"
           "$$dst(x, y) = \\text{saturate\\_cast}\\langle uchar \\rangle \\left( \\alpha \\cdot src(x, y) + \\beta \\right)$$\n"
           "* **$\\alpha$ 对比度倍数**：扩大或压缩灰度级动态跨度；\n"
           "* **$\\beta$ 亮度加减**：全局向上提亮或向下压暗；\n"
           "* **$\\gamma$ 伽马系数**：针对暗部/高光提供非线性幂律补偿。";
}

bool BrightnessContrastOp::Process(const cv::Mat& input, cv::Mat& output, std::string& errorMsg)
{
    if (input.empty()) { errorMsg = "输入图像为空"; return false; }
    double alpha = _params[0].GetDouble();
    int beta = _params[1].GetInt();
    double gamma = _params[2].GetDouble();

    try
    {
        cv::Mat linOut;
        input.convertTo(linOut, -1, alpha, beta);

        if (std::abs(gamma - 1.0) > 1e-3)
        {
            unsigned char lut[256];
            for (int i = 0; i < 256; ++i)
            {
                lut[i] = cv::saturate_cast<uchar>(std::pow(i / 255.0, gamma) * 255.0);
            }
            cv::Mat lutTable(1, 256, CV_8U, lut);
            cv::LUT(linOut, lutTable, output);
        }
        else
        {
            output = linOut;
        }
        return true;
    }
    catch (const cv::Exception& e)
    {
        errorMsg = e.what();
        return false;
    }
}

std::string BrightnessContrastOp::GenerateCppCode(const std::string& inVar, const std::string& outVar) const
{
    double alpha = _params[0].GetDouble();
    int beta = _params[1].GetInt();
    double gamma = _params[2].GetDouble();

    std::stringstream ss;
    ss << "// 亮度对比度与伽马增强参数配置\n"
       << "double alpha = " << alpha << ";            // 对比度增益倍数 (Alpha)\n"
       << "double beta = " << beta << ";              // 亮度加减偏置 (Beta)\n"
       << inVar << ".convertTo(" << outVar << ", -1, alpha, beta);\n";
    if (std::abs(gamma - 1.0) > 1e-3)
    {
        ss << "double gammaValue = " << gamma << ";        // 伽马系数\n"
           << "unsigned char lut[256];\n"
           << "for (int i = 0; i < 256; ++i) lut[i] = cv::saturate_cast<uchar>(std::pow(i / 255.0, gammaValue) * 255.0);\n"
           << "cv::Mat lutTable(1, 256, CV_8U, lut);\n"
           << "cv::LUT(" << outVar << ", lutTable, " << outVar << ");";
    }
    return ss.str();
}

std::string BrightnessContrastOp::GeneratePythonCode(const std::string& inVar, const std::string& outVar) const
{
    double alpha = _params[0].GetDouble();
    int beta = _params[1].GetInt();
    double gamma = _params[2].GetDouble();

    std::stringstream ss;
    ss << "# 亮度对比度与伽马增强参数配置\n"
       << "alpha = " << alpha << "                   # 对比度增益倍数\n"
       << "beta = " << beta << "                     # 亮度加减偏置\n"
       << outVar << " = cv2.convertScaleAbs(" << inVar << ", alpha=alpha, beta=beta)\n";
    if (std::abs(gamma - 1.0) > 1e-3)
    {
        ss << "gamma_val = " << gamma << "\n"
           << "table = np.array([((i / 255.0) ** gamma_val) * 255 for i in range(256)], dtype=np.uint8)\n"
           << outVar << " = cv2.LUT(" << outVar << ", table)";
    }
    return ss.str();
}

// ---------------- EqualizeHistOp ----------------
EqualizeHistOp::EqualizeHistOp()
{
    OpParameter modeParam;
    modeParam.Name = "mode";
    modeParam.DisplayName = "均衡化色彩策略 (Method)";
    modeParam.Type = ParamType::ComboBox;
    modeParam.Value = 0;
    modeParam.EnumOptions = {
        "灰度图像直方图均衡化 (Gray)",
        "HSV 空间明度(V)通道均衡化 (保持色相)",
        "YCrCb 空间亮度(Y)通道均衡化 (工业推荐)",
        "BGR 各颜色通道独立均衡化"
    };
    _params.push_back(modeParam);

    OpParameter blendParam;
    blendParam.Name = "blend";
    blendParam.DisplayName = "混合强度比例 (Blend Ratio %)";
    blendParam.Type = ParamType::IntSlider;
    blendParam.Value = 100;
    blendParam.MinValue = 10;
    blendParam.MaxValue = 100;
    blendParam.Step = 5;
    _params.push_back(blendParam);
}

std::string EqualizeHistOp::GetPrincipleDoc() const
{
    return "### 📘 算子原理：直方图均衡化 (equalizeHist)\n\n"
           "通过累积分布函数 (CDF) 对灰度直方图进行积分变换，非线性拉伸图像动态范围。\n"
           "* **HSV-V / YCrCb-Y 均衡化**：仅增强亮度分量，彻底避免色彩失真与色偏；\n"
           "* **独立通道均衡化**：适用于严重偏色的工业环境下白平衡校正。";
}

bool EqualizeHistOp::Process(const cv::Mat& input, cv::Mat& output, std::string& errorMsg)
{
    if (input.empty()) { errorMsg = "输入图像为空"; return false; }

    int mode = _params[0].GetInt();
    int blend = _params[1].GetInt();
    double alpha = blend / 100.0;

    try
    {
        cv::Mat eqMat;

        if (input.channels() == 1 || mode == 0)
        {
            cv::Mat gray = input;
            if (input.channels() > 1) cv::cvtColor(input, gray, cv::COLOR_BGR2GRAY);
            cv::equalizeHist(gray, eqMat);
        }
        else if (mode == 1) // HSV V
        {
            cv::Mat hsv;
            cv::cvtColor(input, hsv, cv::COLOR_BGR2HSV);
            std::vector<cv::Mat> hsvChannels;
            cv::split(hsv, hsvChannels);
            cv::equalizeHist(hsvChannels[2], hsvChannels[2]);
            cv::merge(hsvChannels, hsv);
            cv::cvtColor(hsv, eqMat, cv::COLOR_HSV2BGR);
        }
        else if (mode == 2) // YCrCb Y
        {
            cv::Mat ycrcb;
            cv::cvtColor(input, ycrcb, cv::COLOR_BGR2YCrCb);
            std::vector<cv::Mat> ycrcbChannels;
            cv::split(ycrcb, ycrcbChannels);
            cv::equalizeHist(ycrcbChannels[0], ycrcbChannels[0]);
            cv::merge(ycrcbChannels, ycrcb);
            cv::cvtColor(ycrcb, eqMat, cv::COLOR_YCrCb2BGR);
        }
        else // BGR split
        {
            std::vector<cv::Mat> bgrChannels;
            cv::split(input, bgrChannels);
            for (auto& ch : bgrChannels) cv::equalizeHist(ch, ch);
            cv::merge(bgrChannels, eqMat);
        }

        if (blend < 100 && input.channels() == eqMat.channels())
        {
            cv::addWeighted(eqMat, alpha, input, 1.0 - alpha, 0, output);
        }
        else
        {
            output = eqMat;
        }
        return true;
    }
    catch (const cv::Exception& e)
    {
        errorMsg = e.what();
        return false;
    }
}

std::string EqualizeHistOp::GenerateCppCode(const std::string& inVar, const std::string& outVar) const
{
    int mode = _params[0].GetInt();
    int blend = _params[1].GetInt();
    double alpha = blend / 100.0;

    std::stringstream ss;
    ss << "// 直方图均衡化参数配置 (模式: " << mode << ", 混合度: " << blend << "%)\n";
    if (mode == 0)
    {
        ss << "cv::Mat gray_" << outVar << ";\n"
           << "if (" << inVar << ".channels() > 1) cv::cvtColor(" << inVar << ", gray_" << outVar << ", cv::COLOR_BGR2GRAY);\n"
           << "else gray_" << outVar << " = " << inVar << ";\n"
           << "cv::equalizeHist(gray_" << outVar << ", " << outVar << ");";
    }
    else if (mode == 1)
    {
        ss << "cv::Mat hsv_" << outVar << ";\n"
           << "cv::cvtColor(" << inVar << ", hsv_" << outVar << ", cv::COLOR_BGR2HSV);\n"
           << "std::vector<cv::Mat> hsvChannels;\n"
           << "cv::split(hsv_" << outVar << ", hsvChannels);\n"
           << "cv::equalizeHist(hsvChannels[2], hsvChannels[2]);\n"
           << "cv::merge(hsvChannels, hsv_" << outVar << ");\n"
           << "cv::cvtColor(hsv_" << outVar << ", " << outVar << ", cv::COLOR_HSV2BGR);";
    }
    else if (mode == 2)
    {
        ss << "cv::Mat ycrcb_" << outVar << ";\n"
           << "cv::cvtColor(" << inVar << ", ycrcb_" << outVar << ", cv::COLOR_BGR2YCrCb);\n"
           << "std::vector<cv::Mat> ycrcbChannels;\n"
           << "cv::split(ycrcb_" << outVar << ", ycrcbChannels);\n"
           << "cv::equalizeHist(ycrcbChannels[0], ycrcbChannels[0]);\n"
           << "cv::merge(ycrcbChannels, ycrcb_" << outVar << ");\n"
           << "cv::cvtColor(ycrcb_" << outVar << ", " << outVar << ", cv::COLOR_YCrCb2BGR);";
    }
    else
    {
        ss << "std::vector<cv::Mat> bgrChannels;\n"
           << "cv::split(" << inVar << ", bgrChannels);\n"
           << "for (auto& ch : bgrChannels) cv::equalizeHist(ch, ch);\n"
           << "cv::merge(bgrChannels, " << outVar << ");";
    }

    if (blend < 100)
    {
        ss << "\ncv::addWeighted(" << outVar << ", " << alpha << ", " << inVar << ", " << (1.0 - alpha) << ", 0, " << outVar << ");";
    }
    return ss.str();
}

std::string EqualizeHistOp::GeneratePythonCode(const std::string& inVar, const std::string& outVar) const
{
    int mode = _params[0].GetInt();
    int blend = _params[1].GetInt();
    double alpha = blend / 100.0;

    std::stringstream ss;
    ss << "# 直方图均衡化参数配置 (模式: " << mode << ", 混合度: " << blend << "%)\n";
    if (mode == 0)
    {
        ss << "gray = " << inVar << " if len(" << inVar << ".shape) == 2 else cv2.cvtColor(" << inVar << ", cv2.COLOR_BGR2GRAY)\n"
           << outVar << " = cv2.equalizeHist(gray)";
    }
    else if (mode == 1)
    {
        ss << "hsv = cv2.cvtColor(" << inVar << ", cv2.COLOR_BGR2HSV)\n"
           << "h, s, v = cv2.split(hsv)\n"
           << "v = cv2.equalizeHist(v)\n"
           << "hsv = cv2.merge([h, s, v])\n"
           << outVar << " = cv2.cvtColor(hsv, cv2.COLOR_HSV2BGR)";
    }
    else if (mode == 2)
    {
        ss << "ycrcb = cv2.cvtColor(" << inVar << ", cv2.COLOR_BGR2YCrCb)\n"
           << "y, cr, cb = cv2.split(ycrcb)\n"
           << "y = cv2.equalizeHist(y)\n"
           << "ycrcb = cv2.merge([y, cr, cb])\n"
           << outVar << " = cv2.cvtColor(ycrcb, cv2.COLOR_YCrCb2BGR)";
    }
    else
    {
        ss << "channels = cv2.split(" << inVar << ")\n"
           << "eq_channels = [cv2.equalizeHist(ch) for ch in channels]\n"
           << outVar << " = cv2.merge(eq_channels)";
    }

    if (blend < 100)
    {
        ss << "\n" << outVar << " = cv2.addWeighted(" << outVar << ", " << alpha << ", " << inVar << ", " << (1.0 - alpha) << ", 0)";
    }
    return ss.str();
}

// ---------------- CLAHEOp ----------------
CLAHEOp::CLAHEOp()
{
    OpParameter clipParam;
    clipParam.Name = "clipLimit";
    clipParam.DisplayName = "对比度截断限制 (ClipLimit)";
    clipParam.Type = ParamType::DoubleSlider;
    clipParam.Value = 2.0;
    clipParam.MinValue = 1.0;
    clipParam.MaxValue = 10.0;
    clipParam.Step = 0.5;
    _params.push_back(clipParam);

    OpParameter gridParam;
    gridParam.Name = "tileGridSize";
    gridParam.DisplayName = "网格分块大小 (TileGridSize)";
    gridParam.Type = ParamType::IntSlider;
    gridParam.Value = 8;
    gridParam.MinValue = 2;
    gridParam.MaxValue = 32;
    gridParam.Step = 2;
    _params.push_back(gridParam);
}

std::string CLAHEOp::GetPrincipleDoc() const
{
    return "### 📘 算子原理：限制对比度自适应直方图均衡化 (CLAHE)\n\n"
           "将图像划分为互不重叠的小网格（Tiles），在局部计算直方图均衡并应用 **ClipLimit** 截断过高直方图峰值以防止局部背景噪声被过度放大，最后通过双线性插值消除块效应。";
}

bool CLAHEOp::Process(const cv::Mat& input, cv::Mat& output, std::string& errorMsg)
{
    if (input.empty()) { errorMsg = "输入图像为空"; return false; }
    cv::Mat gray = input;
    if (input.channels() > 1) cv::cvtColor(input, gray, cv::COLOR_BGR2GRAY);

    double clipLimit = _params[0].GetDouble();
    int grid = _params[1].GetInt();

    try
    {
        cv::Ptr<cv::CLAHE> clahe = cv::createCLAHE(clipLimit, cv::Size(grid, grid));
        clahe->apply(gray, output);
        return true;
    }
    catch (const cv::Exception& e)
    {
        errorMsg = e.what();
        return false;
    }
}

std::string CLAHEOp::GenerateCppCode(const std::string& inVar, const std::string& outVar) const
{
    double clipLimit = _params[0].GetDouble();
    int grid = _params[1].GetInt();

    std::stringstream ss;
    ss << "// CLAHE限制对比度自适应直方图均衡化参数配置\n"
       << "double clipLimit = " << clipLimit << ";          // 局部对比度截断限制 (防止过度放大噪点)\n"
       << "int tileGrid = " << grid << ";               // 分块网格尺寸\n"
       << "cv::Ptr<cv::CLAHE> clahe = cv::createCLAHE(clipLimit, cv::Size(tileGrid, tileGrid));\n"
       << "clahe->apply(" << inVar << ", " << outVar << ");";
    return ss.str();
}

std::string CLAHEOp::GeneratePythonCode(const std::string& inVar, const std::string& outVar) const
{
    double clipLimit = _params[0].GetDouble();
    int grid = _params[1].GetInt();

    std::stringstream ss;
    ss << "# CLAHE限制对比度自适应直方图均衡化参数配置\n"
       << "clip_limit = " << clipLimit << "              # 局部对比度截断限制\n"
       << "tile_grid = (" << grid << ", " << grid << ")           # 分块网格尺寸\n"
       << "clahe = cv2.createCLAHE(clipLimit=clip_limit, tileGridSize=tile_grid)\n"
       << outVar << " = clahe.apply(" << inVar << ")";
    return ss.str();
}

// ---------------- GammaCorrectionOp ----------------
GammaCorrectionOp::GammaCorrectionOp()
{
    OpParameter gammaParam;
    gammaParam.Name = "gamma";
    gammaParam.DisplayName = "伽马系数 (Gamma γ)";
    gammaParam.Type = ParamType::DoubleSlider;
    gammaParam.Value = 1.5;
    gammaParam.MinValue = 0.1;
    gammaParam.MaxValue = 4.0;
    gammaParam.Step = 0.05;
    _params.push_back(gammaParam);
}

std::string GammaCorrectionOp::GetPrincipleDoc() const
{
    return "### 📘 算子原理：非线性伽马校正 (Gamma Correction)\n\n"
           "非线性灰度变换模型：\n"
           "$$dst(x, y) = 255 \\cdot \\left( \\frac{src(x, y)}{255} \\right)^\\gamma$$\n"
           "* **$\\gamma < 1$**：暗部非线性拉伸扩展，提亮阴影细节；\n"
           "* **$\\gamma > 1$**：压低暗部，保留高光反差。";
}

bool GammaCorrectionOp::Process(const cv::Mat& input, cv::Mat& output, std::string& errorMsg)
{
    if (input.empty()) { errorMsg = "输入图像为空"; return false; }
    double gamma = _params[0].GetDouble();

    try
    {
        unsigned char lut[256];
        for (int i = 0; i < 256; ++i)
        {
            lut[i] = cv::saturate_cast<uchar>(std::pow(i / 255.0, gamma) * 255.0);
        }
        cv::Mat lutTable(1, 256, CV_8U, lut);
        cv::LUT(input, lutTable, output);
        return true;
    }
    catch (const cv::Exception& e)
    {
        errorMsg = e.what();
        return false;
    }
}

std::string GammaCorrectionOp::GenerateCppCode(const std::string& inVar, const std::string& outVar) const
{
    double gamma = _params[0].GetDouble();

    std::stringstream ss;
    ss << "// 非线性伽马校正参数配置 (通过查找表 LUT 加速)\n"
       << "double gammaValue = " << gamma << ";        // 伽马系数 (<1提亮暗部, >1压暗)\n"
       << "unsigned char lut[256];\n"
       << "for (int i = 0; i < 256; ++i) {\n"
       << "    lut[i] = cv::saturate_cast<uchar>(std::pow(i / 255.0, gammaValue) * 255.0);\n"
       << "}\n"
       << "cv::Mat lutTable(1, 256, CV_8U, lut);\n"
       << "cv::LUT(" << inVar << ", lutTable, " << outVar << ");";
    return ss.str();
}

std::string GammaCorrectionOp::GeneratePythonCode(const std::string& inVar, const std::string& outVar) const
{
    double gamma = _params[0].GetDouble();

    std::stringstream ss;
    ss << "# 非线性伽马校正参数配置\n"
       << "gamma_val = " << gamma << "              # 伽马系数\n"
       << "inv_gamma = gamma_val\n"
       << "table = np.array([((i / 255.0) ** inv_gamma) * 255 for i in np.arange(0, 256)]).astype(\"uint8\")\n"
       << outVar << " = cv2.LUT(" << inVar << ", table)";
    return ss.str();
}

// ---------------- InvertOp ----------------
InvertOp::InvertOp()
{
    OpParameter modeParam;
    modeParam.Name = "mode";
    modeParam.DisplayName = "反相通道选择 (Target Channels)";
    modeParam.Type = ParamType::ComboBox;
    modeParam.Value = 0;
    modeParam.EnumOptions = {
        "所有通道反相 (Full Invert)",
        "仅反相 Blue/单色通道",
        "仅反相 Green 通道",
        "仅反相 Red 通道"
    };
    _params.push_back(modeParam);

    OpParameter blendParam;
    blendParam.Name = "blend";
    blendParam.DisplayName = "混合透明度 (Blend Ratio %)";
    blendParam.Type = ParamType::IntSlider;
    blendParam.Value = 100;
    blendParam.MinValue = 10;
    blendParam.MaxValue = 100;
    blendParam.Step = 5;
    _params.push_back(blendParam);
}

std::string InvertOp::GetPrincipleDoc() const
{
    return "### 📘 算子原理：图像颜色反相 (bitwise_not)\n\n"
           "对图像中像素进行按位非运算（$dst = 255 - src$），生成底片反色效果，支持单通道独立反相与半透明混合。";
}

bool InvertOp::Process(const cv::Mat& input, cv::Mat& output, std::string& errorMsg)
{
    if (input.empty()) { errorMsg = "输入图像为空"; return false; }
    int mode = _params[0].GetInt();
    int blend = _params[1].GetInt();
    double alpha = blend / 100.0;

    try
    {
        cv::Mat invMat = input.clone();
        if (input.channels() == 1 || mode == 0)
        {
            cv::bitwise_not(input, invMat);
        }
        else if (input.channels() == 3)
        {
            std::vector<cv::Mat> chs;
            cv::split(input, chs);
            if (mode == 1) cv::bitwise_not(chs[0], chs[0]);
            else if (mode == 2) cv::bitwise_not(chs[1], chs[1]);
            else if (mode == 3) cv::bitwise_not(chs[2], chs[2]);
            cv::merge(chs, invMat);
        }

        if (blend < 100)
        {
            cv::addWeighted(invMat, alpha, input, 1.0 - alpha, 0, output);
        }
        else
        {
            output = invMat;
        }
        return true;
    }
    catch (const cv::Exception& e)
    {
        errorMsg = e.what();
        return false;
    }
}

std::string InvertOp::GenerateCppCode(const std::string& inVar, const std::string& outVar) const
{
    int mode = _params[0].GetInt();
    int blend = _params[1].GetInt();
    double alpha = blend / 100.0;

    std::stringstream ss;
    ss << "// 图像颜色反相 (模式: " << mode << ")\n";
    if (mode == 0)
    {
        ss << "cv::bitwise_not(" << inVar << ", " << outVar << ");";
    }
    else
    {
        ss << "std::vector<cv::Mat> chs;\n"
           << "cv::split(" << inVar << ", chs);\n"
           << "cv::bitwise_not(chs[" << (mode - 1) << "], chs[" << (mode - 1) << "]);\n"
           << "cv::merge(chs, " << outVar << ");";
    }

    if (blend < 100)
    {
        ss << "\ncv::addWeighted(" << outVar << ", " << alpha << ", " << inVar << ", " << (1.0 - alpha) << ", 0, " << outVar << ");";
    }
    return ss.str();
}

std::string InvertOp::GeneratePythonCode(const std::string& inVar, const std::string& outVar) const
{
    int mode = _params[0].GetInt();
    int blend = _params[1].GetInt();
    double alpha = blend / 100.0;

    std::stringstream ss;
    ss << "# 图像颜色反相 (模式: " << mode << ")\n";
    if (mode == 0)
    {
        ss << outVar << " = cv2.bitwise_not(" << inVar << ")";
    }
    else
    {
        ss << "chs = list(cv2.split(" << inVar << "))\n"
           << "chs[" << (mode - 1) << "] = cv2.bitwise_not(chs[" << (mode - 1) << "])\n"
           << outVar << " = cv2.merge(chs)";
    }

    if (blend < 100)
    {
        ss << "\n" << outVar << " = cv2.addWeighted(" << outVar << ", " << alpha << ", " << inVar << ", " << (1.0 - alpha) << ", 0)";
    }
    return ss.str();
}

// ---------------- SharpenOp ----------------
SharpenOp::SharpenOp()
{
    OpParameter stParam;
    stParam.Name = "strength";
    stParam.DisplayName = "锐化强度 (Strength)";
    stParam.Type = ParamType::DoubleSlider;
    stParam.Value = 1.0;
    stParam.MinValue = 0.2;
    stParam.MaxValue = 3.0;
    stParam.Step = 0.1;
    _params.push_back(stParam);
}

std::string SharpenOp::GetPrincipleDoc() const
{
    return "### 📘 算子原理：USM 反锐化掩膜 (Unsharp Masking)\n\n"
           "基于原图减去高斯平滑模糊图得到高频细节掩膜，再按强度权重叠加回原图：\n"
           "$$dst = src + strength \\cdot (src - GaussianBlur(src))$$";
}

bool SharpenOp::Process(const cv::Mat& input, cv::Mat& output, std::string& errorMsg)
{
    if (input.empty()) { errorMsg = "输入图像为空"; return false; }
    double strength = _params[0].GetDouble();

    try
    {
        cv::Mat blurred;
        cv::GaussianBlur(input, blurred, cv::Size(0, 0), 3);
        cv::addWeighted(input, 1.0 + strength, blurred, -strength, 0, output);
        return true;
    }
    catch (const cv::Exception& e)
    {
        errorMsg = e.what();
        return false;
    }
}

std::string SharpenOp::GenerateCppCode(const std::string& inVar, const std::string& outVar) const
{
    double strength = _params[0].GetDouble();

    std::stringstream ss;
    ss << "// USM反锐化掩膜增强参数配置\n"
       << "double sharpenStrength = " << strength << ";  // 锐化高频增益强度\n"
       << "cv::Mat blurred_" << outVar << ";\n"
       << "cv::GaussianBlur(" << inVar << ", blurred_" << outVar << ", cv::Size(0, 0), 3.0);\n"
       << "cv::addWeighted(" << inVar << ", 1.0 + sharpenStrength, blurred_" << outVar << ", -sharpenStrength, 0, " << outVar << ");";
    return ss.str();
}

std::string SharpenOp::GeneratePythonCode(const std::string& inVar, const std::string& outVar) const
{
    double strength = _params[0].GetDouble();

    std::stringstream ss;
    ss << "# USM反锐化掩膜增强参数配置\n"
       << "sharpen_strength = " << strength << "         # 锐化高频增益强度\n"
       << "blurred = cv2.GaussianBlur(" << inVar << ", (0, 0), 3.0)\n"
       << outVar << " = cv2.addWeighted(" << inVar << ", 1.0 + sharpen_strength, blurred, -sharpen_strength, 0)";
    return ss.str();
}
