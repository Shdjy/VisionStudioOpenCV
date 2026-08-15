#include "EnhanceOps.h"
#include <sstream>

// ---------------- BrightnessContrastOp ----------------
BrightnessContrastOp::BrightnessContrastOp()
{
    OpParameter alphaParam;
    alphaParam.Name = "alpha";
    alphaParam.DisplayName = "对比度增益 (Alpha)";
    alphaParam.Type = ParamType::DoubleSlider;
    alphaParam.Value = 1.0;
    alphaParam.MinValue = 0.1;
    alphaParam.MaxValue = 3.0;
    alphaParam.Step = 0.05;
    _params.push_back(alphaParam);

    OpParameter betaParam;
    betaParam.Name = "beta";
    betaParam.DisplayName = "亮度偏置 (Beta)";
    betaParam.Type = ParamType::IntSlider;
    betaParam.Value = 0;
    betaParam.MinValue = -100;
    betaParam.MaxValue = 100;
    betaParam.Step = 1;
    _params.push_back(betaParam);
}

std::string BrightnessContrastOp::GetPrincipleDoc() const
{
    return "### 📘 算子原理：亮度和对比度线性调节 (convertScaleAbs)\n\n"
           "线性点变换模型：\n"
           "$$dst(x, y) = \\text{saturate\\_cast}\\langle uchar \\rangle (\\alpha \\cdot src(x, y) + \\beta)$$\n"
           "* **$\\alpha > 1$**：增强对比度（动态范围扩大）；$\\alpha < 1$：降低对比度；\n"
           "* **$\\beta > 0$**：整体提亮；$\\beta < 0$：整体变暗。";
}

bool BrightnessContrastOp::Process(const cv::Mat& input, cv::Mat& output, std::string& errorMsg)
{
    if (input.empty()) { errorMsg = "输入图像为空"; return false; }
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
    ss << "// 亮度与对比度线性变换参数配置\n"
       << "double alpha = " << alpha << ";            // 对比度增益系数 (Alpha)\n"
       << "double beta = " << beta << ";              // 亮度偏移量 (Beta)\n"
       << inVar << ".convertTo(" << outVar << ", -1, alpha, beta);";
    return ss.str();
}

std::string BrightnessContrastOp::GeneratePythonCode(const std::string& inVar, const std::string& outVar) const
{
    double alpha = _params[0].GetDouble();
    int beta = _params[1].GetInt();

    std::stringstream ss;
    ss << "# 亮度与对比度线性变换参数配置\n"
       << "alpha = " << alpha << "                   # 对比度增益系数\n"
       << "beta = " << beta << "                     # 亮度偏移量\n"
       << outVar << " = cv2.convertScaleAbs(" << inVar << ", alpha=alpha, beta=beta)";
    return ss.str();
}

// ---------------- EqualizeHistOp ----------------
EqualizeHistOp::EqualizeHistOp()
{
}

std::string EqualizeHistOp::GetPrincipleDoc() const
{
    return "### 📘 算子原理：全局直方图均衡化 (equalizeHist)\n\n"
           "通过累积分布函数 (CDF) 将原始图像偏窄的灰度级动态范围非线性拉伸至 $[0, 255]$ 均匀分布，大幅提升图像整体对比度与暗部细节。";
}

bool EqualizeHistOp::Process(const cv::Mat& input, cv::Mat& output, std::string& errorMsg)
{
    if (input.empty()) { errorMsg = "输入图像为空"; return false; }
    cv::Mat gray = input;
    if (input.channels() > 1) cv::cvtColor(input, gray, cv::COLOR_BGR2GRAY);

    try
    {
        cv::equalizeHist(gray, output);
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
    ss << "// 全局直方图均衡化 (扩展灰度动态范围)\n"
       << "cv::equalizeHist(" << inVar << ", " << outVar << ");";
    return ss.str();
}

std::string EqualizeHistOp::GeneratePythonCode(const std::string& inVar, const std::string& outVar) const
{
    std::stringstream ss;
    ss << "# 全局直方图均衡化 (扩展灰度动态范围)\n"
       << outVar << " = cv2.equalizeHist(" << inVar << ")";
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
    gammaParam.Value = 1.0;
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
}

std::string InvertOp::GetPrincipleDoc() const
{
    return "### 📘 算子原理：图像反相 (bitwise_not)\n\n"
           "对图像中每个像素进行按位非运算（$dst = 255 - src$），生成底片反色效果。";
}

bool InvertOp::Process(const cv::Mat& input, cv::Mat& output, std::string& errorMsg)
{
    if (input.empty()) { errorMsg = "输入图像为空"; return false; }
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
    ss << "// 图像颜色反相 (底片效果)\n"
       << "cv::bitwise_not(" << inVar << ", " << outVar << ");";
    return ss.str();
}

std::string InvertOp::GeneratePythonCode(const std::string& inVar, const std::string& outVar) const
{
    std::stringstream ss;
    ss << "# 图像颜色反相 (底片效果)\n"
       << outVar << " = cv2.bitwise_not(" << inVar << ")";
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
