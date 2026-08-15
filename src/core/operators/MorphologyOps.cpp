#include "MorphologyOps.h"
#include <sstream>

static cv::Mat GetStructuringElementFromParams(int shapeIdx, int ksize)
{
    int cvShape = cv::MORPH_RECT;
    if (shapeIdx == 1) cvShape = cv::MORPH_CROSS;
    else if (shapeIdx == 2) cvShape = cv::MORPH_ELLIPSE;

    if (ksize < 1) ksize = 1;
    return cv::getStructuringElement(cvShape, cv::Size(ksize, ksize));
}

// ---------------- DilateOp ----------------
DilateOp::DilateOp()
{
    OpParameter shapeParam;
    shapeParam.Name = "shape";
    shapeParam.DisplayName = "核形状 (Shape)";
    shapeParam.Tooltip = "结构元素几何形状";
    shapeParam.Type = ParamType::ComboBox;
    shapeParam.Value = 0;
    shapeParam.EnumOptions = { "矩形 (MORPH_RECT)", "十字形 (MORPH_CROSS)", "椭圆形 (MORPH_ELLIPSE)" };
    _params.push_back(shapeParam);

    OpParameter ksizeParam;
    ksizeParam.Name = "ksize";
    ksizeParam.DisplayName = "核大小 (Ksize)";
    ksizeParam.Tooltip = "结构元素尺寸";
    ksizeParam.Type = ParamType::IntSlider;
    ksizeParam.Value = 3;
    ksizeParam.MinValue = 1;
    ksizeParam.MaxValue = 31;
    ksizeParam.Step = 1;
    _params.push_back(ksizeParam);

    OpParameter iterParam;
    iterParam.Name = "iterations";
    iterParam.DisplayName = "迭代次数 (Iterations)";
    iterParam.Tooltip = "膨胀重复执行次数";
    iterParam.Type = ParamType::IntSlider;
    iterParam.Value = 1;
    iterParam.MinValue = 1;
    iterParam.MaxValue = 10;
    iterParam.Step = 1;
    _params.push_back(iterParam);
}

std::string DilateOp::GetPrincipleDoc() const
{
    return "### 📘 算子原理：图像膨胀 (dilate)\n\n"
           "膨胀是形态学基础运算之一：\n"
           "$$\\text{dst}(x,y) = \\max_{(x',y') \\in \\text{element}} \\text{src}(x+x', y+y')$$\n"
           "**核心效果**：\n"
           "1. 扩张图像中高亮白色区域；\n"
           "2. 填补物体内部由于二值化产生的微小黑色孔洞与缝隙；\n"
           "3. 连接临近的断裂细线。";
}

bool DilateOp::Process(const cv::Mat& input, cv::Mat& output, std::string& errorMsg)
{
    if (input.empty())
    {
        errorMsg = "输入图像为空";
        return false;
    }

    int shape = _params[0].GetInt();
    int ksize = _params[1].GetInt();
    int iter = _params[2].GetInt();
    cv::Mat element = GetStructuringElementFromParams(shape, ksize);

    try
    {
        cv::dilate(input, output, element, cv::Point(-1, -1), iter);
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
    int shape = _params[0].GetInt();
    int ksize = _params[1].GetInt();
    int iter = _params[2].GetInt();
    std::string shapeStr = "cv::MORPH_RECT";
    if (shape == 1) shapeStr = "cv::MORPH_CROSS";
    else if (shape == 2) shapeStr = "cv::MORPH_ELLIPSE";

    std::stringstream ss;
    ss << "cv::Mat kernel_" << outVar << " = cv::getStructuringElement(" << shapeStr << ", cv::Size(" << ksize << ", " << ksize << "));\n"
       << "cv::dilate(" << inVar << ", " << outVar << ", kernel_" << outVar << ", cv::Point(-1, -1), " << iter << ");";
    return ss.str();
}

std::string DilateOp::GeneratePythonCode(const std::string& inVar, const std::string& outVar) const
{
    int shape = _params[0].GetInt();
    int ksize = _params[1].GetInt();
    int iter = _params[2].GetInt();
    std::string shapeStr = "cv2.MORPH_RECT";
    if (shape == 1) shapeStr = "cv2.MORPH_CROSS";
    else if (shape == 2) shapeStr = "cv2.MORPH_ELLIPSE";

    std::stringstream ss;
    ss << "kernel_" << outVar << " = cv2.getStructuringElement(" << shapeStr << ", (" << ksize << ", " << ksize << "))\n"
       << outVar << " = cv2.dilate(" << inVar << ", kernel_" << outVar << ", iterations=" << iter << ")";
    return ss.str();
}

// ---------------- ErodeOp ----------------
ErodeOp::ErodeOp()
{
    OpParameter shapeParam;
    shapeParam.Name = "shape";
    shapeParam.DisplayName = "核形状 (Shape)";
    shapeParam.Tooltip = "结构元素几何形状";
    shapeParam.Type = ParamType::ComboBox;
    shapeParam.Value = 0;
    shapeParam.EnumOptions = { "矩形 (MORPH_RECT)", "十字形 (MORPH_CROSS)", "椭圆形 (MORPH_ELLIPSE)" };
    _params.push_back(shapeParam);

    OpParameter ksizeParam;
    ksizeParam.Name = "ksize";
    ksizeParam.DisplayName = "核大小 (Ksize)";
    ksizeParam.Tooltip = "结构元素尺寸";
    ksizeParam.Type = ParamType::IntSlider;
    ksizeParam.Value = 3;
    ksizeParam.MinValue = 1;
    ksizeParam.MaxValue = 31;
    ksizeParam.Step = 1;
    _params.push_back(ksizeParam);

    OpParameter iterParam;
    iterParam.Name = "iterations";
    iterParam.DisplayName = "迭代次数 (Iterations)";
    iterParam.Tooltip = "腐蚀重复执行次数";
    iterParam.Type = ParamType::IntSlider;
    iterParam.Value = 1;
    iterParam.MinValue = 1;
    iterParam.MaxValue = 10;
    iterParam.Step = 1;
    _params.push_back(iterParam);
}

std::string ErodeOp::GetPrincipleDoc() const
{
    return "### 📘 算子原理：图像腐蚀 (erode)\n\n"
           "腐蚀是形态学的收缩运算：\n"
           "$$\\text{dst}(x,y) = \\min_{(x',y') \\in \\text{element}} \\text{src}(x+x', y+y')$$\n"
           "**核心效果**：\n"
           "1. 收缩高亮前景区域；\n"
           "2. 消除二值图像中毛刺与细小孤立噪点；\n"
           "3. 切断粘连在一起的相邻目标。";
}

bool ErodeOp::Process(const cv::Mat& input, cv::Mat& output, std::string& errorMsg)
{
    if (input.empty())
    {
        errorMsg = "输入图像为空";
        return false;
    }

    int shape = _params[0].GetInt();
    int ksize = _params[1].GetInt();
    int iter = _params[2].GetInt();
    cv::Mat element = GetStructuringElementFromParams(shape, ksize);

    try
    {
        cv::erode(input, output, element, cv::Point(-1, -1), iter);
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
    int shape = _params[0].GetInt();
    int ksize = _params[1].GetInt();
    int iter = _params[2].GetInt();
    std::string shapeStr = "cv::MORPH_RECT";
    if (shape == 1) shapeStr = "cv::MORPH_CROSS";
    else if (shape == 2) shapeStr = "cv::MORPH_ELLIPSE";

    std::stringstream ss;
    ss << "cv::Mat kernel_" << outVar << " = cv::getStructuringElement(" << shapeStr << ", cv::Size(" << ksize << ", " << ksize << "));\n"
       << "cv::erode(" << inVar << ", " << outVar << ", kernel_" << outVar << ", cv::Point(-1, -1), " << iter << ");";
    return ss.str();
}

std::string ErodeOp::GeneratePythonCode(const std::string& inVar, const std::string& outVar) const
{
    int shape = _params[0].GetInt();
    int ksize = _params[1].GetInt();
    int iter = _params[2].GetInt();
    std::string shapeStr = "cv2.MORPH_RECT";
    if (shape == 1) shapeStr = "cv2.MORPH_CROSS";
    else if (shape == 2) shapeStr = "cv2.MORPH_ELLIPSE";

    std::stringstream ss;
    ss << "kernel_" << outVar << " = cv2.getStructuringElement(" << shapeStr << ", (" << ksize << ", " << ksize << "))\n"
       << outVar << " = cv2.erode(" << inVar << ", kernel_" << outVar << ", iterations=" << iter << ")";
    return ss.str();
}

// ---------------- MorphologyExOp ----------------
MorphologyExOp::MorphologyExOp()
{
    OpParameter opParam;
    opParam.Name = "op";
    opParam.DisplayName = "运算类型 (Operation)";
    opParam.Tooltip = "形态学高级操作类型";
    opParam.Type = ParamType::ComboBox;
    opParam.Value = 0;
    opParam.EnumOptions = {
        "开运算 (MORPH_OPEN 先腐蚀后膨胀-去噪)",
        "闭运算 (MORPH_CLOSE 先膨胀后腐蚀-填孔)",
        "形态学梯度 (MORPH_GRADIENT 膨胀减腐蚀-提取边缘)",
        "顶帽运算 (MORPH_TOPHAT 原图减开运算-提取高亮微细结构)",
        "黑帽运算 (MORPH_BLACKHAT 闭运算减原图-提取暗部孔洞)",
        "击中击不中 (MORPH_HITMISS 模板拓扑匹配)"
    };
    _params.push_back(opParam);

    OpParameter shapeParam;
    shapeParam.Name = "shape";
    shapeParam.DisplayName = "核形状 (Shape)";
    shapeParam.Tooltip = "结构元素几何形状";
    shapeParam.Type = ParamType::ComboBox;
    shapeParam.Value = 0;
    shapeParam.EnumOptions = { "矩形 (MORPH_RECT)", "十字形 (MORPH_CROSS)", "椭圆形 (MORPH_ELLIPSE)" };
    _params.push_back(shapeParam);

    OpParameter ksizeParam;
    ksizeParam.Name = "ksize";
    ksizeParam.DisplayName = "核大小 (Ksize)";
    ksizeParam.Tooltip = "结构元素尺寸";
    ksizeParam.Type = ParamType::IntSlider;
    ksizeParam.Value = 5;
    ksizeParam.MinValue = 1;
    ksizeParam.MaxValue = 31;
    ksizeParam.Step = 1;
    _params.push_back(ksizeParam);

    OpParameter iterParam;
    iterParam.Name = "iterations";
    iterParam.DisplayName = "迭代次数 (Iterations)";
    iterParam.Tooltip = "重复执行次数";
    iterParam.Type = ParamType::IntSlider;
    iterParam.Value = 1;
    iterParam.MinValue = 1;
    iterParam.MaxValue = 10;
    iterParam.Step = 1;
    _params.push_back(iterParam);
}

std::string MorphologyExOp::GetPrincipleDoc() const
{
    return "### 📘 算子原理：高级形态学变换 (morphologyEx)\n\n"
           "* **开运算 (Open)**：先腐蚀后膨胀 $\\text{open}(A) = (A \\ominus B) \\oplus B$，用于**消除细小噪点同时保持目标大小不变**。\n"
           "* **闭运算 (Close)**：先膨胀后腐蚀 $\\text{close}(A) = (A \\oplus B) \\ominus B$，用于**填补细小空洞并闭合目标轮廓缝隙**。\n"
           "* **形态学梯度 (Gradient)**：膨胀图减去腐蚀图，获得目标的**结构边缘轮廓**。\n"
           "* **顶帽 (TopHat)**：原图减去开运算，用于**提取不均匀背景下的微小高亮特征**。\n"
           "* **黑帽 (BlackHat)**：闭运算减去原图，用于**提取不均匀背景下的微小暗斑特征**。";
}

bool MorphologyExOp::Process(const cv::Mat& input, cv::Mat& output, std::string& errorMsg)
{
    if (input.empty())
    {
        errorMsg = "输入图像为空";
        return false;
    }

    int opIdx = _params[0].GetInt();
    int cvOp = cv::MORPH_OPEN;
    if (opIdx == 1) cvOp = cv::MORPH_CLOSE;
    else if (opIdx == 2) cvOp = cv::MORPH_GRADIENT;
    else if (opIdx == 3) cvOp = cv::MORPH_TOPHAT;
    else if (opIdx == 4) cvOp = cv::MORPH_BLACKHAT;
    else if (opIdx == 5) cvOp = cv::MORPH_HITMISS;

    int shape = _params[1].GetInt();
    int ksize = _params[2].GetInt();
    int iter = _params[3].GetInt();
    cv::Mat element = GetStructuringElementFromParams(shape, ksize);

    try
    {
        cv::morphologyEx(input, output, cvOp, element, cv::Point(-1, -1), iter);
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
    std::string opStr = "cv::MORPH_OPEN";
    if (opIdx == 1) opStr = "cv::MORPH_CLOSE";
    else if (opIdx == 2) opStr = "cv::MORPH_GRADIENT";
    else if (opIdx == 3) opStr = "cv::MORPH_TOPHAT";
    else if (opIdx == 4) opStr = "cv::MORPH_BLACKHAT";
    else if (opIdx == 5) opStr = "cv::MORPH_HITMISS";

    int shape = _params[1].GetInt();
    int ksize = _params[2].GetInt();
    int iter = _params[3].GetInt();
    std::string shapeStr = "cv::MORPH_RECT";
    if (shape == 1) shapeStr = "cv::MORPH_CROSS";
    else if (shape == 2) shapeStr = "cv::MORPH_ELLIPSE";

    std::stringstream ss;
    ss << "cv::Mat kernel_" << outVar << " = cv::getStructuringElement(" << shapeStr << ", cv::Size(" << ksize << ", " << ksize << "));\n"
       << "cv::morphologyEx(" << inVar << ", " << outVar << ", " << opStr << ", kernel_" << outVar << ", cv::Point(-1, -1), " << iter << ");";
    return ss.str();
}

std::string MorphologyExOp::GeneratePythonCode(const std::string& inVar, const std::string& outVar) const
{
    int opIdx = _params[0].GetInt();
    std::string opStr = "cv2.MORPH_OPEN";
    if (opIdx == 1) opStr = "cv2.MORPH_CLOSE";
    else if (opIdx == 2) opStr = "cv2.MORPH_GRADIENT";
    else if (opIdx == 3) opStr = "cv2.MORPH_TOPHAT";
    else if (opIdx == 4) opStr = "cv2.MORPH_BLACKHAT";
    else if (opIdx == 5) opStr = "cv2.MORPH_HITMISS";

    int shape = _params[1].GetInt();
    int ksize = _params[2].GetInt();
    int iter = _params[3].GetInt();
    std::string shapeStr = "cv2.MORPH_RECT";
    if (shape == 1) shapeStr = "cv2.MORPH_CROSS";
    else if (shape == 2) shapeStr = "cv2.MORPH_ELLIPSE";

    std::stringstream ss;
    ss << "kernel_" << outVar << " = cv2.getStructuringElement(" << shapeStr << ", (" << ksize << ", " << ksize << "))\n"
       << outVar << " = cv2.morphologyEx(" << inVar << ", " << opStr << ", kernel_" << outVar << ", iterations=" << iter << ")";
    return ss.str();
}
