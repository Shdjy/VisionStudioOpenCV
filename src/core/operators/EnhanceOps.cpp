#include "EnhanceOps.h"
#include <sstream>
#include <cmath>

// ---------------- BrightnessContrastOp ----------------
BrightnessContrastOp::BrightnessContrastOp()
{
    OpParameter alphaParam;
    alphaParam.Name = "alpha";
    alphaParam.DisplayName = "对比度增益 (Alpha)";
    alphaParam.Tooltip = "对比度乘数因子 (1.0 为原图，>1 增强对比度，<1 降低对比度)";
    alphaParam.Type = ParamType::DoubleSlider;
    alphaParam.Value = 1.2;
    alphaParam.MinValue = 0.1;
    alphaParam.MaxValue = 3.0;
    alphaParam.Step = 0.05;
    _params.push_back(alphaParam);

    OpParameter betaParam;
    betaParam.Name = "beta";
    betaParam.DisplayName = "亮度偏置 (Beta)";
    betaParam.Tooltip = "亮度增减偏置值 (-100 ~ 100)";
    betaParam.Type = ParamType::IntSlider;
    betaParam.Value = 10;
    betaParam.MinValue = -100;
    betaParam.MaxValue = 100;
    betaParam.Step = 1;
    _params.push_back(betaParam);
}

std::string BrightnessContrastOp::GetPrincipleDoc() const
{
    return "### 📘 算子原理：亮度与对比度线性变换\n\n"
           "对每个像素执行线性变换公式：\n"
           "$$g(x,y) = \\alpha \\cdot f(x,y) + \\beta$$\n"
           "* $\\alpha$ (对比度/斜率)：控制图像明暗拉伸跨度；\n"
           "* $\\beta$ (亮度/截距)：控制图像整体明暗平移。";
}

bool BrightnessContrastOp::Process(const cv::Mat& input, cv::Mat& output, std::string& errorMsg)
{
    if (input.empty())
    {
        errorMsg = "输入图像为空";
        return false;
    }

    double alpha = _params[0].GetDouble();
    int beta = _params[1].GetInt();

    try
    {
        input.convertTo(output, -1, alpha, beta);
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

    std::stringstream ss;
    ss << inVar << ".convertTo(" << outVar << ", -1, " << alpha << ", " << beta << ");";
    return ss.str();
}

std::string BrightnessContrastOp::GeneratePythonCode(const std::string& inVar, const std::string& outVar) const
{
    double alpha = _params[0].GetDouble();
    int beta = _params[1].GetInt();

    std::stringstream ss;
    ss << outVar << " = cv2.convertScaleAbs(" << inVar << ", alpha=" << alpha << ", beta=" << beta << ")";
    return ss.str();
}

// ---------------- EqualizeHistOp ----------------
EqualizeHistOp::EqualizeHistOp()
{
}

std::string EqualizeHistOp::GetPrincipleDoc() const
{
    return "### 📘 算子原理：直方图均衡化 (equalizeHist)\n\n"
           "通过累积分布函数 (CDF) 将图像原先集中在窄带区间的灰度直方图重新映射为全灰度范围 (0~255) 的均匀分布：\n"
           "$$s_k = T(r_k) = (L-1) \\sum_{j=0}^{k} p_r(r_j)$$\n"
           "能够显著增强低照度与背光图像的对比度与层次感。";
}

bool EqualizeHistOp::Process(const cv::Mat& input, cv::Mat& output, std::string& errorMsg)
{
    if (input.empty())
    {
        errorMsg = "输入图像为空";
        return false;
    }

    try
    {
        if (input.channels() == 1)
        {
            cv::equalizeHist(input, output);
        }
        else if (input.channels() == 3)
        {
            cv::Mat ycrcb;
            cv::cvtColor(input, ycrcb, cv::COLOR_BGR2YCrCb);
            std::vector<cv::Mat> channels;
            cv::split(ycrcb, channels);
            cv::equalizeHist(channels[0], channels[0]); // 仅对 Y 亮度通道均衡
            cv::merge(channels, ycrcb);
            cv::cvtColor(ycrcb, output, cv::COLOR_YCrCb2BGR);
        }
        else
        {
            output = input.clone();
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
    std::stringstream ss;
    ss << "if (" << inVar << ".channels() == 1)\n"
       << "{\n"
       << "    cv::equalizeHist(" << inVar << ", " << outVar << ");\n"
       << "}\n"
       << "else\n"
       << "{\n"
       << "    cv::Mat ycrcb;\n"
       << "    cv::cvtColor(" << inVar << ", ycrcb, cv::COLOR_BGR2YCrCb);\n"
       << "    std::vector<cv::Mat> channels;\n"
       << "    cv::split(ycrcb, channels);\n"
       << "    cv::equalizeHist(channels[0], channels[0]);\n"
       << "    cv::merge(channels, ycrcb);\n"
       << "    cv::cvtColor(ycrcb, " << outVar << ", cv::COLOR_YCrCb2BGR);\n"
       << "}";
    return ss.str();
}

std::string EqualizeHistOp::GeneratePythonCode(const std::string& inVar, const std::string& outVar) const
{
    std::stringstream ss;
    ss << "if len(" << inVar << ".shape) == 2 or " << inVar << ".shape[2] == 1:\n"
       << "    " << outVar << " = cv2.equalizeHist(" << inVar << ")\n"
       << "else:\n"
       << "    ycrcb = cv2.cvtColor(" << inVar << ", cv2.COLOR_BGR2YCrCb)\n"
       << "    channels = list(cv2.split(ycrcb))\n"
       << "    channels[0] = cv2.equalizeHist(channels[0])\n"
       << "    " << outVar << " = cv2.cvtColor(cv2.merge(channels), cv2.COLOR_YCrCb2BGR)";
    return ss.str();
}

// ---------------- CLAHEOp ----------------
CLAHEOp::CLAHEOp()
{
    OpParameter clipParam;
    clipParam.Name = "clipLimit";
    clipParam.DisplayName = "对比度截断限制 (ClipLimit)";
    clipParam.Tooltip = "限制对比度放大倍数阈值 (通常 2.0 ~ 4.0)";
    clipParam.Type = ParamType::DoubleSlider;
    clipParam.Value = 2.0;
    clipParam.MinValue = 1.0;
    clipParam.MaxValue = 10.0;
    clipParam.Step = 0.5;
    _params.push_back(clipParam);

    OpParameter gridParam;
    gridParam.Name = "tileGridSize";
    gridParam.DisplayName = "网格分块大小 (TileGridSize)";
    gridParam.Tooltip = "图像划分的网格行列大小 (如 8x8)";
    gridParam.Type = ParamType::IntSlider;
    gridParam.Value = 8;
    gridParam.MinValue = 2;
    gridParam.MaxValue = 32;
    gridParam.Step = 1;
    _params.push_back(gridParam);
}

std::string CLAHEOp::GetPrincipleDoc() const
{
    return "### 📘 算子原理：限制对比度自适应直方图均衡化 (CLAHE)\n\n"
           "普通直方图均衡化倾向于过度放大平坦背景区域的噪声。CLAHE 通过两项关键改进：\n"
           "1. **局部瓦片网格 (Tile Grid)**：将图像划分为若干不重叠的矩形块独立进行均衡化；\n"
           "2. **对比度截断限制 (Clip Limit)**：对超出上限阈值的直方图计数进行裁剪并均匀重分配；\n"
           "3. **双线性插值**：缝合各网格边缘，消除分块边界人工痕迹。";
}

bool CLAHEOp::Process(const cv::Mat& input, cv::Mat& output, std::string& errorMsg)
{
    if (input.empty())
    {
        errorMsg = "输入图像为空";
        return false;
    }

    double clipLimit = _params[0].GetDouble();
    int grid = _params[1].GetInt();
    if (grid < 2) grid = 2;

    try
    {
        cv::Ptr<cv::CLAHE> clahe = cv::createCLAHE(clipLimit, cv::Size(grid, grid));
        if (input.channels() == 1)
        {
            clahe->apply(input, output);
        }
        else if (input.channels() == 3)
        {
            cv::Mat lab;
            cv::cvtColor(input, lab, cv::COLOR_BGR2Lab);
            std::vector<cv::Mat> channels;
            cv::split(lab, channels);
            clahe->apply(channels[0], channels[0]); // 仅对 L 亮度通道处理
            cv::merge(channels, lab);
            cv::cvtColor(lab, output, cv::COLOR_Lab2BGR);
        }
        else
        {
            output = input.clone();
        }
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
    if (grid < 2) grid = 2;

    std::stringstream ss;
    ss << "cv::Ptr<cv::CLAHE> clahe = cv::createCLAHE(" << clipLimit << ", cv::Size(" << grid << ", " << grid << "));\n"
       << "if (" << inVar << ".channels() == 1)\n"
       << "{\n"
       << "    clahe->apply(" << inVar << ", " << outVar << ");\n"
       << "}\n"
       << "else\n"
       << "{\n"
       << "    cv::Mat lab;\n"
       << "    cv::cvtColor(" << inVar << ", lab, cv::COLOR_BGR2Lab);\n"
       << "    std::vector<cv::Mat> channels;\n"
       << "    cv::split(lab, channels);\n"
       << "    clahe->apply(channels[0], channels[0]);\n"
       << "    cv::merge(channels, lab);\n"
       << "    cv::cvtColor(lab, " << outVar << ", cv::COLOR_Lab2BGR);\n"
       << "}";
    return ss.str();
}

std::string CLAHEOp::GeneratePythonCode(const std::string& inVar, const std::string& outVar) const
{
    double clipLimit = _params[0].GetDouble();
    int grid = _params[1].GetInt();
    if (grid < 2) grid = 2;

    std::stringstream ss;
    ss << "clahe = cv2.createCLAHE(clipLimit=" << clipLimit << ", tileGridSize=(" << grid << ", " << grid << "))\n"
       << "if len(" << inVar << ".shape) == 2 or " << inVar << ".shape[2] == 1:\n"
       << "    " << outVar << " = clahe.apply(" << inVar << ")\n"
       << "else:\n"
       << "    lab = cv2.cvtColor(" << inVar << ", cv2.COLOR_BGR2Lab)\n"
       << "    channels = list(cv2.split(lab))\n"
       << "    channels[0] = clahe.apply(channels[0])\n"
       << "    " << outVar << " = cv2.cvtColor(cv2.merge(channels), cv2.COLOR_Lab2BGR)";
    return ss.str();
}

// ---------------- GammaCorrectionOp ----------------
GammaCorrectionOp::GammaCorrectionOp()
{
    OpParameter gammaParam;
    gammaParam.Name = "gamma";
    gammaParam.DisplayName = "伽马值 (Gamma)";
    gammaParam.Tooltip = "Gamma 参数 (>1 增强暗部，<1 压暗亮部)";
    gammaParam.Type = ParamType::DoubleSlider;
    gammaParam.Value = 1.5;
    gammaParam.MinValue = 0.1;
    gammaParam.MaxValue = 4.0;
    gammaParam.Step = 0.05;
    _params.push_back(gammaParam);
}

std::string GammaCorrectionOp::GetPrincipleDoc() const
{
    return "### 📘 算子原理：伽马校正 (Gamma Correction)\n\n"
           "基于幂律响应曲线的非线性明暗调节：\n"
           "$$s = 255 \\cdot \\left( \\frac{r}{255} \\right)^\\gamma$$\n"
           "通常预先建立 256 长度的 Look-Up Table (LUT) 查找表加速逐像素映射。";
}

bool GammaCorrectionOp::Process(const cv::Mat& input, cv::Mat& output, std::string& errorMsg)
{
    if (input.empty())
    {
        errorMsg = "输入图像为空";
        return false;
    }

    double gamma = _params[0].GetDouble();
    if (gamma <= 0.01) gamma = 0.01;

    try
    {
        cv::Mat lut(1, 256, CV_8U);
        uchar* ptr = lut.ptr();
        for (int i = 0; i < 256; ++i)
        {
            ptr[i] = cv::saturate_cast<uchar>(std::pow(i / 255.0, 1.0 / gamma) * 255.0);
        }
        cv::LUT(input, lut, output);
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
    ss << "cv::Mat lut(1, 256, CV_8U);\n"
       << "uchar* ptr = lut.ptr();\n"
       << "for (int i = 0; i < 256; ++i) ptr[i] = cv::saturate_cast<uchar>(std::pow(i / 255.0, 1.0 / " << gamma << ") * 255.0);\n"
       << "cv::LUT(" << inVar << ", lut, " << outVar << ");";
    return ss.str();
}

std::string GammaCorrectionOp::GeneratePythonCode(const std::string& inVar, const std::string& outVar) const
{
    double gamma = _params[0].GetDouble();
    std::stringstream ss;
    ss << "invGamma = 1.0 / " << gamma << "\n"
       << "table = np.array([((i / 255.0) ** invGamma) * 255 for i in np.arange(0, 256)]).astype('uint8')\n"
       << outVar << " = cv2.LUT(" << inVar << ", table)";
    return ss.str();
}

// ---------------- InvertOp ----------------
InvertOp::InvertOp()
{
}

std::string InvertOp::GetPrincipleDoc() const
{
    return "### 📘 算子原理：图像反色 (bitwise_not)\n\n"
           "逐像素执行按位非操作 $\\text{dst}(x,y) = 255 - \\text{src}(x,y)$，常用于将黑底白字转换为白底黑字或生成负片效果。";
}

bool InvertOp::Process(const cv::Mat& input, cv::Mat& output, std::string& errorMsg)
{
    if (input.empty())
    {
        errorMsg = "输入图像为空";
        return false;
    }

    try
    {
        cv::bitwise_not(input, output);
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
    std::stringstream ss;
    ss << "cv::bitwise_not(" << inVar << ", " << outVar << ");";
    return ss.str();
}

std::string InvertOp::GeneratePythonCode(const std::string& inVar, const std::string& outVar) const
{
    std::stringstream ss;
    ss << outVar << " = cv2.bitwise_not(" << inVar << ")";
    return ss.str();
}

// ---------------- SharpenOp ----------------
SharpenOp::SharpenOp()
{
    OpParameter strengthParam;
    strengthParam.Name = "strength";
    strengthParam.DisplayName = "锐化强度 (Strength)";
    strengthParam.Tooltip = "锐化增强倍率 (1.0 ~ 5.0)";
    strengthParam.Type = ParamType::DoubleSlider;
    strengthParam.Value = 1.5;
    strengthParam.MinValue = 0.5;
    strengthParam.MaxValue = 5.0;
    strengthParam.Step = 0.1;
    _params.push_back(strengthParam);
}

std::string SharpenOp::GetPrincipleDoc() const
{
    return "### 📘 算子原理：拉普拉斯高通锐化 (Sharpen)\n\n"
           "构造拉普拉斯锐化滤波算子核，将原图像与高频边缘差分加权融合，增强模糊边缘反差。";
}

bool SharpenOp::Process(const cv::Mat& input, cv::Mat& output, std::string& errorMsg)
{
    if (input.empty())
    {
        errorMsg = "输入图像为空";
        return false;
    }

    double s = _params[0].GetDouble();
    cv::Mat kernel = (cv::Mat_<float>(3, 3) <<  0,  -s,   0,
                                               -s, 1 + 4*s, -s,
                                                0,  -s,   0);
    try
    {
        cv::filter2D(input, output, -1, kernel);
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
    double s = _params[0].GetDouble();
    std::stringstream ss;
    ss << "cv::Mat kernel_" << outVar << " = (cv::Mat_<float>(3,3) << 0, -" << s << ", 0, -" << s << ", " << 1 + 4*s << ", -" << s << ", 0, -" << s << ", 0);\n"
       << "cv::filter2D(" << inVar << ", " << outVar << ", -1, kernel_" << outVar << ");";
    return ss.str();
}

std::string SharpenOp::GeneratePythonCode(const std::string& inVar, const std::string& outVar) const
{
    double s = _params[0].GetDouble();
    std::stringstream ss;
    ss << "kernel_" << outVar << " = np.array([[0, -" << s << ", 0], [-" << s << ", " << 1 + 4*s << ", -" << s << "], [0, -" << s << ", 0]], dtype=np.float32)\n"
       << outVar << " = cv2.filter2D(" << inVar << ", -1, kernel_" << outVar << ")";
    return ss.str();
}
