#include "MorphologyOps.h"
#include <sstream>

// ---------------- DilateOp ----------------
DilateOp::DilateOp()
{
    OpParameter shapeParam;
    shapeParam.Name = "shape";
    shapeParam.DisplayName = "结构元形状 (Shape)";
    shapeParam.Type = ParamType::ComboBox;
    shapeParam.Value = 0;
    shapeParam.EnumOptions = { "矩形 (Rect)", "十字交叉 (Cross)", "椭圆 (Ellipse)" };
    _params.push_back(shapeParam);

    OpParameter ksizeParam;
    ksizeParam.Name = "ksize";
    ksizeParam.DisplayName = "结构元尺寸 (Size)";
    ksizeParam.Type = ParamType::IntSlider;
    ksizeParam.Value = 3;
    ksizeParam.MinValue = 1;
    ksizeParam.MaxValue = 31;
    ksizeParam.Step = 2;
    _params.push_back(ksizeParam);

    OpParameter iterParam;
    iterParam.Name = "iterations";
    iterParam.DisplayName = "迭代次数 (Iterations)";
    iterParam.Type = ParamType::IntSlider;
    iterParam.Value = 1;
    iterParam.MinValue = 1;
    iterParam.MaxValue = 10;
    iterParam.Step = 1;
    _params.push_back(iterParam);
}

std::string DilateOp::GetPrincipleDoc() const
{
    return "### 📘 算子原理：形态学膨胀 (dilate)\n\n"
           "用结构元核在图像上滑动，输出为结构元覆盖区域内的**最大像素值**：\n"
           "$$dst(x, y) = \\max_{(i, j) \\in kernel} src(x+i, y+j)$$\n"
           "**效果**：扩大高亮（白色）目标连通域，填补前景物体内部孔洞与边缘缺口。";
}

bool DilateOp::Process(const cv::Mat& input, cv::Mat& output, std::string& errorMsg)
{
    if (input.empty()) { errorMsg = "输入图像为空"; return false; }
    int shapeIdx = _params[0].GetInt();
    int ksize = _params[1].GetInt();
    if (ksize % 2 == 0) ksize += 1;
    int iters = _params[2].GetInt();

    int cvShape = cv::MORPH_RECT;
    if (shapeIdx == 1) cvShape = cv::MORPH_CROSS;
    else if (shapeIdx == 2) cvShape = cv::MORPH_ELLIPSE;

    try
    {
        cv::Mat element = cv::getStructuringElement(cvShape, cv::Size(ksize, ksize));
        cv::dilate(input, output, element, cv::Point(-1, -1), iters);
        return true;
    }
    catch (const cv::Exception& e)
    {
        errorMsg = e.what();
        return false;
    }
}

std::string DilateOp::GenerateCppCode(const std::string& inVar, const std::string& outVar) const
{
    int shapeIdx = _params[0].GetInt();
    int ksize = _params[1].GetInt();
    if (ksize % 2 == 0) ksize += 1;
    int iters = _params[2].GetInt();
    std::string shapeStr = "cv::MORPH_RECT";
    if (shapeIdx == 1) shapeStr = "cv::MORPH_CROSS";
    else if (shapeIdx == 2) shapeStr = "cv::MORPH_ELLIPSE";

    std::stringstream ss;
    ss << "// 形态学膨胀参数配置\n"
       << "int morphShape = " << shapeStr << ";\n"
       << "int ksize = " << ksize << ";             // 结构元尺寸\n"
       << "int iterations = " << iters << ";        // 迭代次数\n"
       << "cv::Mat element = cv::getStructuringElement(morphShape, cv::Size(ksize, ksize));\n"
       << "cv::dilate(" << inVar << ", " << outVar << ", element, cv::Point(-1, -1), iterations);";
    return ss.str();
}

std::string DilateOp::GeneratePythonCode(const std::string& inVar, const std::string& outVar) const
{
    int shapeIdx = _params[0].GetInt();
    int ksize = _params[1].GetInt();
    if (ksize % 2 == 0) ksize += 1;
    int iters = _params[2].GetInt();
    std::string shapeStr = "cv2.MORPH_RECT";
    if (shapeIdx == 1) shapeStr = "cv2.MORPH_CROSS";
    else if (shapeIdx == 2) shapeStr = "cv2.MORPH_ELLIPSE";

    std::stringstream ss;
    ss << "# 形态学膨胀参数配置\n"
       << "morph_shape = " << shapeStr << "\n"
       << "ksize = (" << ksize << ", " << ksize << ")         # 结构元尺寸\n"
       << "iterations = " << iters << "             # 迭代次数\n"
       << "element = cv2.getStructuringElement(morph_shape, ksize)\n"
       << outVar << " = cv2.dilate(" << inVar << ", element, iterations=iterations)";
    return ss.str();
}

// ---------------- ErodeOp ----------------
ErodeOp::ErodeOp()
{
    OpParameter shapeParam;
    shapeParam.Name = "shape";
    shapeParam.DisplayName = "结构元形状 (Shape)";
    shapeParam.Type = ParamType::ComboBox;
    shapeParam.Value = 0;
    shapeParam.EnumOptions = { "矩形 (Rect)", "十字交叉 (Cross)", "椭圆 (Ellipse)" };
    _params.push_back(shapeParam);

    OpParameter ksizeParam;
    ksizeParam.Name = "ksize";
    ksizeParam.DisplayName = "结构元尺寸 (Size)";
    ksizeParam.Type = ParamType::IntSlider;
    ksizeParam.Value = 3;
    ksizeParam.MinValue = 1;
    ksizeParam.MaxValue = 31;
    ksizeParam.Step = 2;
    _params.push_back(ksizeParam);

    OpParameter iterParam;
    iterParam.Name = "iterations";
    iterParam.DisplayName = "迭代次数 (Iterations)";
    iterParam.Type = ParamType::IntSlider;
    iterParam.Value = 1;
    iterParam.MinValue = 1;
    iterParam.MaxValue = 10;
    iterParam.Step = 1;
    _params.push_back(iterParam);
}

std::string ErodeOp::GetPrincipleDoc() const
{
    return "### 📘 算子原理：形态学腐蚀 (erode)\n\n"
           "用结构元核在图像上滑动，输出为结构元覆盖区域内的**最小像素值**：\n"
           "$$dst(x, y) = \\min_{(i, j) \\in kernel} src(x+i, y+j)$$\n"
           "**效果**：消除细小毛刺与噪点孤岛，分离粘连的前景物体。";
}

bool ErodeOp::Process(const cv::Mat& input, cv::Mat& output, std::string& errorMsg)
{
    if (input.empty()) { errorMsg = "输入图像为空"; return false; }
    int shapeIdx = _params[0].GetInt();
    int ksize = _params[1].GetInt();
    if (ksize % 2 == 0) ksize += 1;
    int iters = _params[2].GetInt();

    int cvShape = cv::MORPH_RECT;
    if (shapeIdx == 1) cvShape = cv::MORPH_CROSS;
    else if (shapeIdx == 2) cvShape = cv::MORPH_ELLIPSE;

    try
    {
        cv::Mat element = cv::getStructuringElement(cvShape, cv::Size(ksize, ksize));
        cv::erode(input, output, element, cv::Point(-1, -1), iters);
        return true;
    }
    catch (const cv::Exception& e)
    {
        errorMsg = e.what();
        return false;
    }
}

std::string ErodeOp::GenerateCppCode(const std::string& inVar, const std::string& outVar) const
{
    int shapeIdx = _params[0].GetInt();
    int ksize = _params[1].GetInt();
    if (ksize % 2 == 0) ksize += 1;
    int iters = _params[2].GetInt();
    std::string shapeStr = "cv::MORPH_RECT";
    if (shapeIdx == 1) shapeStr = "cv::MORPH_CROSS";
    else if (shapeIdx == 2) shapeStr = "cv::MORPH_ELLIPSE";

    std::stringstream ss;
    ss << "// 形态学腐蚀参数配置\n"
       << "int morphShape = " << shapeStr << ";\n"
       << "int ksize = " << ksize << ";             // 结构元尺寸\n"
       << "int iterations = " << iters << ";        // 迭代次数\n"
       << "cv::Mat element = cv::getStructuringElement(morphShape, cv::Size(ksize, ksize));\n"
       << "cv::erode(" << inVar << ", " << outVar << ", element, cv::Point(-1, -1), iterations);";
    return ss.str();
}

std::string ErodeOp::GeneratePythonCode(const std::string& inVar, const std::string& outVar) const
{
    int shapeIdx = _params[0].GetInt();
    int ksize = _params[1].GetInt();
    if (ksize % 2 == 0) ksize += 1;
    int iters = _params[2].GetInt();
    std::string shapeStr = "cv2.MORPH_RECT";
    if (shapeIdx == 1) shapeStr = "cv2.MORPH_CROSS";
    else if (shapeIdx == 2) shapeStr = "cv2.MORPH_ELLIPSE";

    std::stringstream ss;
    ss << "# 形态学腐蚀参数配置\n"
       << "morph_shape = " << shapeStr << "\n"
       << "ksize = (" << ksize << ", " << ksize << ")         # 结构元尺寸\n"
       << "iterations = " << iters << "             # 迭代次数\n"
       << "element = cv2.getStructuringElement(morph_shape, ksize)\n"
       << outVar << " = cv2.erode(" << inVar << ", element, iterations=iterations)";
    return ss.str();
}

// ---------------- MorphologyExOp ----------------
MorphologyExOp::MorphologyExOp()
{
    OpParameter opParam;
    opParam.Name = "op";
    opParam.DisplayName = "形态学操作类型 (Operation)";
    opParam.Type = ParamType::ComboBox;
    opParam.Value = 0; // Open
    opParam.EnumOptions = {
        "开运算 (Open - 先腐蚀后膨胀: 消除孤立点)",
        "闭运算 (Close - 先膨胀后腐蚀: 填充微小孔洞)",
        "形态学梯度 (Gradient - 膨胀减腐蚀: 提取边缘轮廓)",
        "顶帽运算 (TopHat - 原图减开运算: 提取高亮细小特征)",
        "黑帽运算 (BlackHat - 闭运算减原图: 提取暗色细小特征)",
        "击中击不中 (HitMiss - 形状精确结构模式匹配)"
    };
    _params.push_back(opParam);

    OpParameter shapeParam;
    shapeParam.Name = "shape";
    shapeParam.DisplayName = "结构元形状 (Shape)";
    shapeParam.Type = ParamType::ComboBox;
    shapeParam.Value = 0;
    shapeParam.EnumOptions = { "矩形 (Rect)", "十字交叉 (Cross)", "椭圆 (Ellipse)" };
    _params.push_back(shapeParam);

    OpParameter ksizeParam;
    ksizeParam.Name = "ksize";
    ksizeParam.DisplayName = "结构元尺寸 (Size)";
    ksizeParam.Type = ParamType::IntSlider;
    ksizeParam.Value = 5;
    ksizeParam.MinValue = 1;
    ksizeParam.MaxValue = 31;
    ksizeParam.Step = 2;
    _params.push_back(ksizeParam);
}

std::string MorphologyExOp::GetPrincipleDoc() const
{
    return "### 📘 算子原理：高级形态学变换 (morphologyEx)\n\n"
           "* **开运算 (Open)**：$dst = open(src, element) = dilate(erode(src))$\n"
           "* **闭运算 (Close)**：$dst = close(src, element) = erode(dilate(src))$\n"
           "* **形态学梯度 (Gradient)**：$dst = dilate(src) - erode(src)$\n"
           "* **顶帽 (TopHat)**：$dst = src - open(src)$\n"
           "* **黑帽 (BlackHat)**：$dst = close(src) - src$";
}

bool MorphologyExOp::Process(const cv::Mat& input, cv::Mat& output, std::string& errorMsg)
{
    if (input.empty()) { errorMsg = "输入图像为空"; return false; }
    int opIdx = _params[0].GetInt();
    int shapeIdx = _params[1].GetInt();
    int ksize = _params[2].GetInt();
    if (ksize % 2 == 0) ksize += 1;

    int cvOp = cv::MORPH_OPEN;
    if (opIdx == 1) cvOp = cv::MORPH_CLOSE;
    else if (opIdx == 2) cvOp = cv::MORPH_GRADIENT;
    else if (opIdx == 3) cvOp = cv::MORPH_TOPHAT;
    else if (opIdx == 4) cvOp = cv::MORPH_BLACKHAT;
    else if (opIdx == 5) cvOp = cv::MORPH_HITMISS;

    int cvShape = cv::MORPH_RECT;
    if (shapeIdx == 1) cvShape = cv::MORPH_CROSS;
    else if (shapeIdx == 2) cvShape = cv::MORPH_ELLIPSE;

    try
    {
        cv::Mat element = cv::getStructuringElement(cvShape, cv::Size(ksize, ksize));
        cv::morphologyEx(input, output, cvOp, element);
        return true;
    }
    catch (const cv::Exception& e)
    {
        errorMsg = e.what();
        return false;
    }
}

std::string MorphologyExOp::GenerateCppCode(const std::string& inVar, const std::string& outVar) const
{
    int opIdx = _params[0].GetInt();
    int shapeIdx = _params[1].GetInt();
    int ksize = _params[2].GetInt();
    if (ksize % 2 == 0) ksize += 1;

    std::string opStr = "cv::MORPH_OPEN";
    std::string desc = "开运算 (先腐蚀后膨胀)";
    if (opIdx == 1) { opStr = "cv::MORPH_CLOSE"; desc = "闭运算 (先膨胀后腐蚀)"; }
    else if (opIdx == 2) { opStr = "cv::MORPH_GRADIENT"; desc = "形态学梯度 (提取边缘)"; }
    else if (opIdx == 3) { opStr = "cv::MORPH_TOPHAT"; desc = "顶帽运算"; }
    else if (opIdx == 4) { opStr = "cv::MORPH_BLACKHAT"; desc = "黑帽运算"; }
    else if (opIdx == 5) { opStr = "cv::MORPH_HITMISS"; desc = "击中击不中"; }

    std::string shapeStr = (shapeIdx == 1) ? "cv::MORPH_CROSS" : (shapeIdx == 2 ? "cv::MORPH_ELLIPSE" : "cv::MORPH_RECT");

    std::stringstream ss;
    ss << "// 高级形态学操作: " << desc << "\n"
       << "int morphOp = " << opStr << ";\n"
       << "int morphShape = " << shapeStr << ";\n"
       << "int ksize = " << ksize << ";             // 结构元尺寸\n"
       << "cv::Mat element = cv::getStructuringElement(morphShape, cv::Size(ksize, ksize));\n"
       << "cv::morphologyEx(" << inVar << ", " << outVar << ", morphOp, element);";
    return ss.str();
}

std::string MorphologyExOp::GeneratePythonCode(const std::string& inVar, const std::string& outVar) const
{
    int opIdx = _params[0].GetInt();
    int shapeIdx = _params[1].GetInt();
    int ksize = _params[2].GetInt();
    if (ksize % 2 == 0) ksize += 1;

    std::string opStr = "cv2.MORPH_OPEN";
    std::string desc = "开运算 (先腐蚀后膨胀)";
    if (opIdx == 1) { opStr = "cv2.MORPH_CLOSE"; desc = "闭运算"; }
    else if (opIdx == 2) { opStr = "cv2.MORPH_GRADIENT"; desc = "形态学梯度"; }
    else if (opIdx == 3) { opStr = "cv2.MORPH_TOPHAT"; desc = "顶帽运算"; }
    else if (opIdx == 4) { opStr = "cv2.MORPH_BLACKHAT"; desc = "黑帽运算"; }
    else if (opIdx == 5) { opStr = "cv2.MORPH_HITMISS"; desc = "击中击不中"; }

    std::string shapeStr = (shapeIdx == 1) ? "cv2.MORPH_CROSS" : (shapeIdx == 2 ? "cv2.MORPH_ELLIPSE" : "cv2.MORPH_RECT");

    std::stringstream ss;
    ss << "# 高级形态学操作: " << desc << "\n"
       << "morph_op = " << opStr << "\n"
       << "morph_shape = " << shapeStr << "\n"
       << "ksize = (" << ksize << ", " << ksize << ")         # 结构元尺寸\n"
       << "element = cv2.getStructuringElement(morph_shape, ksize)\n"
       << outVar << " = cv2.morphologyEx(" << inVar << ", morph_op, element)";
    return ss.str();
}
