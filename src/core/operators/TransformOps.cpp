#include "TransformOps.h"
#include <sstream>

// ---------------- ResizeOp ----------------
ResizeOp::ResizeOp()
{
    OpParameter scaleParam;
    scaleParam.Name = "scale";
    scaleParam.DisplayName = "缩放比例 (Scale)";
    scaleParam.Tooltip = "缩放倍数 (0.1 ~ 5.0)";
    scaleParam.Type = ParamType::DoubleSlider;
    scaleParam.Value = 0.5;
    scaleParam.MinValue = 0.1;
    scaleParam.MaxValue = 4.0;
    scaleParam.Step = 0.05;
    _params.push_back(scaleParam);

    OpParameter interpParam;
    interpParam.Name = "interpolation";
    interpParam.DisplayName = "插值方式 (Interpolation)";
    interpParam.Tooltip = "像素重采样算法";
    interpParam.Type = ParamType::ComboBox;
    interpParam.Value = 1; // Linear
    interpParam.EnumOptions = {
        "最近邻插值 (INTER_NEAREST - 速度最快)",
        "双线性插值 (INTER_LINEAR - 推荐默认)",
        "双三次插值 (INTER_CUBIC - 放大效果好)",
        "区域像素重采样 (INTER_AREA - 缩小效果最佳)",
        "Lanczos插值 (INTER_LANCZOS4 - 高保真)"
    };
    _params.push_back(interpParam);
}

std::string ResizeOp::GetPrincipleDoc() const
{
    return "### 📘 算子原理：图像缩放 (resize)\n\n"
           "调整图像的像素空间分辨率：\n"
           "* **INTER_NEAREST**：取最近像素点，计算极快但存在马赛克；\n"
           "* **INTER_LINEAR**：双线性插值，平衡速度与画质；\n"
           "* **INTER_AREA**：基于像素区域重采样的缩小算法，避免摩尔纹失真；\n"
           "* **INTER_CUBIC**：4x4 邻域双三次插值，放大时更加平滑。";
}

bool ResizeOp::Process(const cv::Mat& input, cv::Mat& output, std::string& errorMsg)
{
    if (input.empty())
    {
        errorMsg = "输入图像为空";
        return false;
    }

    double scale = _params[0].GetDouble();
    if (scale <= 0.01) { scale = 0.01; }
    int interpIdx = _params[1].GetInt();
    int cvInterp = cv::INTER_LINEAR;
    if (interpIdx == 0) cvInterp = cv::INTER_NEAREST;
    else if (interpIdx == 2) cvInterp = cv::INTER_CUBIC;
    else if (interpIdx == 3) cvInterp = cv::INTER_AREA;
    else if (interpIdx == 4) cvInterp = cv::INTER_LANCZOS4;

    try
    {
        cv::resize(input, output, cv::Size(), scale, scale, cvInterp);
        return true;
    }
    catch (const cv::Exception& e)
    {
        errorMsg = e.what();
        return false;
    }
}

std::string ResizeOp::GenerateCppCode(const std::string& inVar, const std::string& outVar) const
{
    double scale = _params[0].GetDouble();
    int interpIdx = _params[1].GetInt();
    std::string interpStr = "cv::INTER_LINEAR";
    if (interpIdx == 0) interpStr = "cv::INTER_NEAREST";
    else if (interpIdx == 2) interpStr = "cv::INTER_CUBIC";
    else if (interpIdx == 3) interpStr = "cv::INTER_AREA";
    else if (interpIdx == 4) interpStr = "cv::INTER_LANCZOS4";

    std::stringstream ss;
    ss << "cv::resize(" << inVar << ", " << outVar << ", cv::Size(), " << scale << ", " << scale << ", " << interpStr << ");";
    return ss.str();
}

std::string ResizeOp::GeneratePythonCode(const std::string& inVar, const std::string& outVar) const
{
    double scale = _params[0].GetDouble();
    int interpIdx = _params[1].GetInt();
    std::string interpStr = "cv2.INTER_LINEAR";
    if (interpIdx == 0) interpStr = "cv2.INTER_NEAREST";
    else if (interpIdx == 2) interpStr = "cv2.INTER_CUBIC";
    else if (interpIdx == 3) interpStr = "cv2.INTER_AREA";
    else if (interpIdx == 4) interpStr = "cv2.INTER_LANCZOS4";

    std::stringstream ss;
    ss << outVar << " = cv2.resize(" << inVar << ", None, fx=" << scale << ", fy=" << scale << ", interpolation=" << interpStr << ")";
    return ss.str();
}

// ---------------- RotateOp ----------------
RotateOp::RotateOp()
{
    OpParameter angleParam;
    angleParam.Name = "angle";
    angleParam.DisplayName = "旋转角度 (Angle °)";
    angleParam.Tooltip = "逆时针旋转角度 (-180° ~ 180°)";
    angleParam.Type = ParamType::DoubleSlider;
    angleParam.Value = 45.0;
    angleParam.MinValue = -180.0;
    angleParam.MaxValue = 180.0;
    angleParam.Step = 1.0;
    _params.push_back(angleParam);

    OpParameter scaleParam;
    scaleParam.Name = "scale";
    scaleParam.DisplayName = "缩放系数 (Scale)";
    scaleParam.Tooltip = "旋转同时的缩放倍数";
    scaleParam.Type = ParamType::DoubleSlider;
    scaleParam.Value = 1.0;
    scaleParam.MinValue = 0.1;
    scaleParam.MaxValue = 2.0;
    scaleParam.Step = 0.05;
    _params.push_back(scaleParam);
}

std::string RotateOp::GetPrincipleDoc() const
{
    return "### 📘 算子原理：仿射旋转 (getRotationMatrix2D & warpAffine)\n\n"
           "以图像中心点 $(c_x, c_y)$ 为锚点构建仿射变换矩阵：\n"
           "$$M = \\begin{bmatrix} \\alpha & \\beta & (1-\\alpha)c_x - \\beta c_y \\\\ -\\beta & \\alpha & \\beta c_x + (1-\\alpha)c_y \\end{bmatrix}$$\n"
           "其中 $\\alpha = \\text{scale} \\cdot \\cos(\\theta), \\beta = \\text{scale} \\cdot \\sin(\\theta)$。";
}

bool RotateOp::Process(const cv::Mat& input, cv::Mat& output, std::string& errorMsg)
{
    if (input.empty())
    {
        errorMsg = "输入图像为空";
        return false;
    }

    double angle = _params[0].GetDouble();
    double scale = _params[1].GetDouble();
    cv::Point2f center(input.cols / 2.0f, input.rows / 2.0f);

    try
    {
        cv::Mat rotMat = cv::getRotationMatrix2D(center, angle, scale);
        cv::warpAffine(input, output, rotMat, input.size(), cv::INTER_LINEAR, cv::BORDER_CONSTANT, cv::Scalar(0, 0, 0));
        return true;
    }
    catch (const cv::Exception& e)
    {
        errorMsg = e.what();
        return false;
    }
}

std::string RotateOp::GenerateCppCode(const std::string& inVar, const std::string& outVar) const
{
    double angle = _params[0].GetDouble();
    double scale = _params[1].GetDouble();

    std::stringstream ss;
    ss << "cv::Point2f center_" << outVar << "(" << inVar << ".cols / 2.0f, " << inVar << ".rows / 2.0f);\n"
       << "cv::Mat rotMat_" << outVar << " = cv::getRotationMatrix2D(center_" << outVar << ", " << angle << ", " << scale << ");\n"
       << "cv::warpAffine(" << inVar << ", " << outVar << ", rotMat_" << outVar << ", " << inVar << ".size());";
    return ss.str();
}

std::string RotateOp::GeneratePythonCode(const std::string& inVar, const std::string& outVar) const
{
    double angle = _params[0].GetDouble();
    double scale = _params[1].GetDouble();

    std::stringstream ss;
    ss << "h, w = " << inVar << ".shape[:2]\n"
       << "center = (w / 2.0, h / 2.0)\n"
       << "rot_mat = cv2.getRotationMatrix2D(center, " << angle << ", " << scale << ")\n"
       << outVar << " = cv2.warpAffine(" << inVar << ", rot_mat, (w, h))";
    return ss.str();
}

// ---------------- FlipOp ----------------
FlipOp::FlipOp()
{
    OpParameter modeParam;
    modeParam.Name = "flipCode";
    modeParam.DisplayName = "翻转模式 (FlipCode)";
    modeParam.Tooltip = "镜像翻转轴线选择";
    modeParam.Type = ParamType::ComboBox;
    modeParam.Value = 1; // 水平
    modeParam.EnumOptions = { "垂直翻转 (沿 X 轴翻转，FlipCode=0)", "水平翻转 (沿 Y 轴翻转，FlipCode=1)", "对角双向翻转 (沿 XY 双轴，FlipCode=-1)" };
    _params.push_back(modeParam);
}

std::string FlipOp::GetPrincipleDoc() const
{
    return "### 📘 算子原理：图像镜像 (flip)\n\n"
           "围绕垂直轴、水平轴或同时围绕两者进行像素坐标倒置映射：\n"
           "* **flipCode = 1**：水平镜像（左变为右，常用于相机镜像修正）；\n"
           "* **flipCode = 0**：垂直镜像（上变为下）；\n"
           "* **flipCode = -1**：水平+垂直对角翻转（等同于 180° 旋转）。";
}

bool FlipOp::Process(const cv::Mat& input, cv::Mat& output, std::string& errorMsg)
{
    if (input.empty())
    {
        errorMsg = "输入图像为空";
        return false;
    }

    int modeIdx = _params[0].GetInt();
    int flipCode = 1;
    if (modeIdx == 0) flipCode = 0;
    else if (modeIdx == 2) flipCode = -1;

    try
    {
        cv::flip(input, output, flipCode);
        return true;
    }
    catch (const cv::Exception& e)
    {
        errorMsg = e.what();
        return false;
    }
}

std::string FlipOp::GenerateCppCode(const std::string& inVar, const std::string& outVar) const
{
    int modeIdx = _params[0].GetInt();
    int flipCode = 1;
    if (modeIdx == 0) flipCode = 0;
    else if (modeIdx == 2) flipCode = -1;

    std::stringstream ss;
    ss << "cv::flip(" << inVar << ", " << outVar << ", " << flipCode << ");";
    return ss.str();
}

std::string FlipOp::GeneratePythonCode(const std::string& inVar, const std::string& outVar) const
{
    int modeIdx = _params[0].GetInt();
    int flipCode = 1;
    if (modeIdx == 0) flipCode = 0;
    else if (modeIdx == 2) flipCode = -1;

    std::stringstream ss;
    ss << outVar << " = cv2.flip(" << inVar << ", " << flipCode << ")";
    return ss.str();
}

// ---------------- CropRoiOp ----------------
CropRoiOp::CropRoiOp()
{
    OpParameter xParam;
    xParam.Name = "x_pct";
    xParam.DisplayName = "起始 X 比例 (%)";
    xParam.Tooltip = "左上角 X 占图像宽度的百分比";
    xParam.Type = ParamType::IntSlider;
    xParam.Value = 10;
    xParam.MinValue = 0;
    xParam.MaxValue = 90;
    xParam.Step = 1;
    _params.push_back(xParam);

    OpParameter yParam;
    yParam.Name = "y_pct";
    yParam.DisplayName = "起始 Y 比例 (%)";
    yParam.Tooltip = "左上角 Y 占图像高度的百分比";
    yParam.Type = ParamType::IntSlider;
    yParam.Value = 10;
    yParam.MinValue = 0;
    yParam.MaxValue = 90;
    yParam.Step = 1;
    _params.push_back(yParam);

    OpParameter wParam;
    wParam.Name = "w_pct";
    wParam.DisplayName = "宽度比例 (%)";
    wParam.Tooltip = "ROI 宽度百分比";
    wParam.Type = ParamType::IntSlider;
    wParam.Value = 80;
    wParam.MinValue = 5;
    wParam.MaxValue = 100;
    wParam.Step = 1;
    _params.push_back(wParam);

    OpParameter hParam;
    hParam.Name = "h_pct";
    hParam.DisplayName = "高度比例 (%)";
    hParam.Tooltip = "ROI 高度百分比";
    hParam.Type = ParamType::IntSlider;
    hParam.Value = 80;
    hParam.MinValue = 5;
    hParam.MaxValue = 100;
    hParam.Step = 1;
    _params.push_back(hParam);
}

std::string CropRoiOp::GetPrincipleDoc() const
{
    return "### 📘 算子原理：ROI 感兴趣区域裁剪 (Crop / ROI)\n\n"
           "通过矩阵切片 `image(Rect(x, y, width, height))` 提取特定矩形区域，锁定核心检测视野并大幅降低后续算法的计算耗时。";
}

bool CropRoiOp::Process(const cv::Mat& input, cv::Mat& output, std::string& errorMsg)
{
    if (input.empty())
    {
        errorMsg = "输入图像为空";
        return false;
    }

    int x = (_params[0].GetInt() * input.cols) / 100;
    int y = (_params[1].GetInt() * input.rows) / 100;
    int w = (_params[2].GetInt() * input.cols) / 100;
    int h = (_params[3].GetInt() * input.rows) / 100;

    if (x + w > input.cols) { w = input.cols - x; }
    if (y + h > input.rows) { h = input.rows - y; }
    if (w <= 0 || h <= 0)
    {
        errorMsg = "裁剪区域长宽非法";
        return false;
    }

    try
    {
        cv::Rect roi(x, y, w, h);
        output = input(roi).clone();
        return true;
    }
    catch (const cv::Exception& e)
    {
        errorMsg = e.what();
        return false;
    }
}

std::string CropRoiOp::GenerateCppCode(const std::string& inVar, const std::string& outVar) const
{
    int xPct = _params[0].GetInt(), yPct = _params[1].GetInt();
    int wPct = _params[2].GetInt(), hPct = _params[3].GetInt();

    std::stringstream ss;
    ss << "int x_" << outVar << " = (" << inVar << ".cols * " << xPct << ") / 100;\n"
       << "int y_" << outVar << " = (" << inVar << ".rows * " << yPct << ") / 100;\n"
       << "int w_" << outVar << " = (" << inVar << ".cols * " << wPct << ") / 100;\n"
       << "int h_" << outVar << " = (" << inVar << ".rows * " << hPct << ") / 100;\n"
       << "cv::Rect roi_" << outVar << "(x_" << outVar << ", y_" << outVar << ", w_" << outVar << ", h_" << outVar << ");\n"
       << outVar << " = " << inVar << "(roi_" << outVar << ").clone();";
    return ss.str();
}

std::string CropRoiOp::GeneratePythonCode(const std::string& inVar, const std::string& outVar) const
{
    int xPct = _params[0].GetInt(), yPct = _params[1].GetInt();
    int wPct = _params[2].GetInt(), hPct = _params[3].GetInt();

    std::stringstream ss;
    ss << "h, w = " << inVar << ".shape[:2]\n"
       << "x = int(w * " << xPct / 100.0 << ")\n"
       << "y = int(h * " << yPct / 100.0 << ")\n"
       << "roi_w = int(w * " << wPct / 100.0 << ")\n"
       << "roi_h = int(h * " << hPct / 100.0 << ")\n"
       << outVar << " = " << inVar << "[y:y+roi_h, x:x+roi_w].copy()";
    return ss.str();
}
