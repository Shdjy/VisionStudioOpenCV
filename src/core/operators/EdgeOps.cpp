#include "EdgeOps.h"
#include <sstream>

// ---------------- CannyOp ----------------
CannyOp::CannyOp()
{
    OpParameter thresh1;
    thresh1.Name = "threshold1";
    thresh1.DisplayName = "低阈值 (Threshold 1)";
    thresh1.Tooltip = "滞后阈值分割的较小阈值 (弱边缘)";
    thresh1.Type = ParamType::DoubleSlider;
    thresh1.Value = 50.0;
    thresh1.MinValue = 0.0;
    thresh1.MaxValue = 500.0;
    thresh1.Step = 1.0;
    _params.push_back(thresh1);

    OpParameter thresh2;
    thresh2.Name = "threshold2";
    thresh2.DisplayName = "高阈值 (Threshold 2)";
    thresh2.Tooltip = "滞后阈值分割的较大阈值 (强边缘)";
    thresh2.Type = ParamType::DoubleSlider;
    thresh2.Value = 150.0;
    thresh2.MinValue = 0.0;
    thresh2.MaxValue = 500.0;
    thresh2.Step = 1.0;
    _params.push_back(thresh2);

    OpParameter aperture;
    aperture.Name = "apertureSize";
    aperture.DisplayName = "Sobel孔径 (ApertureSize)";
    aperture.Tooltip = "内部 Sobel 算子孔径大小 (3, 5, 7)";
    aperture.Type = ParamType::IntSlider;
    aperture.Value = 3;
    aperture.MinValue = 3;
    aperture.MaxValue = 7;
    aperture.Step = 2;
    _params.push_back(aperture);

    OpParameter l2grad;
    l2grad.Name = "L2gradient";
    l2grad.DisplayName = "L2 梯度范数 (L2gradient)";
    l2grad.Tooltip = "是否使用更精确的 L2 范数 sqrt(dx^2+dy^2)";
    l2grad.Type = ParamType::CheckBox;
    l2grad.Value = false;
    _params.push_back(l2grad);
}

std::string CannyOp::GetPrincipleDoc() const
{
    return "### 📘 算子原理：Canny 边缘检测 (Canny)\n\n"
           "Canny 是机器视觉中最经典也是综合指标最优的边缘检测器，包含 4 大阶段：\n"
           "1. **高斯平滑**：抑制高频噪点；\n"
           "2. **计算梯度幅值与方向**：利用 Sobel 算子求取 $G_x$ 与 $G_y$；\n"
           "3. **非极大值抑制 (NMS)**：沿梯度方向细化边缘，仅保留局部最大值点；\n"
           "4. **双阈值滞后跟踪**：\n"
           "   * 梯度大于 `threshold2`：确认为**强边缘**保留；\n"
           "   * 梯度介于 `threshold1` 与 `threshold2` 之间：仅当与强边缘连通时才作为**弱边缘**保留；\n"
           "   * 梯度小于 `threshold1`：判定为噪声舍弃。";
}

bool CannyOp::Process(const cv::Mat& input, cv::Mat& output, std::string& errorMsg)
{
    if (input.empty())
    {
        errorMsg = "输入图像为空";
        return false;
    }

    cv::Mat gray;
    if (input.channels() > 1)
    {
        cv::cvtColor(input, gray, cv::COLOR_BGR2GRAY);
    }
    else
    {
        gray = input;
    }

    double t1 = _params[0].GetDouble();
    double t2 = _params[1].GetDouble();
    int aperture = _params[2].GetInt();
    if (aperture % 2 == 0) { aperture += 1; }
    bool l2 = _params[3].GetBool();

    try
    {
        cv::Canny(gray, output, t1, t2, aperture, l2);
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
    int aperture = _params[2].GetInt();
    if (aperture % 2 == 0) { aperture += 1; }
    std::string l2Str = _params[3].GetBool() ? "true" : "false";

    std::stringstream ss;
    ss << "cv::Mat gray_" << outVar << ";\n"
       << "if (" << inVar << ".channels() > 1) cv::cvtColor(" << inVar << ", gray_" << outVar << ", cv::COLOR_BGR2GRAY);\n"
       << "else gray_" << outVar << " = " << inVar << ";\n"
       << "cv::Canny(gray_" << outVar << ", " << outVar << ", " << t1 << ", " << t2 << ", " << aperture << ", " << l2Str << ");";
    return ss.str();
}

std::string CannyOp::GeneratePythonCode(const std::string& inVar, const std::string& outVar) const
{
    double t1 = _params[0].GetDouble();
    double t2 = _params[1].GetDouble();
    int aperture = _params[2].GetInt();
    if (aperture % 2 == 0) { aperture += 1; }
    std::string l2Str = _params[3].GetBool() ? "True" : "False";

    std::stringstream ss;
    ss << "gray_" << outVar << " = cv2.cvtColor(" << inVar << ", cv2.COLOR_BGR2GRAY) if len(" << inVar << ".shape) == 3 else " << inVar << "\n"
       << outVar << " = cv2.Canny(gray_" << outVar << ", " << t1 << ", " << t2 << ", apertureSize=" << aperture << ", L2gradient=" << l2Str << ")";
    return ss.str();
}

// ---------------- SobelOp ----------------
SobelOp::SobelOp()
{
    OpParameter dirParam;
    dirParam.Name = "direction";
    dirParam.DisplayName = "导数方向 (Direction)";
    dirParam.Tooltip = "选择求导的方向维度";
    dirParam.Type = ParamType::ComboBox;
    dirParam.Value = 2; // XY 合成
    dirParam.EnumOptions = { "仅 X 方向梯度 (dx=1, dy=0)", "仅 Y 方向梯度 (dx=0, dy=1)", "XY 综合梯度幅值 (|dx|+|dy|)" };
    _params.push_back(dirParam);

    OpParameter ksizeParam;
    ksizeParam.Name = "ksize";
    ksizeParam.DisplayName = "核大小 (Ksize)";
    ksizeParam.Tooltip = "Sobel 核大小 (1, 3, 5, 7)";
    ksizeParam.Type = ParamType::IntSlider;
    ksizeParam.Value = 3;
    ksizeParam.MinValue = 1;
    ksizeParam.MaxValue = 7;
    ksizeParam.Step = 2;
    _params.push_back(ksizeParam);

    OpParameter scaleParam;
    scaleParam.Name = "scale";
    scaleParam.DisplayName = "缩放尺度 (Scale)";
    scaleParam.Tooltip = "计算梯度的缩放比例系数";
    scaleParam.Type = ParamType::DoubleSlider;
    scaleParam.Value = 1.0;
    scaleParam.MinValue = 0.1;
    scaleParam.MaxValue = 5.0;
    scaleParam.Step = 0.1;
    _params.push_back(scaleParam);
}

std::string SobelOp::GetPrincipleDoc() const
{
    return "### 📘 算子原理：Sobel 边缘微分算子 (Sobel)\n\n"
           "Sobel 算子利用差分近似一阶导数，在抑制噪声的同时计算边缘：\n"
           "$$G_x = \\begin{bmatrix} -1 & 0 & +1 \\\\ -2 & 0 & +2 \\\\ -1 & 0 & +1 \\end{bmatrix} * I, \\quad G_y = \\begin{bmatrix} -1 & -2 & -1 \\\\ 0 & 0 & 0 \\\\ +1 & +2 & +1 \\end{bmatrix} * I$$\n"
           "综合幅值通常近似为 $G = |G_x| + |G_y|$。";
}

bool SobelOp::Process(const cv::Mat& input, cv::Mat& output, std::string& errorMsg)
{
    if (input.empty())
    {
        errorMsg = "输入图像为空";
        return false;
    }

    cv::Mat gray;
    if (input.channels() > 1) { cv::cvtColor(input, gray, cv::COLOR_BGR2GRAY); }
    else { gray = input; }

    int dir = _params[0].GetInt();
    int ksize = _params[1].GetInt();
    if (ksize % 2 == 0) { ksize += 1; }
    double scale = _params[2].GetDouble();

    try
    {
        if (dir == 0) // X
        {
            cv::Mat gradX;
            cv::Sobel(gray, gradX, CV_16S, 1, 0, ksize, scale);
            cv::convertScaleAbs(gradX, output);
        }
        else if (dir == 1) // Y
        {
            cv::Mat gradY;
            cv::Sobel(gray, gradY, CV_16S, 0, 1, ksize, scale);
            cv::convertScaleAbs(gradY, output);
        }
        else // XY
        {
            cv::Mat gradX, gradY, absX, absY;
            cv::Sobel(gray, gradX, CV_16S, 1, 0, ksize, scale);
            cv::Sobel(gray, gradY, CV_16S, 0, 1, ksize, scale);
            cv::convertScaleAbs(gradX, absX);
            cv::convertScaleAbs(gradY, absY);
            cv::addWeighted(absX, 0.5, absY, 0.5, 0, output);
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
    if (ksize % 2 == 0) { ksize += 1; }
    double scale = _params[2].GetDouble();

    std::stringstream ss;
    ss << "cv::Mat gray_" << outVar << ";\n"
       << "if (" << inVar << ".channels() > 1) cv::cvtColor(" << inVar << ", gray_" << outVar << ", cv::COLOR_BGR2GRAY);\n"
       << "else gray_" << outVar << " = " << inVar << ";\n";

    if (dir == 0)
    {
        ss << "cv::Mat gradX_" << outVar << ";\n"
           << "cv::Sobel(gray_" << outVar << ", gradX_" << outVar << ", CV_16S, 1, 0, " << ksize << ", " << scale << ");\n"
           << "cv::convertScaleAbs(gradX_" << outVar << ", " << outVar << ");";
    }
    else if (dir == 1)
    {
        ss << "cv::Mat gradY_" << outVar << ";\n"
           << "cv::Sobel(gray_" << outVar << ", gradY_" << outVar << ", CV_16S, 0, 1, " << ksize << ", " << scale << ");\n"
           << "cv::convertScaleAbs(gradY_" << outVar << ", " << outVar << ");";
    }
    else
    {
        ss << "cv::Mat gradX_" << outVar << ", gradY_" << outVar << ", absX, absY;\n"
           << "cv::Sobel(gray_" << outVar << ", gradX_" << outVar << ", CV_16S, 1, 0, " << ksize << ", " << scale << ");\n"
           << "cv::Sobel(gray_" << outVar << ", gradY_" << outVar << ", CV_16S, 0, 1, " << ksize << ", " << scale << ");\n"
           << "cv::convertScaleAbs(gradX_" << outVar << ", absX);\n"
           << "cv::convertScaleAbs(gradY_" << outVar << ", absY);\n"
           << "cv::addWeighted(absX, 0.5, absY, 0.5, 0, " << outVar << ");";
    }
    return ss.str();
}

std::string SobelOp::GeneratePythonCode(const std::string& inVar, const std::string& outVar) const
{
    int dir = _params[0].GetInt();
    int ksize = _params[1].GetInt();
    if (ksize % 2 == 0) { ksize += 1; }
    double scale = _params[2].GetDouble();

    std::stringstream ss;
    ss << "gray_" << outVar << " = cv2.cvtColor(" << inVar << ", cv2.COLOR_BGR2GRAY) if len(" << inVar << ".shape) == 3 else " << inVar << "\n";
    if (dir == 0)
    {
        ss << "gradX = cv2.Sobel(gray_" << outVar << ", cv2.CV_16S, 1, 0, ksize=" << ksize << ", scale=" << scale << ")\n"
           << outVar << " = cv2.convertScaleAbs(gradX)";
    }
    else if (dir == 1)
    {
        ss << "gradY = cv2.Sobel(gray_" << outVar << ", cv2.CV_16S, 0, 1, ksize=" << ksize << ", scale=" << scale << ")\n"
           << outVar << " = cv2.convertScaleAbs(gradY)";
    }
    else
    {
        ss << "gradX = cv2.Sobel(gray_" << outVar << ", cv2.CV_16S, 1, 0, ksize=" << ksize << ", scale=" << scale << ")\n"
           << "gradY = cv2.Sobel(gray_" << outVar << ", cv2.CV_16S, 0, 1, ksize=" << ksize << ", scale=" << scale << ")\n"
           << "absX = cv2.convertScaleAbs(gradX)\n"
           << "absY = cv2.convertScaleAbs(gradY)\n"
           << outVar << " = cv2.addWeighted(absX, 0.5, absY, 0.5, 0)";
    }
    return ss.str();
}

// ---------------- ScharrOp ----------------
ScharrOp::ScharrOp()
{
    OpParameter dirParam;
    dirParam.Name = "direction";
    dirParam.DisplayName = "导数方向 (Direction)";
    dirParam.Tooltip = "求导方向";
    dirParam.Type = ParamType::ComboBox;
    dirParam.Value = 2;
    dirParam.EnumOptions = { "X 方向梯度", "Y 方向梯度", "XY 综合幅值" };
    _params.push_back(dirParam);
}

std::string ScharrOp::GetPrincipleDoc() const
{
    return "### 📘 算子原理：Scharr 算子 (Scharr)\n\n"
           "当核大小为 3 时，Sobel 算子在对角线方向的梯度误差较大。Scharr 使用专门优化过的卷积核：\n"
           "$$G_x = \\begin{bmatrix} -3 & 0 & +3 \\\\ -10 & 0 & +10 \\\\ -3 & 0 & +3 \\end{bmatrix}, \\quad G_y = \\begin{bmatrix} -3 & -10 & -3 \\\\ 0 & 0 & 0 \\\\ +3 & +10 & +3 \\end{bmatrix}$$\n"
           "提供更高的旋转不变性与边缘精度。";
}

bool ScharrOp::Process(const cv::Mat& input, cv::Mat& output, std::string& errorMsg)
{
    if (input.empty())
    {
        errorMsg = "输入图像为空";
        return false;
    }

    cv::Mat gray;
    if (input.channels() > 1) { cv::cvtColor(input, gray, cv::COLOR_BGR2GRAY); }
    else { gray = input; }

    int dir = _params[0].GetInt();
    try
    {
        if (dir == 0)
        {
            cv::Mat gradX;
            cv::Scharr(gray, gradX, CV_16S, 1, 0);
            cv::convertScaleAbs(gradX, output);
        }
        else if (dir == 1)
        {
            cv::Mat gradY;
            cv::Scharr(gray, gradY, CV_16S, 0, 1);
            cv::convertScaleAbs(gradY, output);
        }
        else
        {
            cv::Mat gradX, gradY, absX, absY;
            cv::Scharr(gray, gradX, CV_16S, 1, 0);
            cv::Scharr(gray, gradY, CV_16S, 0, 1);
            cv::convertScaleAbs(gradX, absX);
            cv::convertScaleAbs(gradY, absY);
            cv::addWeighted(absX, 0.5, absY, 0.5, 0, output);
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
    std::stringstream ss;
    ss << "cv::Mat gray_" << outVar << ", gradX, gradY, absX, absY;\n"
       << "if (" << inVar << ".channels() > 1) cv::cvtColor(" << inVar << ", gray_" << outVar << ", cv::COLOR_BGR2GRAY);\n"
       << "else gray_" << outVar << " = " << inVar << ";\n"
       << "cv::Scharr(gray_" << outVar << ", gradX, CV_16S, 1, 0);\n"
       << "cv::Scharr(gray_" << outVar << ", gradY, CV_16S, 0, 1);\n"
       << "cv::convertScaleAbs(gradX, absX);\n"
       << "cv::convertScaleAbs(gradY, absY);\n"
       << "cv::addWeighted(absX, 0.5, absY, 0.5, 0, " << outVar << ");";
    return ss.str();
}

std::string ScharrOp::GeneratePythonCode(const std::string& inVar, const std::string& outVar) const
{
    std::stringstream ss;
    ss << "gray_" << outVar << " = cv2.cvtColor(" << inVar << ", cv2.COLOR_BGR2GRAY) if len(" << inVar << ".shape) == 3 else " << inVar << "\n"
       << "gradX = cv2.Scharr(gray_" << outVar << ", cv2.CV_16S, 1, 0)\n"
       << "gradY = cv2.Scharr(gray_" << outVar << ", cv2.CV_16S, 0, 1)\n"
       << "absX = cv2.convertScaleAbs(gradX)\n"
       << "absY = cv2.convertScaleAbs(gradY)\n"
       << outVar << " = cv2.addWeighted(absX, 0.5, absY, 0.5, 0)";
    return ss.str();
}

// ---------------- LaplacianOp ----------------
LaplacianOp::LaplacianOp()
{
    OpParameter ksizeParam;
    ksizeParam.Name = "ksize";
    ksizeParam.DisplayName = "孔径大小 (Ksize)";
    ksizeParam.Tooltip = "二阶导数计算核大小 (1, 3, 5, 7)";
    ksizeParam.Type = ParamType::IntSlider;
    ksizeParam.Value = 3;
    ksizeParam.MinValue = 1;
    ksizeParam.MaxValue = 7;
    ksizeParam.Step = 2;
    _params.push_back(ksizeParam);

    OpParameter scaleParam;
    scaleParam.Name = "scale";
    scaleParam.DisplayName = "缩放尺度 (Scale)";
    scaleParam.Tooltip = "缩放比例系数";
    scaleParam.Type = ParamType::DoubleSlider;
    scaleParam.Value = 1.0;
    scaleParam.MinValue = 0.1;
    scaleParam.MaxValue = 5.0;
    scaleParam.Step = 0.1;
    _params.push_back(scaleParam);
}

std::string LaplacianOp::GetPrincipleDoc() const
{
    return "### 📘 算子原理：拉普拉斯算子 (Laplacian)\n\n"
           "拉普拉斯算子是二阶空间微分算子：\n"
           "$$\\Delta I = \\frac{\\partial^2 I}{\\partial x^2} + \\frac{\\partial^2 I}{\\partial y^2}$$\n"
           "由于采用二阶导数零交叉点检测边缘，它具有**旋转不变性（各向同性）**，能够同时捕获各个方向的边缘。";
}

bool LaplacianOp::Process(const cv::Mat& input, cv::Mat& output, std::string& errorMsg)
{
    if (input.empty())
    {
        errorMsg = "输入图像为空";
        return false;
    }

    cv::Mat gray;
    if (input.channels() > 1) { cv::cvtColor(input, gray, cv::COLOR_BGR2GRAY); }
    else { gray = input; }

    int ksize = _params[0].GetInt();
    if (ksize % 2 == 0) { ksize += 1; }
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
    if (ksize % 2 == 0) { ksize += 1; }
    double scale = _params[1].GetDouble();

    std::stringstream ss;
    ss << "cv::Mat gray_" << outVar << ", lap_" << outVar << ";\n"
       << "if (" << inVar << ".channels() > 1) cv::cvtColor(" << inVar << ", gray_" << outVar << ", cv::COLOR_BGR2GRAY);\n"
       << "else gray_" << outVar << " = " << inVar << ";\n"
       << "cv::Laplacian(gray_" << outVar << ", lap_" << outVar << ", CV_16S, " << ksize << ", " << scale << ");\n"
       << "cv::convertScaleAbs(lap_" << outVar << ", " << outVar << ");";
    return ss.str();
}

std::string LaplacianOp::GeneratePythonCode(const std::string& inVar, const std::string& outVar) const
{
    int ksize = _params[0].GetInt();
    if (ksize % 2 == 0) { ksize += 1; }
    double scale = _params[1].GetDouble();

    std::stringstream ss;
    ss << "gray_" << outVar << " = cv2.cvtColor(" << inVar << ", cv2.COLOR_BGR2GRAY) if len(" << inVar << ".shape) == 3 else " << inVar << "\n"
       << "lap = cv2.Laplacian(gray_" << outVar << ", cv2.CV_16S, ksize=" << ksize << ", scale=" << scale << ")\n"
       << outVar << " = cv2.convertScaleAbs(lap)";
    return ss.str();
}
