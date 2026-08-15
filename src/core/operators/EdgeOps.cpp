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
    return "### 📘 算子原理：Canny 边缘检测 (Canny)\n\n"
           "经典的极值抑制多阶段边缘检测算法：\n"
           "1. 高斯滤波降噪；\n"
           "2. 计算梯度幅值与方向；\n"
           "3. **非极大值抑制 (NMS)**，细化边缘；\n"
           "4. **双阈值滞后跟踪**：高于 Threshold2 为确定边缘，低于 Threshold1 抑制，介于两者之间的若与确定边缘连通则保留。";
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
           "结合高斯平滑和微分求导，计算一阶离散差分梯度：\n"
           "$$G_x = \\begin{bmatrix} -1 & 0 & 1 \\\\ -2 & 0 & 2 \\\\ -1 & 0 & 1 \\end{bmatrix} * I, \\quad G_y = \\begin{bmatrix} -1 & -2 & -1 \\\\ 0 & 0 & 0 \\\\ 1 & 2 & 1 \\end{bmatrix} * I$$\n"
           "合成幅值：$G = \\sqrt{G_x^2 + G_y^2}$。";
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
           "当 Sobel 算子尺寸为 $3 \\times 3$ 时，可能存在方向旋转不变性误差。Scharr 算子针对小核进行了优化：\n"
           "$$G_x = \\begin{bmatrix} -3 & 0 & 3 \\\\ -10 & 0 & 10 \\\\ -3 & 0 & 3 \\end{bmatrix}$$\n"
           "**优势**：在检测细微边缘时比 Sobel 更准确、梯度幅值更强。";
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
    return "### 📘 算子原理：拉普拉斯二阶导数边缘检测 (Laplacian)\n\n"
           "拉普拉斯算子是**各向同性**的二阶微分算子：\n"
           "$$\\Delta I = \\frac{\\partial^2 I}{\\partial x^2} + \\frac{\\partial^2 I}{\\partial y^2}$$\n"
           "**特点**：检测图像灰度变化的过零点 (Zero-Crossing)，对孤立细线和角点特别敏感。";
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
