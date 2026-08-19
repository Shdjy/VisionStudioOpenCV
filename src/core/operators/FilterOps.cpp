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
           "**【核心思想】**\n"
           "高斯滤波是一种线性平滑滤波，利用二维高斯正态分布权重对邻域像素进行加权平均。距离核中心越近的像素分配更高的权重，能有效滤除高斯随机白噪声并最大程度保留边缘轮廓。\n\n"
           "**【数学模型 / 运算公式】**\n"
           "```\n"
           "                    1               -(x² + y²) / (2·σ²)\n"
           "   G(x, y)  =  ────────────  ·  e\n"
           "                 2 · π · σ²\n"
           "```\n\n"
           "**【符号与参数解析】**\n"
           "- **(x, y)**：相对于滤波核中心的邻域像素空间相对坐标偏移量；\n"
           "- **σ (sigmaX)**：高斯核的标准差，控制分布曲线的平缓程度与模糊扩散半径；\n"
           "- **KSize (2r + 1)**：滤波窗口核尺寸，必须为正奇数（如 3×3, 5×5）。\n\n"
           "**【工程应用建议】**\n"
           "- 噪声滤除：常作为 Canny 边缘检测、阈值分割或特征提取的前置预处理步骤；\n"
           "- 模糊调优：增大核尺寸与 σ 会加剧平滑模糊程度，需在降噪与边缘清晰度之间权衡。";
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
           "**【核心思想】**\n"
           "均值滤波是一种线性低通空间滤波技术，将目标像素替换为其矩形邻域窗口内所有像素灰度值的算术平均值。\n\n"
           "**【数学模型 / 运算公式】**\n"
           "```\n"
           "                 1          k/2     k/2\n"
           "   dst(x, y) = ───────  ·   ∑       ∑     src(x + i, y + j)\n"
           "                ksize²    i=-k/2  j=-k/2\n"
           "```\n\n"
           "**【符号与参数解析】**\n"
           "- **src(x, y)**：原始输入图像在该坐标处的像素灰度值；\n"
           "- **dst(x, y)**：滤波计算后输出图像的像素灰度值；\n"
           "- **ksize**：滑动滤波窗口边长（如 3×3, 5×5）。\n\n"
           "**【工程应用建议】**\n"
           "- 适用：快速均匀消除微弱高频背景噪点，计算效率极高；\n"
           "- 局限：对图像强边缘轮廓破坏较大，易造成物体轮廓发虚模糊。";
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
           "**【核心思想】**\n"
           "中值滤波是一种典型的非线性统计排序滤波技术，以邻域像素灰度值的统计中位数（Median）取代中心像素值，不依赖代数加权。\n\n"
           "**【数学模型 / 运算公式】**\n"
           "```\n"
           "   dst(x, y) = Median { src(x + i, y + j) | (i, j) ∈ Neighborhood(ksize) }\n"
           "```\n\n"
           "**【符号与参数解析】**\n"
           "- **Median { ... }**：将滤波窗口内 ksize×ksize 个像素按灰度升序排序后取中间值；\n"
           "- **ksize**：邻域孔径边长，必须为正奇数（如 3, 5, 7）。\n\n"
           "**【工程应用建议】**\n"
           "- 椒盐降噪王：工业视觉中消除传感器坏点、粉尘反光颗粒、脉冲式孤立噪点的首选；\n"
           "- 边缘保护优：相比均值滤波，能在去噪的同时保持阶跃边缘不产生弥散模糊。";
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
           "**【核心思想】**\n"
           "双边滤波是一种结合了【空间几何邻近度】与【像素值灰度相似度】的双高斯非线性保边滤波，平坦区域充分平滑，边缘处权重衰减为 0 从而无损保留边界清晰度。\n\n"
           "**【数学模型 / 运算公式】**\n"
           "```\n"
           "                 1\n"
           "   dst(p) = ─────────── · ∑  src(q) · G_space(||p - q||) · G_color(|src(p) - src(q)|)\n"
           "             W_norm(p)   q∈Ω\n"
           "\n"
           "   其中: G_space(Δd) = exp(-Δd² / (2·σ_space²))\n"
           "         G_color(ΔI) = exp(-ΔI² / (2·σ_color²))\n"
           "```\n\n"
           "**【符号与参数解析】**\n"
           "- **d**：滤波过程中每个像素邻域的直径大小；\n"
           "- **sigmaColor**：灰度相似度权重高斯核的标准差，值越大允许更大色差混合；\n"
           "- **sigmaSpace**：空间几何距离权重高斯核的标准差，值越大影响范围越广。\n\n"
           "**【工程应用建议】**\n"
           "- 保边降噪首选：工件表面缺陷检测前置去噪、人脸磨皮等同时需要平滑背景与锁定边缘轮廓的场景。";
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
    return "### 📘 算子原理：自定义 2D 空间卷积滤波 (filter2D)\n\n"
           "**【核心思想】**\n"
           "通过预定义或自定义权重矩阵（卷积核 Kernel）与原始图像进行二维互相关/卷积运算，实现锐化、浮雕、高通滤波及边缘增强等定制化特征提取。\n\n"
           "**【数学模型 / 运算公式】**\n"
           "```\n"
           "                 k_h-1  k_w-1\n"
           "   dst(x, y) =    ∑      ∑    kernel(i, j) · src(x + i - anchor.x, y + j - anchor.y)\n"
           "                 i=0    j=0\n"
           "```\n\n"
           "**【常用典型核矩阵】**\n"
           "```\n"
           "   锐化核(Sharpen)         高通核(High-pass)         浮雕核(Emboss)\n"
           "   ┌  0  -1   0 ┐          ┌ -1  -1  -1 ┐           ┌ -2  -1   0 ┐\n"
           "   │ -1   5  -1 │          │ -1   8  -1 │           │ -1   1   1 │\n"
           "   └  0  -1   0 ┘          └ -1  -1  -1 ┘           └  0   1   2 ┘\n"
           "```\n\n"
           "**【符号与参数解析】**\n"
           "- **kernel(i, j)**：卷积核各点权重系数矩阵；\n"
           "- **anchor**：卷积核锚点坐标，默认位于几何中心 (1, 1)。";
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
