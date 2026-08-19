#include "EdgeOps.h"
#include <sstream>

// ---------------- CannyOp ----------------
CannyOp::CannyOp()
{
    OpParameter t1Param;
    t1Param.Name = "threshold1";
    t1Param.DisplayName = "低阈值 (Threshold1)";
    t1Param.Type = ParamType::DoubleSlider;
    t1Param.Value = 50.0;
    t1Param.MinValue = 0.0;
    t1Param.MaxValue = 500.0;
    t1Param.Step = 1.0;
    _params.push_back(t1Param);

    OpParameter t2Param;
    t2Param.Name = "threshold2";
    t2Param.DisplayName = "高阈值 (Threshold2)";
    t2Param.Type = ParamType::DoubleSlider;
    t2Param.Value = 150.0;
    t2Param.MinValue = 0.0;
    t2Param.MaxValue = 500.0;
    t2Param.Step = 1.0;
    _params.push_back(t2Param);

    OpParameter apParam;
    apParam.Name = "apertureSize";
    apParam.DisplayName = "Sobel核孔径 (ApertureSize)";
    apParam.Type = ParamType::IntSlider;
    apParam.Value = 3;
    apParam.MinValue = 3;
    apParam.MaxValue = 7;
    apParam.Step = 2;
    _params.push_back(apParam);

    OpParameter l2Param;
    l2Param.Name = "L2gradient";
    l2Param.DisplayName = "精确L2范数 (L2gradient)";
    l2Param.Type = ParamType::CheckBox;
    l2Param.Value = false;
    _params.push_back(l2Param);
}

std::string CannyOp::GetPrincipleDoc() const
{
    return "### 📘 算子原理：Canny 多阶段双阈值边缘检测 (Canny)\n\n"
           "**【核心思想】**\n"
           "Canny 是工业界应用最广泛的经典最优边缘检测算法，满足低错误率、高定位精度和单边缘响应三大准则。通过梯度极值点非极大值抑制结合双阈值滞后跟踪，有效消除伪边缘与噪点。\n\n"
           "**【四步计算流程与数学模型】**\n"
           "```\n"
           "   1. 高斯平滑:  I_smooth = I * G(σ)\n"
           "   2. 梯度计算:  Gx = ∂I/∂x,  Gy = ∂I/∂y\n"
           "                 幅值 |M| = √(Gx² + Gy²),  方向 θ = arctan(Gy / Gx)\n"
           "   3. 非极大值抑制 (NMS): 沿梯度法线方向进行局部最大值插值比较，抑制非极大值点\n"
           "   4. 双阈值滞后跟踪 (Hysteresis):\n"
           "        ┌ |M| ≥ threshold2  ──> 强边缘 (绝对保留)\n"
           "        ├ |M| < threshold1  ──> 噪点 (彻底抑制)\n"
           "        └ threshold1 ≤ |M| < threshold2 ──> 若与强边缘 8-连通则保留，否则舍弃\n"
           "```\n\n"
           "**【符号与参数解析】**\n"
           "- **threshold1 (低阈值)**：弱边缘连接下限门限；\n"
           "- **threshold2 (高阈值)**：强边缘判定触发上限门限（工业推荐比例约为 1:2 ~ 1:3）；\n"
           "- **apertureSize**：用于求导的 Sobel 核窗口孔径（通常选 3）；\n"
           "- **L2gradient**：是否启用精确欧氏范数 √(Gx²+Gy²)，若为 false 则采用近似绝对值 |Gx|+|Gy| 加快速度。";
}

bool CannyOp::Process(const cv::Mat& input, cv::Mat& output, std::string& errorMsg)
{
    if (input.empty()) { errorMsg = "输入图像为空"; return false; }
    cv::Mat gray = input;
    if (input.channels() > 1)
    {
        cv::cvtColor(input, gray, cv::COLOR_BGR2GRAY);
    }

    double t1 = _params[0].GetDouble();
    double t2 = _params[1].GetDouble();
    int ap = _params[2].GetInt();
    if (ap % 2 == 0) ap += 1;
    bool l2 = _params[3].GetBool();

    try
    {
        cv::Canny(gray, output, t1, t2, ap, l2);
        return true;
    }
    catch (const cv::Exception& e)
    {
        errorMsg = e.what();
        return false;
    }
}

std::string CannyOp::GenerateCppCode(const std::string& inVar, const std::string& outVar) const
{
    double t1 = _params[0].GetDouble();
    double t2 = _params[1].GetDouble();
    int ap = _params[2].GetInt();
    if (ap % 2 == 0) ap += 1;
    bool l2 = _params[3].GetBool();

    std::stringstream ss;
    ss << "// Canny双阈值边缘检测参数配置\n"
       << "double threshold1 = " << t1 << ";         // 低阈值 (滞后连接)\n"
       << "double threshold2 = " << t2 << ";        // 高阈值 (强边缘判定)\n"
       << "int apertureSize = " << ap << ";             // Sobel核孔径大小\n"
       << "bool L2gradient = " << (l2 ? "true" : "false") << ";           // 精确L2梯度模长\n"
       << "cv::Canny(" << inVar << ", " << outVar << ", threshold1, threshold2, apertureSize, L2gradient);";
    return ss.str();
}

std::string CannyOp::GeneratePythonCode(const std::string& inVar, const std::string& outVar) const
{
    double t1 = _params[0].GetDouble();
    double t2 = _params[1].GetDouble();
    int ap = _params[2].GetInt();
    if (ap % 2 == 0) ap += 1;
    bool l2 = _params[3].GetBool();

    std::stringstream ss;
    ss << "# Canny双阈值边缘检测参数配置\n"
       << "threshold1 = " << t1 << "              # 低阈值\n"
       << "threshold2 = " << t2 << "             # 高阈值\n"
       << "aperture_size = " << ap << "               # Sobel核孔径大小\n"
       << "l2_gradient = " << (l2 ? "True" : "False") << "\n"
       << outVar << " = cv2.Canny(" << inVar << ", threshold1, threshold2, apertureSize=aperture_size, L2gradient=l2_gradient)";
    return ss.str();
}

// ---------------- SobelOp ----------------
SobelOp::SobelOp()
{
    OpParameter dirParam;
    dirParam.Name = "direction";
    dirParam.DisplayName = "导数方向 (Direction)";
    dirParam.Type = ParamType::ComboBox;
    dirParam.Value = 2; // XY 合成
    dirParam.EnumOptions = { "X 方向导数 (垂直边缘)", "Y 方向导数 (水平边缘)", "XY 方向合成" };
    _params.push_back(dirParam);

    OpParameter ksizeParam;
    ksizeParam.Name = "ksize";
    ksizeParam.DisplayName = "核大小 (KSize)";
    ksizeParam.Type = ParamType::IntSlider;
    ksizeParam.Value = 3;
    ksizeParam.MinValue = 1;
    ksizeParam.MaxValue = 7;
    ksizeParam.Step = 2;
    _params.push_back(ksizeParam);

    OpParameter scaleParam;
    scaleParam.Name = "scale";
    scaleParam.DisplayName = "缩放比例 (Scale)";
    scaleParam.Type = ParamType::DoubleSlider;
    scaleParam.Value = 1.0;
    scaleParam.MinValue = 0.1;
    scaleParam.MaxValue = 5.0;
    scaleParam.Step = 0.1;
    _params.push_back(scaleParam);
}

std::string SobelOp::GetPrincipleDoc() const
{
    return "### 📘 算子原理：Sobel 一阶导数边缘检测 (Sobel)\n\n"
           "**【核心思想】**\n"
           "Sobel 算子将高斯加权平滑与一阶空间中心差分求导相结合，分别在水平与垂直方向对灰度变化剧烈的边缘轮廓进行差分增强，兼具抗噪性与边缘检测能力。\n\n"
           "**【数学模型 / 卷积核矩阵】**\n"
           "```\n"
           "   水平导数核 Gx (检测垂直边缘)        垂直导数核 Gy (检测水平边缘)\n"
           "        ┌ -1   0   1 ┐                     ┌ -1  -2  -1 ┐\n"
           "   Gx = │ -2   0   2 │ · I            Gy = │  0   0   0 │ · I\n"
           "        └ -1   0   1 ┘                     └  1   2   1 ┘\n"
           "\n"
           "   合成梯度幅值: G = √(Gx² + Gy²)  或近似快速值: G ≈ 0.5·|Gx| + 0.5·|Gy|\n"
           "```\n\n"
           "**【符号与参数解析】**\n"
           "- **direction**：计算 X 方向、Y 方向或 XY 双向合成梯度；\n"
           "- **ksize**：微分核尺寸（通常为 3, 5, 7），核越大抗噪越强但边缘定位精度稍降；\n"
           "- **scale**：输出梯度幅值的放大缩放因子。";
}

bool SobelOp::Process(const cv::Mat& input, cv::Mat& output, std::string& errorMsg)
{
    if (input.empty()) { errorMsg = "输入图像为空"; return false; }
    cv::Mat gray = input;
    if (input.channels() > 1) cv::cvtColor(input, gray, cv::COLOR_BGR2GRAY);

    int dir = _params[0].GetInt();
    int ksize = _params[1].GetInt();
    if (ksize % 2 == 0) ksize += 1;
    double scale = _params[2].GetDouble();

    try
    {
        if (dir == 0) // X
        {
            cv::Mat grad_x, abs_grad_x;
            cv::Sobel(gray, grad_x, CV_16S, 1, 0, ksize, scale);
            cv::convertScaleAbs(grad_x, output);
        }
        else if (dir == 1) // Y
        {
            cv::Mat grad_y, abs_grad_y;
            cv::Sobel(gray, grad_y, CV_16S, 0, 1, ksize, scale);
            cv::convertScaleAbs(grad_y, output);
        }
        else // XY
        {
            cv::Mat grad_x, grad_y, abs_grad_x, abs_grad_y;
            cv::Sobel(gray, grad_x, CV_16S, 1, 0, ksize, scale);
            cv::Sobel(gray, grad_y, CV_16S, 0, 1, ksize, scale);
            cv::convertScaleAbs(grad_x, abs_grad_x);
            cv::convertScaleAbs(grad_y, abs_grad_y);
            cv::addWeighted(abs_grad_x, 0.5, abs_grad_y, 0.5, 0, output);
        }
        return true;
    }
    catch (const cv::Exception& e)
    {
        errorMsg = e.what();
        return false;
    }
}

std::string SobelOp::GenerateCppCode(const std::string& inVar, const std::string& outVar) const
{
    int dir = _params[0].GetInt();
    int ksize = _params[1].GetInt();
    if (ksize % 2 == 0) ksize += 1;
    double scale = _params[2].GetDouble();

    std::stringstream ss;
    ss << "// Sobel一阶梯度边缘检测参数配置\n"
       << "int ksize = " << ksize << ";             // 核大小\n"
       << "double scale = " << scale << ";          // 梯度缩放比例\n";
    if (dir == 0)
    {
        ss << "cv::Mat gradX;\n"
           << "cv::Sobel(" << inVar << ", gradX, CV_16S, 1, 0, ksize, scale);\n"
           << "cv::convertScaleAbs(gradX, " << outVar << ");";
    }
    else if (dir == 1)
    {
        ss << "cv::Mat gradY;\n"
           << "cv::Sobel(" << inVar << ", gradY, CV_16S, 0, 1, ksize, scale);\n"
           << "cv::convertScaleAbs(gradY, " << outVar << ");";
    }
    else
    {
        ss << "cv::Mat gradX, gradY, absGradX, absGradY;\n"
           << "cv::Sobel(" << inVar << ", gradX, CV_16S, 1, 0, ksize, scale);\n"
           << "cv::Sobel(" << inVar << ", gradY, CV_16S, 0, 1, ksize, scale);\n"
           << "cv::convertScaleAbs(gradX, absGradX);\n"
           << "cv::convertScaleAbs(gradY, absGradY);\n"
           << "cv::addWeighted(absGradX, 0.5, absGradY, 0.5, 0, " << outVar << ");";
    }
    return ss.str();
}

std::string SobelOp::GeneratePythonCode(const std::string& inVar, const std::string& outVar) const
{
    int dir = _params[0].GetInt();
    int ksize = _params[1].GetInt();
    if (ksize % 2 == 0) ksize += 1;
    double scale = _params[2].GetDouble();

    std::stringstream ss;
    ss << "# Sobel一阶梯度边缘检测参数配置\n"
       << "ksize = " << ksize << "                 # 核大小\n"
       << "scale = " << scale << "                 # 梯度缩放比例\n";
    if (dir == 0)
    {
        ss << "grad_x = cv2.Sobel(" << inVar << ", cv2.CV_16S, 1, 0, ksize=ksize, scale=scale)\n"
           << outVar << " = cv2.convertScaleAbs(grad_x)";
    }
    else if (dir == 1)
    {
        ss << "grad_y = cv2.Sobel(" << inVar << ", cv2.CV_16S, 0, 1, ksize=ksize, scale=scale)\n"
           << outVar << " = cv2.convertScaleAbs(grad_y)";
    }
    else
    {
        ss << "grad_x = cv2.Sobel(" << inVar << ", cv2.CV_16S, 1, 0, ksize=ksize, scale=scale)\n"
           << "grad_y = cv2.Sobel(" << inVar << ", cv2.CV_16S, 0, 1, ksize=ksize, scale=scale)\n"
           << "abs_x = cv2.convertScaleAbs(grad_x)\n"
           << "abs_y = cv2.convertScaleAbs(grad_y)\n"
           << outVar << " = cv2.addWeighted(abs_x, 0.5, abs_y, 0.5, 0)";
    }
    return ss.str();
}

// ---------------- ScharrOp ----------------
ScharrOp::ScharrOp()
{
    OpParameter dirParam;
    dirParam.Name = "direction";
    dirParam.DisplayName = "导数方向 (Direction)";
    dirParam.Type = ParamType::ComboBox;
    dirParam.Value = 2; // XY
    dirParam.EnumOptions = { "X 方向导数", "Y 方向导数", "XY 方向合成" };
    _params.push_back(dirParam);

    OpParameter scaleParam;
    scaleParam.Name = "scale";
    scaleParam.DisplayName = "缩放比例 (Scale)";
    scaleParam.Type = ParamType::DoubleSlider;
    scaleParam.Value = 1.0;
    scaleParam.MinValue = 0.1;
    scaleParam.MaxValue = 5.0;
    scaleParam.Step = 0.1;
    _params.push_back(scaleParam);
}

std::string ScharrOp::GetPrincipleDoc() const
{
    return "### 📘 算子原理：Scharr 高精度边缘检测 (Scharr)\n\n"
           "**【核心思想】**\n"
           "标准 3×3 Sobel 算子在非轴向角度存在一定旋转不变性偏差。Scharr 算子针对 3×3 小尺度空间核重新进行了系数数值优化，具有更高的方向对称性与细微边缘响应灵敏度。\n\n"
           "**【数学模型 / 卷积核矩阵】**\n"
           "```\n"
           "   水平高精度核 Gx                        垂直高精度核 Gy\n"
           "        ┌  -3   0   3 ┐                        ┌  -3 -10  -3 ┐\n"
           "   Gx = │ -10   0  10 │ · I               Gy = │   0   0   0 │ · I\n"
           "        └  -3   0   3 ┘                        └   3  10   3 ┘\n"
           "```\n\n"
           "**【符号与参数解析】**\n"
           "- **direction**：选择求导方向分量或全向合成；\n"
           "- **scale**：梯度增益系数。\n\n"
           "**【工程应用建议】**\n"
           "- 相比 Sobel 3×3：在检测低对比度浅边缘、极细划痕或精密工件毛刺时响应更强、定位更锐利。";
}

bool ScharrOp::Process(const cv::Mat& input, cv::Mat& output, std::string& errorMsg)
{
    if (input.empty()) { errorMsg = "输入图像为空"; return false; }
    cv::Mat gray = input;
    if (input.channels() > 1) cv::cvtColor(input, gray, cv::COLOR_BGR2GRAY);

    int dir = _params[0].GetInt();
    double scale = _params[1].GetDouble();

    try
    {
        if (dir == 0)
        {
            cv::Mat grad_x;
            cv::Scharr(gray, grad_x, CV_16S, 1, 0, scale);
            cv::convertScaleAbs(grad_x, output);
        }
        else if (dir == 1)
        {
            cv::Mat grad_y;
            cv::Scharr(gray, grad_y, CV_16S, 0, 1, scale);
            cv::convertScaleAbs(grad_y, output);
        }
        else
        {
            cv::Mat grad_x, grad_y, abs_grad_x, abs_grad_y;
            cv::Scharr(gray, grad_x, CV_16S, 1, 0, scale);
            cv::Scharr(gray, grad_y, CV_16S, 0, 1, scale);
            cv::convertScaleAbs(grad_x, abs_grad_x);
            cv::convertScaleAbs(grad_y, abs_grad_y);
            cv::addWeighted(abs_grad_x, 0.5, abs_grad_y, 0.5, 0, output);
        }
        return true;
    }
    catch (const cv::Exception& e)
    {
        errorMsg = e.what();
        return false;
    }
}

std::string ScharrOp::GenerateCppCode(const std::string& inVar, const std::string& outVar) const
{
    int dir = _params[0].GetInt();
    double scale = _params[1].GetDouble();

    std::stringstream ss;
    ss << "// Scharr高精度梯度算子参数配置\n"
       << "double scale = " << scale << ";          // 梯度缩放比例\n";
    if (dir == 0)
    {
        ss << "cv::Mat gradX;\n"
           << "cv::Scharr(" << inVar << ", gradX, CV_16S, 1, 0, scale);\n"
           << "cv::convertScaleAbs(gradX, " << outVar << ");";
    }
    else if (dir == 1)
    {
        ss << "cv::Mat gradY;\n"
           << "cv::Scharr(" << inVar << ", gradY, CV_16S, 0, 1, scale);\n"
           << "cv::convertScaleAbs(gradY, " << outVar << ");";
    }
    else
    {
        ss << "cv::Mat gradX, gradY, absGradX, absGradY;\n"
           << "cv::Scharr(" << inVar << ", gradX, CV_16S, 1, 0, scale);\n"
           << "cv::Scharr(" << inVar << ", gradY, CV_16S, 0, 1, scale);\n"
           << "cv::convertScaleAbs(gradX, absGradX);\n"
           << "cv::convertScaleAbs(gradY, absGradY);\n"
           << "cv::addWeighted(absGradX, 0.5, absGradY, 0.5, 0, " << outVar << ");";
    }
    return ss.str();
}

std::string ScharrOp::GeneratePythonCode(const std::string& inVar, const std::string& outVar) const
{
    int dir = _params[0].GetInt();
    double scale = _params[1].GetDouble();

    std::stringstream ss;
    ss << "# Scharr高精度梯度算子参数配置\n"
       << "scale = " << scale << "                 # 梯度缩放比例\n";
    if (dir == 0)
    {
        ss << "grad_x = cv2.Scharr(" << inVar << ", cv2.CV_16S, 1, 0, scale=scale)\n"
           << outVar << " = cv2.convertScaleAbs(grad_x)";
    }
    else if (dir == 1)
    {
        ss << "grad_y = cv2.Scharr(" << inVar << ", cv2.CV_16S, 0, 1, scale=scale)\n"
           << outVar << " = cv2.convertScaleAbs(grad_y)";
    }
    else
    {
        ss << "grad_x = cv2.Scharr(" << inVar << ", cv2.CV_16S, 1, 0, scale=scale)\n"
           << "grad_y = cv2.Scharr(" << inVar << ", cv2.CV_16S, 0, 1, scale=scale)\n"
           << "abs_x = cv2.convertScaleAbs(grad_x)\n"
           << "abs_y = cv2.convertScaleAbs(grad_y)\n"
           << outVar << " = cv2.addWeighted(abs_x, 0.5, abs_y, 0.5, 0)";
    }
    return ss.str();
}

// ---------------- LaplacianOp ----------------
LaplacianOp::LaplacianOp()
{
    OpParameter ksizeParam;
    ksizeParam.Name = "ksize";
    ksizeParam.DisplayName = "核大小 (KSize)";
    ksizeParam.Type = ParamType::IntSlider;
    ksizeParam.Value = 3;
    ksizeParam.MinValue = 1;
    ksizeParam.MaxValue = 7;
    ksizeParam.Step = 2;
    _params.push_back(ksizeParam);

    OpParameter scaleParam;
    scaleParam.Name = "scale";
    scaleParam.DisplayName = "缩放比例 (Scale)";
    scaleParam.Type = ParamType::DoubleSlider;
    scaleParam.Value = 1.0;
    scaleParam.MinValue = 0.1;
    scaleParam.MaxValue = 5.0;
    scaleParam.Step = 0.1;
    _params.push_back(scaleParam);
}

std::string LaplacianOp::GetPrincipleDoc() const
{
    return "### 📘 算子原理：拉普拉斯二阶微分边缘检测 (Laplacian)\n\n"
           "**【核心思想】**\n"
           "拉普拉斯算子是一种二维各向同性（旋转不变）二阶微分算子，通过寻找图像灰度二阶导数的零交叉点 (Zero-Crossing) 来精确定位边缘。\n\n"
           "**【数学模型 / 微分方程与离散核】**\n"
           "```\n"
           "   连续空间公式:  ΔI = ∂²I/∂x² + ∂²I/∂y²\n"
           "\n"
           "   标准 3×3 4-邻域离散核               扩展 3×3 8-邻域离散核\n"
           "        ┌  0   1   0 ┐                     ┌  1   1   1 ┐\n"
           "    L = │  1  -4   1 │                 L = │  1  -8   1 │\n"
           "        └  0   1   0 ┘                     └  1   1   1 ┘\n"
           "```\n\n"
           "**【符号与参数解析】**\n"
           "- **ksize**：二阶导数核孔径（必须为正奇数）；\n"
           "- **scale**：二阶差分结果的缩放乘数。\n\n"
           "**【工程应用建议】**\n"
           "- 特点：对孤立噪点、细线条与角点高度敏感，通常建议先进行高斯平滑（即 LoG 算子组合）以避免噪点被二次放大。";
}

bool LaplacianOp::Process(const cv::Mat& input, cv::Mat& output, std::string& errorMsg)
{
    if (input.empty()) { errorMsg = "输入图像为空"; return false; }
    cv::Mat gray = input;
    if (input.channels() > 1) cv::cvtColor(input, gray, cv::COLOR_BGR2GRAY);

    int ksize = _params[0].GetInt();
    if (ksize % 2 == 0) ksize += 1;
    double scale = _params[1].GetDouble();

    try
    {
        cv::Mat lap;
        cv::Laplacian(gray, lap, CV_16S, ksize, scale);
        cv::convertScaleAbs(lap, output);
        return true;
    }
    catch (const cv::Exception& e)
    {
        errorMsg = e.what();
        return false;
    }
}

std::string LaplacianOp::GenerateCppCode(const std::string& inVar, const std::string& outVar) const
{
    int ksize = _params[0].GetInt();
    if (ksize % 2 == 0) ksize += 1;
    double scale = _params[1].GetDouble();

    std::stringstream ss;
    ss << "// 拉普拉斯二阶微分边缘检测参数配置\n"
       << "int ksize = " << ksize << ";             // 核孔径大小\n"
       << "double scale = " << scale << ";          // 梯度缩放系数\n"
       << "cv::Mat lap_" << outVar << ";\n"
       << "cv::Laplacian(" << inVar << ", lap_" << outVar << ", CV_16S, ksize, scale);\n"
       << "cv::convertScaleAbs(lap_" << outVar << ", " << outVar << ");";
    return ss.str();
}

std::string LaplacianOp::GeneratePythonCode(const std::string& inVar, const std::string& outVar) const
{
    int ksize = _params[0].GetInt();
    if (ksize % 2 == 0) ksize += 1;
    double scale = _params[1].GetDouble();

    std::stringstream ss;
    ss << "# 拉普拉斯二阶微分边缘检测参数配置\n"
       << "ksize = " << ksize << "                 # 核孔径大小\n"
       << "scale = " << scale << "                 # 梯度缩放系数\n"
       << "lap_" << outVar << " = cv2.Laplacian(" << inVar << ", cv2.CV_16S, ksize=ksize, scale=scale)\n"
       << outVar << " = cv2.convertScaleAbs(lap_" << outVar << ")";
    return ss.str();
}
