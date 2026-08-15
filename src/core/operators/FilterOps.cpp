#include "FilterOps.h"
#include <sstream>

// ---------------- GaussianBlurOp ----------------
GaussianBlurOp::GaussianBlurOp()
{
    OpParameter ksizeParam;
    ksizeParam.Name = "ksize";
    ksizeParam.DisplayName = "核大小 (Ksize)";
    ksizeParam.Tooltip = "高斯核尺寸 (必须为奇数，如 3, 5, 7, 9...)";
    ksizeParam.Type = ParamType::IntSlider;
    ksizeParam.Value = 5;
    ksizeParam.MinValue = 1;
    ksizeParam.MaxValue = 31;
    ksizeParam.Step = 2;
    _params.push_back(ksizeParam);

    OpParameter sigmaParam;
    sigmaParam.Name = "sigmaX";
    sigmaParam.DisplayName = "标准差 (SigmaX)";
    sigmaParam.Tooltip = "X方向高斯核标准差 (0 表示由核大小自动计算)";
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
           "高斯滤波是一种线性平滑滤波，其权重服从二维高斯正态分布：\n"
           "$$G(x, y) = \\frac{1}{2\\pi\\sigma^2} e^{-\\frac{x^2+y^2}{2\\sigma^2}}$$\n\n"
           "**参数指南**：\n"
           "* `ksize`：核大小，必须为**正奇数**。核越大，模糊程度越高。\n"
           "* `sigmaX`：标准差，决定高斯曲线的胖瘦。若设为 0，OpenCV 会根据公式 $\\sigma = 0.3 \\times ((ksize-1)\\times 0.5 - 1) + 0.8$ 自动估算。";
}

bool GaussianBlurOp::Process(const cv::Mat& input, cv::Mat& output, std::string& errorMsg)
{
    if (input.empty())
    {
        errorMsg = "输入图像为空";
        return false;
    }

    int ksize = _params[0].GetInt();
    if (ksize % 2 == 0) { ksize += 1; }
    if (ksize < 1) { ksize = 1; }
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
    if (ksize % 2 == 0) { ksize += 1; }
    double sigmaX = _params[1].GetDouble();

    std::stringstream ss;
    ss << "cv::GaussianBlur(" << inVar << ", " << outVar << ", cv::Size(" << ksize << ", " << ksize << "), " << sigmaX << ");";
    return ss.str();
}

std::string GaussianBlurOp::GeneratePythonCode(const std::string& inVar, const std::string& outVar) const
{
    int ksize = _params[0].GetInt();
    if (ksize % 2 == 0) { ksize += 1; }
    double sigmaX = _params[1].GetDouble();

    std::stringstream ss;
    ss << outVar << " = cv2.GaussianBlur(" << inVar << ", (" << ksize << ", " << ksize << "), " << sigmaX << ")";
    return ss.str();
}

// ---------------- BlurOp ----------------
BlurOp::BlurOp()
{
    OpParameter ksizeParam;
    ksizeParam.Name = "ksize";
    ksizeParam.DisplayName = "核大小 (Ksize)";
    ksizeParam.Tooltip = "均值核尺寸 (正整数，如 3, 5, 7...)";
    ksizeParam.Type = ParamType::IntSlider;
    ksizeParam.Value = 5;
    ksizeParam.MinValue = 1;
    ksizeParam.MaxValue = 31;
    ksizeParam.Step = 1;
    _params.push_back(ksizeParam);
}

std::string BlurOp::GetPrincipleDoc() const
{
    return "### 📘 算子原理：均值滤波 (blur)\n\n"
           "均值滤波使用归一化矩形核进行卷积，核内所有像素权重均等：\n"
           "$$K = \\frac{1}{K_{width} \\times K_{height}} \\begin{bmatrix} 1 & 1 & \\dots & 1 \\\\ \\vdots & \\vdots & \\ddots & \\vdots \\\\ 1 & 1 & \\dots & 1 \\end{bmatrix}$$\n"
           "主要用于快速消除图像中的颗粒噪声，但对边缘细节模糊较明显。";
}

bool BlurOp::Process(const cv::Mat& input, cv::Mat& output, std::string& errorMsg)
{
    if (input.empty())
    {
        errorMsg = "输入图像为空";
        return false;
    }

    int ksize = _params[0].GetInt();
    if (ksize < 1) { ksize = 1; }

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
    std::stringstream ss;
    ss << "cv::blur(" << inVar << ", " << outVar << ", cv::Size(" << ksize << ", " << ksize << "));";
    return ss.str();
}

std::string BlurOp::GeneratePythonCode(const std::string& inVar, const std::string& outVar) const
{
    int ksize = _params[0].GetInt();
    std::stringstream ss;
    ss << outVar << " = cv2.blur(" << inVar << ", (" << ksize << ", " << ksize << "))";
    return ss.str();
}

// ---------------- MedianBlurOp ----------------
MedianBlurOp::MedianBlurOp()
{
    OpParameter ksizeParam;
    ksizeParam.Name = "ksize";
    ksizeParam.DisplayName = "核大小 (Ksize)";
    ksizeParam.Tooltip = "中值滤波孔径线性尺寸 (必须为奇数且大于1)";
    ksizeParam.Type = ParamType::IntSlider;
    ksizeParam.Value = 5;
    ksizeParam.MinValue = 3;
    ksizeParam.MaxValue = 31;
    ksizeParam.Step = 2;
    _params.push_back(ksizeParam);
}

std::string MedianBlurOp::GetPrincipleDoc() const
{
    return "### 📘 算子原理：中值滤波 (medianBlur)\n\n"
           "中值滤波是一种典型的**非线性滤波**技术。它将像素邻域窗口内的所有灰度值排序，取中间值作为输出。\n"
           "**核心优势**：在消除**椒盐噪声 (Salt-and-Pepper Noise)** 时极其有效，且比均值滤波更能保护边缘轮廓。";
}

bool MedianBlurOp::Process(const cv::Mat& input, cv::Mat& output, std::string& errorMsg)
{
    if (input.empty())
    {
        errorMsg = "输入图像为空";
        return false;
    }

    int ksize = _params[0].GetInt();
    if (ksize % 2 == 0) { ksize += 1; }
    if (ksize < 3) { ksize = 3; }

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
    if (ksize % 2 == 0) { ksize += 1; }
    std::stringstream ss;
    ss << "cv::medianBlur(" << inVar << ", " << outVar << ", " << ksize << ");";
    return ss.str();
}

std::string MedianBlurOp::GeneratePythonCode(const std::string& inVar, const std::string& outVar) const
{
    int ksize = _params[0].GetInt();
    if (ksize % 2 == 0) { ksize += 1; }
    std::stringstream ss;
    ss << outVar << " = cv2.medianBlur(" << inVar << ", " << ksize << ")";
    return ss.str();
}

// ---------------- BilateralFilterOp ----------------
BilateralFilterOp::BilateralFilterOp()
{
    OpParameter dParam;
    dParam.Name = "d";
    dParam.DisplayName = "邻域直径 (d)";
    dParam.Tooltip = "过滤期间使用的每个像素邻域的直径 (若为负数则由 sigmaSpace 决定)";
    dParam.Type = ParamType::IntSlider;
    dParam.Value = 9;
    dParam.MinValue = 1;
    dParam.MaxValue = 25;
    dParam.Step = 2;
    _params.push_back(dParam);

    OpParameter sigmaColorParam;
    sigmaColorParam.Name = "sigmaColor";
    sigmaColorParam.DisplayName = "色彩标准差 (SigmaColor)";
    sigmaColorParam.Tooltip = "颜色空间中的过滤标准差。值越大，更宽泛的颜色将被混合";
    sigmaColorParam.Type = ParamType::DoubleSlider;
    sigmaColorParam.Value = 75.0;
    sigmaColorParam.MinValue = 10.0;
    sigmaColorParam.MaxValue = 200.0;
    sigmaColorParam.Step = 5.0;
    _params.push_back(sigmaColorParam);

    OpParameter sigmaSpaceParam;
    sigmaSpaceParam.Name = "sigmaSpace";
    sigmaSpaceParam.DisplayName = "坐标空间标准差 (SigmaSpace)";
    sigmaSpaceParam.Tooltip = "坐标空间中的过滤标准差。值越大，越远的像素相互影响";
    sigmaSpaceParam.Type = ParamType::DoubleSlider;
    sigmaSpaceParam.Value = 75.0;
    sigmaSpaceParam.MinValue = 10.0;
    sigmaSpaceParam.MaxValue = 200.0;
    sigmaSpaceParam.Step = 5.0;
    _params.push_back(sigmaSpaceParam);
}

std::string BilateralFilterOp::GetPrincipleDoc() const
{
    return "### 📘 算子原理：双边滤波 (bilateralFilter)\n\n"
           "双边滤波是一种结合**空间距离**与**像素色彩相似度**的高阶非线性滤波：\n"
           "* **空间权重**：距离中心越近，权重越大；\n"
           "* **值域权重**：与中心像素颜色越接近，权重越大。\n\n"
           "**效果**：在平滑平坦区域噪声的同时，由于边缘两侧颜色差异大导致值域权重接近0，因此**完美保留边缘陡峭度**（美颜磨皮经典算法）。";
}

bool BilateralFilterOp::Process(const cv::Mat& input, cv::Mat& output, std::string& errorMsg)
{
    if (input.empty())
    {
        errorMsg = "输入图像为空";
        return false;
    }

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
    ss << "cv::bilateralFilter(" << inVar << ", " << outVar << ", " << d << ", " << sigmaColor << ", " << sigmaSpace << ");";
    return ss.str();
}

std::string BilateralFilterOp::GeneratePythonCode(const std::string& inVar, const std::string& outVar) const
{
    int d = _params[0].GetInt();
    double sigmaColor = _params[1].GetDouble();
    double sigmaSpace = _params[2].GetDouble();

    std::stringstream ss;
    ss << outVar << " = cv2.bilateralFilter(" << inVar << ", " << d << ", " << sigmaColor << ", " << sigmaSpace << ")";
    return ss.str();
}

// ---------------- Filter2DOp ----------------
Filter2DOp::Filter2DOp()
{
    OpParameter presetParam;
    presetParam.Name = "preset";
    presetParam.DisplayName = "卷积核预设 (Kernel Preset)";
    presetParam.Tooltip = "选择常用的 3x3 自定义滤波核";
    presetParam.Type = ParamType::ComboBox;
    presetParam.Value = 0;
    presetParam.EnumOptions = {
        "锐化核 (Sharpen)",
        "浮雕核 (Emboss)",
        "边缘增强核 (Edge Enhance)",
        "高通滤波核 (High-Pass)"
    };
    _params.push_back(presetParam);
}

std::string Filter2DOp::GetPrincipleDoc() const
{
    return "### 📘 算子原理：2D 自定义卷积核 (filter2D)\n\n"
           "对图像执行任意 2D 矩阵互相关运算：\n"
           "$$\\text{dst}(x, y) = \\sum_{x', y'} K(x', y') \\cdot \\text{src}(x+x', y+y')$$\n"
           "内置锐化核、浮雕效果、边缘提取等经典图像处理矩阵。";
}

bool Filter2DOp::Process(const cv::Mat& input, cv::Mat& output, std::string& errorMsg)
{
    if (input.empty())
    {
        errorMsg = "输入图像为空";
        return false;
    }

    int preset = _params[0].GetInt();
    cv::Mat kernel;

    switch (preset)
    {
        case 0: // 锐化
            kernel = (cv::Mat_<float>(3, 3) <<  0, -1,  0,
                                               -1,  5, -1,
                                                0, -1,  0);
            break;
        case 1: // 浮雕
            kernel = (cv::Mat_<float>(3, 3) << -2, -1,  0,
                                               -1,  1,  1,
                                                0,  1,  2);
            break;
        case 2: // 边缘增强
            kernel = (cv::Mat_<float>(3, 3) << -1, -1, -1,
                                               -1,  9, -1,
                                               -1, -1, -1);
            break;
        case 3: // 高通
            kernel = (cv::Mat_<float>(3, 3) << -1, -1, -1,
                                               -1,  8, -1,
                                               -1, -1, -1);
            break;
        default:
            kernel = cv::Mat::eye(3, 3, CV_32F);
            break;
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
    int preset = _params[0].GetInt();
    std::string kStr = "cv::Mat kernel = (cv::Mat_<float>(3,3) << 0,-1,0, -1,5,-1, 0,-1,0);";
    if (preset == 1) kStr = "cv::Mat kernel = (cv::Mat_<float>(3,3) << -2,-1,0, -1,1,1, 0,1,2);";
    else if (preset == 2) kStr = "cv::Mat kernel = (cv::Mat_<float>(3,3) << -1,-1,-1, -1,9,-1, -1,-1,-1);";
    else if (preset == 3) kStr = "cv::Mat kernel = (cv::Mat_<float>(3,3) << -1,-1,-1, -1,8,-1, -1,-1,-1);";

    std::stringstream ss;
    ss << kStr << "\n"
       << "cv::filter2D(" << inVar << ", " << outVar << ", -1, kernel);";
    return ss.str();
}

std::string Filter2DOp::GeneratePythonCode(const std::string& inVar, const std::string& outVar) const
{
    int preset = _params[0].GetInt();
    std::string kStr = "kernel = np.array([[0, -1, 0], [-1, 5, -1], [0, -1, 0]], dtype=np.float32)";
    if (preset == 1) kStr = "kernel = np.array([[-2, -1, 0], [-1, 1, 1], [0, 1, 2]], dtype=np.float32)";
    else if (preset == 2) kStr = "kernel = np.array([[-1, -1, -1], [-1, 9, -1], [-1, -1, -1]], dtype=np.float32)";
    else if (preset == 3) kStr = "kernel = np.array([[-1, -1, -1], [-1, 8, -1], [-1, -1, -1]], dtype=np.float32)";

    std::stringstream ss;
    ss << kStr << "\n"
       << outVar << " = cv2.filter2D(" << inVar << ", -1, kernel)";
    return ss.str();
}
