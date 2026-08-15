#include "FilterOps.h"
#include <sstream>

// ---------------- GaussianBlurOp ----------------
GaussianBlurOp::GaussianBlurOp()
{
    OpParameter ksizeParam;
    ksizeParam.Name = "ksize";
    ksizeParam.DisplayName = "高斯核大小 (KSize)";
    ksizeParam.Tooltip = "核宽和高，必须为正奇数 (如 3, 5, 7...)";
    ksizeParam.Type = ParamType::IntSlider;
    ksizeParam.Value = 5;
    ksizeParam.MinValue = 1;
    ksizeParam.MaxValue = 31;
    ksizeParam.Step = 2;
    _params.push_back(ksizeParam);

    OpParameter sigmaParam;
    sigmaParam.Name = "sigmaX";
    sigmaParam.DisplayName = "X方向标准差 (SigmaX)";
    sigmaParam.Tooltip = "高斯核的标准差，0 表示根据核大小自动计算";
    sigmaParam.Type = ParamType::DoubleSlider;
    sigmaParam.Value = 1.5;
    sigmaParam.MinValue = 0.0;
    sigmaParam.MaxValue = 10.0;
    sigmaParam.Step = 0.1;
    _params.push_back(sigmaParam);
}

std::string GaussianBlurOp::GetPrincipleDoc() const
{
    return "### 📘 算子原理：高斯滤波 (GaussianBlur)\n\n"
           "高斯滤波是一种线性平滑滤波，其权重服从二维正态高斯分布：\n"
           "$$G(x, y) = \\frac{1}{2\\pi\\sigma^2} e^{-\\frac{x^2+y^2}{2\\sigma^2}}$$\n"
           "**特点**：越靠近核中心的像素权重越大，非常适合消除服从高斯分布的高频随机噪声。";
}

bool GaussianBlurOp::Process(const cv::Mat& input, cv::Mat& output, std::string& errorMsg)
{
    if (input.empty()) { errorMsg = "输入图像为空"; return false; }
    int ksize = _params[0].GetInt();
    if (ksize % 2 == 0) ksize += 1;
    double sigmaX = _params[1].GetDouble();

    try
    {
        cv::GaussianBlur(input, output, cv::Size(ksize, ksize), sigmaX);
        return true;
    }
    catch (const cv::Exception& e)
    {
        errorMsg = e.what();
        return false;
    }
}

std::string GaussianBlurOp::GenerateCppCode(const std::string& inVar, const std::string& outVar) const
{
    int ksize = _params[0].GetInt();
    if (ksize % 2 == 0) ksize += 1;
    double sigmaX = _params[1].GetDouble();

    std::stringstream ss;
    ss << "// 高斯滤波参数配置\n"
       << "int ksize = " << ksize << ";             // 核大小 (正奇数)\n"
       << "double sigmaX = " << sigmaX << ";         // X方向标准差\n"
       << "cv::GaussianBlur(" << inVar << ", " << outVar << ", cv::Size(ksize, ksize), sigmaX);";
    return ss.str();
}

std::string GaussianBlurOp::GeneratePythonCode(const std::string& inVar, const std::string& outVar) const
{
    int ksize = _params[0].GetInt();
    if (ksize % 2 == 0) ksize += 1;
    double sigmaX = _params[1].GetDouble();

    std::stringstream ss;
    ss << "# 高斯滤波参数配置\n"
       << "ksize = (" << ksize << ", " << ksize << ")         # 核大小 (正奇数)\n"
       << "sigma_x = " << sigmaX << "            # X方向标准差\n"
       << outVar << " = cv2.GaussianBlur(" << inVar << ", ksize, sigma_x)";
    return ss.str();
}

// ---------------- BlurOp ----------------
BlurOp::BlurOp()
{
    OpParameter ksizeParam;
    ksizeParam.Name = "ksize";
    ksizeParam.DisplayName = "均值核大小 (KSize)";
    ksizeParam.Type = ParamType::IntSlider;
    ksizeParam.Value = 5;
    ksizeParam.MinValue = 1;
    ksizeParam.MaxValue = 31;
    ksizeParam.Step = 1;
    _params.push_back(ksizeParam);
}

std::string BlurOp::GetPrincipleDoc() const
{
    return "### 📘 算子原理：均值滤波 (blur / boxFilter)\n\n"
           "每个像素点的输出为邻域窗口内所有像素的算术平均值：\n"
           "$$K = \\frac{1}{ksize \\times ksize} \\begin{bmatrix} 1 & \\dots & 1 \\\\ \\vdots & \\ddots & \\vdots \\\\ 1 & \\dots & 1 \\end{bmatrix}$$\n"
           "**特点**：运算速度快，但会造成边缘细节模糊。";
}

bool BlurOp::Process(const cv::Mat& input, cv::Mat& output, std::string& errorMsg)
{
    if (input.empty()) { errorMsg = "输入图像为空"; return false; }
    int ksize = _params[0].GetInt();
    if (ksize < 1) ksize = 1;

    try
    {
        cv::blur(input, output, cv::Size(ksize, ksize));
        return true;
    }
    catch (const cv::Exception& e)
    {
        errorMsg = e.what();
        return false;
    }
}

std::string BlurOp::GenerateCppCode(const std::string& inVar, const std::string& outVar) const
{
    int ksize = _params[0].GetInt();
    if (ksize < 1) ksize = 1;

    std::stringstream ss;
    ss << "// 均值滤波参数配置\n"
       << "int ksize = " << ksize << ";             // 均值核尺寸\n"
       << "cv::blur(" << inVar << ", " << outVar << ", cv::Size(ksize, ksize));";
    return ss.str();
}

std::string BlurOp::GeneratePythonCode(const std::string& inVar, const std::string& outVar) const
{
    int ksize = _params[0].GetInt();
    if (ksize < 1) ksize = 1;

    std::stringstream ss;
    ss << "# 均值滤波参数配置\n"
       << "ksize = (" << ksize << ", " << ksize << ")         # 均值核尺寸\n"
       << outVar << " = cv2.blur(" << inVar << ", ksize)";
    return ss.str();
}

// ---------------- MedianBlurOp ----------------
MedianBlurOp::MedianBlurOp()
{
    OpParameter ksizeParam;
    ksizeParam.Name = "ksize";
    ksizeParam.DisplayName = "中值核大小 (KSize)";
    ksizeParam.Type = ParamType::IntSlider;
    ksizeParam.Value = 5;
    ksizeParam.MinValue = 1;
    ksizeParam.MaxValue = 31;
    ksizeParam.Step = 2;
    _params.push_back(ksizeParam);
}

std::string MedianBlurOp::GetPrincipleDoc() const
{
    return "### 📘 算子原理：中值滤波 (medianBlur)\n\n"
           "非线性滤波技术，取邻域所有像素灰度值排序后的中间值作为输出。\n"
           "**特别擅长**：去除椒盐噪声 (Salt & Pepper Noise) 且有效保护边缘不被过度模糊。";
}

bool MedianBlurOp::Process(const cv::Mat& input, cv::Mat& output, std::string& errorMsg)
{
    if (input.empty()) { errorMsg = "输入图像为空"; return false; }
    int ksize = _params[0].GetInt();
    if (ksize % 2 == 0) ksize += 1;

    try
    {
        cv::medianBlur(input, output, ksize);
        return true;
    }
    catch (const cv::Exception& e)
    {
        errorMsg = e.what();
        return false;
    }
}

std::string MedianBlurOp::GenerateCppCode(const std::string& inVar, const std::string& outVar) const
{
    int ksize = _params[0].GetInt();
    if (ksize % 2 == 0) ksize += 1;

    std::stringstream ss;
    ss << "// 中值滤波参数配置 (去除椒盐噪声)\n"
       << "int ksize = " << ksize << ";             // 窗口孔径大小 (奇数)\n"
       << "cv::medianBlur(" << inVar << ", " << outVar << ", ksize);";
    return ss.str();
}

std::string MedianBlurOp::GeneratePythonCode(const std::string& inVar, const std::string& outVar) const
{
    int ksize = _params[0].GetInt();
    if (ksize % 2 == 0) ksize += 1;

    std::stringstream ss;
    ss << "# 中值滤波参数配置 (去除椒盐噪声)\n"
       << "ksize = " << ksize << "                 # 窗口孔径大小 (奇数)\n"
       << outVar << " = cv2.medianBlur(" << inVar << ", ksize)";
    return ss.str();
}

// ---------------- BilateralFilterOp ----------------
BilateralFilterOp::BilateralFilterOp()
{
    OpParameter dParam;
    dParam.Name = "d";
    dParam.DisplayName = "邻域直径 (d)";
    dParam.Type = ParamType::IntSlider;
    dParam.Value = 9;
    dParam.MinValue = 1;
    dParam.MaxValue = 25;
    dParam.Step = 2;
    _params.push_back(dParam);

    OpParameter scParam;
    scParam.Name = "sigmaColor";
    scParam.DisplayName = "颜色空间方差 (SigmaColor)";
    scParam.Type = ParamType::DoubleSlider;
    scParam.Value = 75.0;
    scParam.MinValue = 10.0;
    scParam.MaxValue = 200.0;
    scParam.Step = 5.0;
    _params.push_back(scParam);

    OpParameter ssParam;
    ssParam.Name = "sigmaSpace";
    ssParam.DisplayName = "坐标空间方差 (SigmaSpace)";
    ssParam.Type = ParamType::DoubleSlider;
    ssParam.Value = 75.0;
    ssParam.MinValue = 10.0;
    ssParam.MaxValue = 200.0;
    ssParam.Step = 5.0;
    _params.push_back(ssParam);
}

std::string BilateralFilterOp::GetPrincipleDoc() const
{
    return "### 📘 算子原理：双边滤波 (bilateralFilter)\n\n"
           "双边滤波结合了**空间邻近度**和**像素值相似度**双重高斯核：\n"
           "* 平坦区域进行平滑降噪；\n"
           "* 边缘区域由于像素差值大，权重趋向于 0，因此能完美**保留边缘结构**（磨皮美颜算法核心基础）。";
}

bool BilateralFilterOp::Process(const cv::Mat& input, cv::Mat& output, std::string& errorMsg)
{
    if (input.empty()) { errorMsg = "输入图像为空"; return false; }
    int d = _params[0].GetInt();
    double sigmaColor = _params[1].GetDouble();
    double sigmaSpace = _params[2].GetDouble();

    try
    {
        cv::bilateralFilter(input, output, d, sigmaColor, sigmaSpace);
        return true;
    }
    catch (const cv::Exception& e)
    {
        errorMsg = e.what();
        return false;
    }
}

std::string BilateralFilterOp::GenerateCppCode(const std::string& inVar, const std::string& outVar) const
{
    int d = _params[0].GetInt();
    double sigmaColor = _params[1].GetDouble();
    double sigmaSpace = _params[2].GetDouble();

    std::stringstream ss;
    ss << "// 双边滤波 (保边去噪) 参数配置\n"
       << "int d = " << d << ";                  // 滤波邻域直径\n"
       << "double sigmaColor = " << sigmaColor << ";    // 颜色相似度权重方差\n"
       << "double sigmaSpace = " << sigmaSpace << ";    // 空间距离权重方差\n"
       << "cv::bilateralFilter(" << inVar << ", " << outVar << ", d, sigmaColor, sigmaSpace);";
    return ss.str();
}

std::string BilateralFilterOp::GeneratePythonCode(const std::string& inVar, const std::string& outVar) const
{
    int d = _params[0].GetInt();
    double sigmaColor = _params[1].GetDouble();
    double sigmaSpace = _params[2].GetDouble();

    std::stringstream ss;
    ss << "# 双边滤波 (保边去噪) 参数配置\n"
       << "d = " << d << "                      # 滤波邻域直径\n"
       << "sigma_color = " << sigmaColor << "          # 颜色相似度权重方差\n"
       << "sigma_space = " << sigmaSpace << "          # 空间距离权重方差\n"
       << outVar << " = cv2.bilateralFilter(" << inVar << ", d, sigma_color, sigma_space)";
    return ss.str();
}

// ---------------- Filter2DOp ----------------
Filter2DOp::Filter2DOp()
{
    OpParameter typeParam;
    typeParam.Name = "type";
    typeParam.DisplayName = "卷积核预设 (Kernel Type)";
    typeParam.Type = ParamType::ComboBox;
    typeParam.Value = 0;
    typeParam.EnumOptions = {
        "锐化核 (Sharpen 3x3)",
        "浮雕效果 (Emboss 3x3)",
        "高通滤波 (High-pass 3x3)",
        "边缘增强 (Edge Enhance 3x3)"
    };
    _params.push_back(typeParam);
}

std::string Filter2DOp::GetPrincipleDoc() const
{
    return "### 📘 算子原理：自定义 2D 卷积滤波 (filter2D)\n\n"
           "通过自定义 $3 \\times 3$ 卷积核与图像做二维互相关运算：\n"
           "$$dst(x, y) = \\sum_{i} \\sum_{j} kernel(i, j) \\cdot src(x+i-anchor.x, y+j-anchor.y)$$";
}

bool Filter2DOp::Process(const cv::Mat& input, cv::Mat& output, std::string& errorMsg)
{
    if (input.empty()) { errorMsg = "输入图像为空"; return false; }
    int type = _params[0].GetInt();

    cv::Mat kernel;
    if (type == 0) // Sharpen
    {
        kernel = (cv::Mat_<float>(3, 3) << 0, -1, 0, -1, 5, -1, 0, -1, 0);
    }
    else if (type == 1) // Emboss
    {
        kernel = (cv::Mat_<float>(3, 3) << -2, -1, 0, -1, 1, 1, 0, 1, 2);
    }
    else if (type == 2) // Highpass
    {
        kernel = (cv::Mat_<float>(3, 3) << -1, -1, -1, -1, 8, -1, -1, -1, -1);
    }
    else // Edge enhance
    {
        kernel = (cv::Mat_<float>(3, 3) << 0, 0, 0, -1, 1, 0, 0, 0, 0);
    }

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

std::string Filter2DOp::GenerateCppCode(const std::string& inVar, const std::string& outVar) const
{
    int type = _params[0].GetInt();
    std::string kInit = "0, -1, 0, -1, 5, -1, 0, -1, 0";
    std::string desc = "锐化卷积核 (Sharpen 3x3)";
    if (type == 1) { kInit = "-2, -1, 0, -1, 1, 1, 0, 1, 2"; desc = "浮雕卷积核 (Emboss 3x3)"; }
    else if (type == 2) { kInit = "-1, -1, -1, -1, 8, -1, -1, -1, -1"; desc = "高通卷积核 (High-pass 3x3)"; }
    else if (type == 3) { kInit = "0, 0, 0, -1, 1, 0, 0, 0, 0"; desc = "边缘增强卷积核 (Edge Enhance)"; }

    std::stringstream ss;
    ss << "// 2D自定义卷积参数: " << desc << "\n"
       << "cv::Mat kernel = (cv::Mat_<float>(3, 3) << " << kInit << ");\n"
       << "cv::filter2D(" << inVar << ", " << outVar << ", -1, kernel);";
    return ss.str();
}

std::string Filter2DOp::GeneratePythonCode(const std::string& inVar, const std::string& outVar) const
{
    int type = _params[0].GetInt();
    std::string kArr = "[[0, -1, 0], [-1, 5, -1], [0, -1, 0]]";
    std::string desc = "锐化卷积核 (Sharpen 3x3)";
    if (type == 1) { kArr = "[[-2, -1, 0], [-1, 1, 1], [0, 1, 2]]"; desc = "浮雕卷积核 (Emboss 3x3)"; }
    else if (type == 2) { kArr = "[[-1, -1, -1], [-1, 8, -1], [-1, -1, -1]]"; desc = "高通卷积核 (High-pass 3x3)"; }
    else if (type == 3) { kArr = "[[0, 0, 0], [-1, 1, 0], [0, 0, 0]]"; desc = "边缘增强卷积核"; }

    std::stringstream ss;
    ss << "# 2D自定义卷积参数: " << desc << "\n"
       << "kernel = np.array(" << kArr << ", dtype=np.float32)\n"
       << outVar << " = cv2.filter2D(" << inVar << ", -1, kernel)";
    return ss.str();
}
