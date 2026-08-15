#include "TransformOps.h"
#include <sstream>

// ---------------- ResizeOp ----------------
ResizeOp::ResizeOp()
{
    OpParameter scaleParam;
    scaleParam.Name = "scale";
    scaleParam.DisplayName = "缩放比例 (Scale)";
    scaleParam.Type = ParamType::DoubleSlider;
    scaleParam.Value = 1.0;
    scaleParam.MinValue = 0.1;
    scaleParam.MaxValue = 5.0;
    scaleParam.Step = 0.05;
    _params.push_back(scaleParam);

    OpParameter interpParam;
    interpParam.Name = "interpolation";
    interpParam.DisplayName = "插值算法 (Interpolation)";
    interpParam.Type = ParamType::ComboBox;
    interpParam.Value = 1; // Linear
    interpParam.EnumOptions = {
        "最近邻插值 (Nearest)",
        "双线性插值 (Linear - 推荐)",
        "双三次插值 (Cubic - 高画质)",
        "区域像素重采样 (Area - 缩小专用)"
    };
    _params.push_back(interpParam);
}

std::string ResizeOp::GetPrincipleDoc() const
{
    return "### 📘 算子原理：几何缩放 (resize)\n\n"
           "* **最近邻 (NEAREST)**：速度最快，有马赛克锯齿。\n"
           "* **双线性 (LINEAR)**：取 $2 \\times 2$ 邻域加权，速度与画质均衡。\n"
           "* **双三次 (CUBIC)**：取 $4 \\times 4$ 邻域三次样条插值，边缘平滑。\n"
           "* **区域插值 (AREA)**：图像缩小采样时能有效避免莫尔纹伪影。";
}

bool ResizeOp::Process(const cv::Mat& input, cv::Mat& output, std::string& errorMsg)
{
    if (input.empty()) { errorMsg = "输入图像为空"; return false; }
    double scale = _params[0].GetDouble();
    int interpIdx = _params[1].GetInt();

    int cvInterp = cv::INTER_LINEAR;
    if (interpIdx == 0) cvInterp = cv::INTER_NEAREST;
    else if (interpIdx == 2) cvInterp = cv::INTER_CUBIC;
    else if (interpIdx == 3) cvInterp = cv::INTER_AREA;

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

    std::stringstream ss;
    ss << "// 几何图像缩放参数配置\n"
       << "double scaleFactor = " << scale << ";        // 缩放比例因子\n"
       << "int interpolation = " << interpStr << ";\n"
       << "cv::resize(" << inVar << ", " << outVar << ", cv::Size(), scaleFactor, scaleFactor, interpolation);";
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

    std::stringstream ss;
    ss << "# 几何图像缩放参数配置\n"
       << "scale_factor = " << scale << "             # 缩放比例因子\n"
       << "interpolation = " << interpStr << "\n"
       << outVar << " = cv2.resize(" << inVar << ", None, fx=scale_factor, fy=scale_factor, interpolation=interpolation)";
    return ss.str();
}

// ---------------- RotateOp ----------------
RotateOp::RotateOp()
{
    OpParameter angleParam;
    angleParam.Name = "angle";
    angleParam.DisplayName = "旋转角度 (Angle °)";
    angleParam.Type = ParamType::DoubleSlider;
    angleParam.Value = 0.0;
    angleParam.MinValue = -180.0;
    angleParam.MaxValue = 180.0;
    angleParam.Step = 1.0;
    _params.push_back(angleParam);

    OpParameter scaleParam;
    scaleParam.Name = "scale";
    scaleParam.DisplayName = "缩放比例 (Scale)";
    scaleParam.Type = ParamType::DoubleSlider;
    scaleParam.Value = 1.0;
    scaleParam.MinValue = 0.1;
    scaleParam.MaxValue = 3.0;
    scaleParam.Step = 0.05;
    _params.push_back(scaleParam);
}

std::string RotateOp::GetPrincipleDoc() const
{
    return "### 📘 算子原理：中心仿射旋转 (getRotationMatrix2D + warpAffine)\n\n"
           "以图像中心点为原点，构建二维仿射变换矩阵：\n"
           "$$\\begin{bmatrix} \\alpha & \\beta & (1-\\alpha)\\cdot cx - \\beta\\cdot cy \\\\ -\\beta & \\alpha & \\beta\\cdot cx + (1-\\alpha)\\cdot cy \\end{bmatrix}$$\n"
           "其中 $\\alpha = scale \\cdot \\cos(\\theta), \\beta = scale \\cdot \\sin(\\theta)$。";
}

bool RotateOp::Process(const cv::Mat& input, cv::Mat& output, std::string& errorMsg)
{
    if (input.empty()) { errorMsg = "输入图像为空"; return false; }
    double angle = _params[0].GetDouble();
    double scale = _params[1].GetDouble();

    if (std::abs(angle) < 1e-4 && std::abs(scale - 1.0) < 1e-4)
    {
        output = input.clone();
        return true;
    }

    try
    {
        cv::Point2f center(input.cols / 2.0f, input.rows / 2.0f);
        cv::Mat rotMat = cv::getRotationMatrix2D(center, angle, scale);
        cv::warpAffine(input, output, rotMat, input.size(), cv::INTER_LINEAR, cv::BORDER_CONSTANT, cv::Scalar());
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
    ss << "// 图像中心仿射旋转参数配置\n"
       << "double rotAngle = " << angle << ";          // 旋转角度 (度)\n"
       << "double rotScale = " << scale << ";          // 同步缩放因子\n"
       << "cv::Point2f center(" << inVar << ".cols / 2.0f, " << inVar << ".rows / 2.0f);\n"
       << "cv::Mat rotMat = cv::getRotationMatrix2D(center, rotAngle, rotScale);\n"
       << "cv::warpAffine(" << inVar << ", " << outVar << ", rotMat, " << inVar << ".size());";
    return ss.str();
}

std::string RotateOp::GeneratePythonCode(const std::string& inVar, const std::string& outVar) const
{
    double angle = _params[0].GetDouble();
    double scale = _params[1].GetDouble();

    std::stringstream ss;
    ss << "# 图像中心仿射旋转参数配置\n"
       << "rot_angle = " << angle << "               # 旋转角度 (度)\n"
       << "rot_scale = " << scale << "               # 同步缩放因子\n"
       << "h, w = " << inVar << ".shape[:2]\n"
       << "center = (w / 2.0, h / 2.0)\n"
       << "rot_mat = cv2.getRotationMatrix2D(center, rot_angle, rot_scale)\n"
       << outVar << " = cv2.warpAffine(" << inVar << ", rot_mat, (w, h))";
    return ss.str();
}

// ---------------- FlipOp ----------------
FlipOp::FlipOp()
{
    OpParameter modeParam;
    modeParam.Name = "mode";
    modeParam.DisplayName = "翻转模式 (Mode)";
    modeParam.Type = ParamType::ComboBox;
    modeParam.Value = 0;
    modeParam.EnumOptions = {
        "水平镜像 (Horizontal Flip)",
        "垂直镜像 (Vertical Flip)",
        "水平与垂直双向镜像 (Both Flip)"
    };
    _params.push_back(modeParam);
}

std::string FlipOp::GetPrincipleDoc() const
{
    return "### 📘 算子原理：图像镜像翻转 (flip)\n\n"
           "* **flipCode = 1**：水平翻转（左右镜像，沿 Y 轴对称）\n"
           "* **flipCode = 0**：垂直翻转（上下镜像，沿 X 轴对称）\n"
           "* **flipCode = -1**：双向翻转（原点中心对称）";
}

bool FlipOp::Process(const cv::Mat& input, cv::Mat& output, std::string& errorMsg)
{
    if (input.empty()) { errorMsg = "输入图像为空"; return false; }
    int mode = _params[0].GetInt();
    int flipCode = (mode == 0) ? 1 : (mode == 1 ? 0 : -1);

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
    int mode = _params[0].GetInt();
    std::string desc = (mode == 0) ? "1; // 1: 水平镜像 (沿Y轴)" : (mode == 1 ? "0; // 0: 垂直镜像 (沿X轴)" : "-1; // -1: 双向镜像 (中心对称)");

    std::stringstream ss;
    ss << "// 图像翻转镜像参数配置\n"
       << "int flipCode = " << desc << "\n"
       << "cv::flip(" << inVar << ", " << outVar << ", flipCode);";
    return ss.str();
}

std::string FlipOp::GeneratePythonCode(const std::string& inVar, const std::string& outVar) const
{
    int mode = _params[0].GetInt();
    std::string desc = (mode == 0) ? "1 # 1: 水平镜像" : (mode == 1 ? "0 # 0: 垂直镜像" : "-1 # -1: 双向镜像");

    std::stringstream ss;
    ss << "# 图像翻转镜像参数配置\n"
       << "flip_code = " << desc << "\n"
       << outVar << " = cv2.flip(" << inVar << ", flip_code)";
    return ss.str();
}

// ---------------- CropRoiOp ----------------
CropRoiOp::CropRoiOp()
{
    OpParameter xParam;
    xParam.Name = "xRatio";
    xParam.DisplayName = "左边界比例 (X Ratio %)";
    xParam.Type = ParamType::IntSlider;
    xParam.Value = 10;
    xParam.MinValue = 0;
    xParam.MaxValue = 90;
    xParam.Step = 1;
    _params.push_back(xParam);

    OpParameter yParam;
    yParam.Name = "yRatio";
    yParam.DisplayName = "上边界比例 (Y Ratio %)";
    yParam.Type = ParamType::IntSlider;
    yParam.Value = 10;
    yParam.MinValue = 0;
    yParam.MaxValue = 90;
    yParam.Step = 1;
    _params.push_back(yParam);

    OpParameter wParam;
    wParam.Name = "wRatio";
    wParam.DisplayName = "裁剪宽度比例 (Width Ratio %)";
    wParam.Type = ParamType::IntSlider;
    wParam.Value = 80;
    wParam.MinValue = 10;
    wParam.MaxValue = 100;
    wParam.Step = 1;
    _params.push_back(wParam);

    OpParameter hParam;
    hParam.Name = "hRatio";
    hParam.DisplayName = "裁剪高度比例 (Height Ratio %)";
    hParam.Type = ParamType::IntSlider;
    hParam.Value = 80;
    hParam.MinValue = 10;
    hParam.MaxValue = 100;
    hParam.Step = 1;
    _params.push_back(hParam);
}

std::string CropRoiOp::GetPrincipleDoc() const
{
    return "### 📘 算子原理：ROI 感兴趣区域裁剪 (ROI Crop)\n\n"
           "通过矩形选框提取关键目标区域，剔除无关背景干扰，减少后续算法计算量。";
}

bool CropRoiOp::Process(const cv::Mat& input, cv::Mat& output, std::string& errorMsg)
{
    if (input.empty()) { errorMsg = "输入图像为空"; return false; }
    int xR = _params[0].GetInt();
    int yR = _params[1].GetInt();
    int wR = _params[2].GetInt();
    int hR = _params[3].GetInt();

    int x = (input.cols * xR) / 100;
    int y = (input.rows * yR) / 100;
    int w = (input.cols * wR) / 100;
    int h = (input.rows * hR) / 100;

    if (x + w > input.cols) w = input.cols - x;
    if (y + h > input.rows) h = input.rows - y;

    if (w <= 0 || h <= 0)
    {
        errorMsg = "裁剪区域长宽无效";
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
    int xR = _params[0].GetInt();
    int yR = _params[1].GetInt();
    int wR = _params[2].GetInt();
    int hR = _params[3].GetInt();

    std::stringstream ss;
    ss << "// ROI区域裁剪参数配置 (基于图像比例百分比计算)\n"
       << "int cropX = (" << inVar << ".cols * " << xR << ") / 100;      // 起始X坐标 (" << xR << "%)\n"
       << "int cropY = (" << inVar << ".rows * " << yR << ") / 100;      // 起始Y坐标 (" << yR << "%)\n"
       << "int cropW = (" << inVar << ".cols * " << wR << ") / 100;      // 宽度 (" << wR << "%)\n"
       << "int cropH = (" << inVar << ".rows * " << hR << ") / 100;      // 高度 (" << hR << "%)\n"
       << "cv::Rect roi(cropX, cropY, cropW, cropH);\n"
       << outVar << " = " << inVar << "(roi).clone();";
    return ss.str();
}

std::string CropRoiOp::GeneratePythonCode(const std::string& inVar, const std::string& outVar) const
{
    int xR = _params[0].GetInt();
    int yR = _params[1].GetInt();
    int wR = _params[2].GetInt();
    int hR = _params[3].GetInt();

    std::stringstream ss;
    ss << "# ROI区域裁剪参数配置\n"
       << "h_orig, w_orig = " << inVar << ".shape[:2]\n"
       << "crop_x = int((w_orig * " << xR << ") / 100)       # 起始X (" << xR << "%)\n"
       << "crop_y = int((h_orig * " << yR << ") / 100)       # 起始Y (" << yR << "%)\n"
       << "crop_w = int((w_orig * " << wR << ") / 100)       # 宽度 (" << wR << "%)\n"
       << "crop_h = int((h_orig * " << hR << ") / 100)       # 高度 (" << hR << "%)\n"
       << outVar << " = " << inVar << "[crop_y:crop_y+crop_h, crop_x:crop_x+crop_w].copy()";
    return ss.str();
}
