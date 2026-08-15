#include "ThresholdOps.h"
#include <sstream>

// ---------------- ThresholdOp ----------------
ThresholdOp::ThresholdOp()
{
    OpParameter thParam;
    thParam.Name = "thresh";
    thParam.DisplayName = "阈值 (Threshold)";
    thParam.Type = ParamType::DoubleSlider;
    thParam.Value = 127.0;
    thParam.MinValue = 0.0;
    thParam.MaxValue = 255.0;
    thParam.Step = 1.0;
    _params.push_back(thParam);

    OpParameter maxParam;
    maxParam.Name = "maxval";
    maxParam.DisplayName = "最大灰度值 (MaxVal)";
    maxParam.Type = ParamType::DoubleSlider;
    maxParam.Value = 255.0;
    maxParam.MinValue = 0.0;
    maxParam.MaxValue = 255.0;
    maxParam.Step = 1.0;
    _params.push_back(maxParam);

    OpParameter typeParam;
    typeParam.Name = "type";
    typeParam.DisplayName = "阈值化类型 (Type)";
    typeParam.Type = ParamType::ComboBox;
    typeParam.Value = 0;
    typeParam.EnumOptions = {
        "Binary (高于阈值设为MaxVal)",
        "Binary Inverted (低于阈值设为MaxVal)",
        "Truncate (高于阈值截断为阈值)",
        "To Zero (低于阈值置零)",
        "To Zero Inverted (高于阈值置零)"
    };
    _params.push_back(typeParam);
}

std::string ThresholdOp::GetPrincipleDoc() const
{
    return "### 📘 算子原理：基础阈值二值化 (threshold)\n\n"
           "将单通道灰度图像转化为黑白二值图：\n"
           "* **THRESH_BINARY**：$dst(x, y) = \\begin{cases} maxval & \\text{if } src(x, y) > thresh \\\\ 0 & \\text{otherwise} \\end{cases}$\n"
           "* **THRESH_BINARY_INV**：$dst(x, y) = \\begin{cases} 0 & \\text{if } src(x, y) > thresh \\\\ maxval & \\text{otherwise} \\end{cases}$";
}

bool ThresholdOp::Process(const cv::Mat& input, cv::Mat& output, std::string& errorMsg)
{
    if (input.empty()) { errorMsg = "输入图像为空"; return false; }
    cv::Mat gray = input;
    if (input.channels() > 1)
    {
        cv::cvtColor(input, gray, cv::COLOR_BGR2GRAY);
    }

    double thresh = _params[0].GetDouble();
    double maxval = _params[1].GetDouble();
    int typeIdx = _params[2].GetInt();
    int cvType = cv::THRESH_BINARY;
    if (typeIdx == 1) cvType = cv::THRESH_BINARY_INV;
    else if (typeIdx == 2) cvType = cv::THRESH_TRUNC;
    else if (typeIdx == 3) cvType = cv::THRESH_TOZERO;
    else if (typeIdx == 4) cvType = cv::THRESH_TOZERO_INV;

    try
    {
        cv::threshold(gray, output, thresh, maxval, cvType);
        return true;
    }
    catch (const cv::Exception& e)
    {
        errorMsg = e.what();
        return false;
    }
}

std::string ThresholdOp::GenerateCppCode(const std::string& inVar, const std::string& outVar) const
{
    double thresh = _params[0].GetDouble();
    double maxval = _params[1].GetDouble();
    int typeIdx = _params[2].GetInt();
    std::string typeStr = "cv::THRESH_BINARY";
    if (typeIdx == 1) typeStr = "cv::THRESH_BINARY_INV";
    else if (typeIdx == 2) typeStr = "cv::THRESH_TRUNC";
    else if (typeIdx == 3) typeStr = "cv::THRESH_TOZERO";
    else if (typeIdx == 4) typeStr = "cv::THRESH_TOZERO_INV";

    std::stringstream ss;
    ss << "// 固定阈值分割参数配置\n"
       << "double thresholdValue = " << thresh << ";    // 目标分割阈值\n"
       << "double maxValue = " << maxval << ";            // 满足条件时的最大值\n"
       << "int thresholdType = " << typeStr << ";\n"
       << "cv::threshold(" << inVar << ", " << outVar << ", thresholdValue, maxValue, thresholdType);";
    return ss.str();
}

std::string ThresholdOp::GeneratePythonCode(const std::string& inVar, const std::string& outVar) const
{
    double thresh = _params[0].GetDouble();
    double maxval = _params[1].GetDouble();
    int typeIdx = _params[2].GetInt();
    std::string typeStr = "cv2.THRESH_BINARY";
    if (typeIdx == 1) typeStr = "cv2.THRESH_BINARY_INV";
    else if (typeIdx == 2) typeStr = "cv2.THRESH_TRUNC";
    else if (typeIdx == 3) typeStr = "cv2.THRESH_TOZERO";
    else if (typeIdx == 4) typeStr = "cv2.THRESH_TOZERO_INV";

    std::stringstream ss;
    ss << "# 固定阈值分割参数配置\n"
       << "thresh_val = " << thresh << "              # 目标分割阈值\n"
       << "max_val = " << maxval << "                 # 满足条件时的最大值\n"
       << "thresh_type = " << typeStr << "\n"
       << "_, " << outVar << " = cv2.threshold(" << inVar << ", thresh_val, max_val, thresh_type)";
    return ss.str();
}

// ---------------- OtsuThresholdOp ----------------
OtsuThresholdOp::OtsuThresholdOp()
{
    OpParameter maxParam;
    maxParam.Name = "maxval";
    maxParam.DisplayName = "最大灰度值 (MaxVal)";
    maxParam.Type = ParamType::DoubleSlider;
    maxParam.Value = 255.0;
    maxParam.MinValue = 0.0;
    maxParam.MaxValue = 255.0;
    maxParam.Step = 1.0;
    _params.push_back(maxParam);

    OpParameter invParam;
    invParam.Name = "invert";
    invParam.DisplayName = "反相二值化 (Invert)";
    invParam.Type = ParamType::CheckBox;
    invParam.Value = false;
    _params.push_back(invParam);
}

std::string OtsuThresholdOp::GetPrincipleDoc() const
{
    return "### 📘 算子原理：大津自适应阈值 (Otsu Threshold)\n\n"
           "根据整幅图像灰度直方图的**类间方差最大化**准则，自动计算出最佳全局全局二值化阈值。";
}

bool OtsuThresholdOp::Process(const cv::Mat& input, cv::Mat& output, std::string& errorMsg)
{
    if (input.empty()) { errorMsg = "输入图像为空"; return false; }
    cv::Mat gray = input;
    if (input.channels() > 1)
    {
        cv::cvtColor(input, gray, cv::COLOR_BGR2GRAY);
    }

    double maxval = _params[0].GetDouble();
    bool inv = _params[1].GetBool();
    int flag = cv::THRESH_OTSU | (inv ? cv::THRESH_BINARY_INV : cv::THRESH_BINARY);

    try
    {
        cv::threshold(gray, output, 0, maxval, flag);
        return true;
    }
    catch (const cv::Exception& e)
    {
        errorMsg = e.what();
        return false;
    }
}

std::string OtsuThresholdOp::GenerateCppCode(const std::string& inVar, const std::string& outVar) const
{
    double maxval = _params[0].GetDouble();
    bool inv = _params[1].GetBool();
    std::string typeStr = inv ? "cv::THRESH_BINARY_INV | cv::THRESH_OTSU" : "cv::THRESH_BINARY | cv::THRESH_OTSU";

    std::stringstream ss;
    ss << "// Otsu大津自适应阈值分割 (自动计算类间方差最大阈值)\n"
       << "double maxValue = " << maxval << ";\n"
       << "int thresholdType = " << typeStr << ";\n"
       << "double autoThresh = cv::threshold(" << inVar << ", " << outVar << ", 0, maxValue, thresholdType);";
    return ss.str();
}

std::string OtsuThresholdOp::GeneratePythonCode(const std::string& inVar, const std::string& outVar) const
{
    double maxval = _params[0].GetDouble();
    bool inv = _params[1].GetBool();
    std::string typeStr = inv ? "cv2.THRESH_BINARY_INV + cv2.THRESH_OTSU" : "cv2.THRESH_BINARY + cv2.THRESH_OTSU";

    std::stringstream ss;
    ss << "# Otsu大津自适应阈值分割 (自动计算类间方差最大阈值)\n"
       << "max_val = " << maxval << "\n"
       << "thresh_type = " << typeStr << "\n"
       << "auto_thresh, " << outVar << " = cv2.threshold(" << inVar << ", 0, max_val, thresh_type)";
    return ss.str();
}

// ---------------- AdaptiveThresholdOp ----------------
AdaptiveThresholdOp::AdaptiveThresholdOp()
{
    OpParameter maxParam;
    maxParam.Name = "maxval";
    maxParam.DisplayName = "最大灰度值 (MaxVal)";
    maxParam.Type = ParamType::DoubleSlider;
    maxParam.Value = 255.0;
    maxParam.MinValue = 0.0;
    maxParam.MaxValue = 255.0;
    maxParam.Step = 1.0;
    _params.push_back(maxParam);

    OpParameter methodParam;
    methodParam.Name = "method";
    methodParam.DisplayName = "自适应方法 (Method)";
    methodParam.Type = ParamType::ComboBox;
    methodParam.Value = 1; // Gaussian
    methodParam.EnumOptions = {
        "Mean (邻域均值)",
        "Gaussian (邻域高斯加权)"
    };
    _params.push_back(methodParam);

    OpParameter bsParam;
    bsParam.Name = "blockSize";
    bsParam.DisplayName = "邻域块大小 (BlockSize)";
    bsParam.Type = ParamType::IntSlider;
    bsParam.Value = 11;
    bsParam.MinValue = 3;
    bsParam.MaxValue = 99;
    bsParam.Step = 2;
    _params.push_back(bsParam);

    OpParameter cParam;
    cParam.Name = "C";
    cParam.DisplayName = "微调常数 (C)";
    cParam.Type = ParamType::DoubleSlider;
    cParam.Value = 2.0;
    cParam.MinValue = -20.0;
    cParam.MaxValue = 20.0;
    cParam.Step = 0.5;
    _params.push_back(cParam);
}

std::string AdaptiveThresholdOp::GetPrincipleDoc() const
{
    return "### 📘 算子原理：局部自适应阈值 (adaptiveThreshold)\n\n"
           "针对图像中**光照不均匀**、局部阴影问题，对图像每个像素独立计算其局部邻域的加权均值减去常数 $C$ 作为该像素的专属阈值。";
}

bool AdaptiveThresholdOp::Process(const cv::Mat& input, cv::Mat& output, std::string& errorMsg)
{
    if (input.empty()) { errorMsg = "输入图像为空"; return false; }
    cv::Mat gray = input;
    if (input.channels() > 1)
    {
        cv::cvtColor(input, gray, cv::COLOR_BGR2GRAY);
    }

    double maxval = _params[0].GetDouble();
    int methodIdx = _params[1].GetInt();
    int blockSize = _params[2].GetInt();
    if (blockSize % 2 == 0) blockSize += 1;
    double C = _params[3].GetDouble();

    int cvMethod = (methodIdx == 0) ? cv::ADAPTIVE_THRESH_MEAN_C : cv::ADAPTIVE_THRESH_GAUSSIAN_C;

    try
    {
        cv::adaptiveThreshold(gray, output, maxval, cvMethod, cv::THRESH_BINARY, blockSize, C);
        return true;
    }
    catch (const cv::Exception& e)
    {
        errorMsg = e.what();
        return false;
    }
}

std::string AdaptiveThresholdOp::GenerateCppCode(const std::string& inVar, const std::string& outVar) const
{
    double maxval = _params[0].GetDouble();
    int methodIdx = _params[1].GetInt();
    int blockSize = _params[2].GetInt();
    if (blockSize % 2 == 0) blockSize += 1;
    double C = _params[3].GetDouble();
    std::string methodStr = (methodIdx == 0) ? "cv::ADAPTIVE_THRESH_MEAN_C" : "cv::ADAPTIVE_THRESH_GAUSSIAN_C";

    std::stringstream ss;
    ss << "// 局部自适应阈值 (解决光照不均问题)\n"
       << "double maxValue = " << maxval << ";\n"
       << "int adaptiveMethod = " << methodStr << ";\n"
       << "int thresholdType = cv::THRESH_BINARY;\n"
       << "int blockSize = " << blockSize << ";            // 局部窗口大小 (奇数)\n"
       << "double C = " << C << ";                   // 常数偏移量\n"
       << "cv::adaptiveThreshold(" << inVar << ", " << outVar << ", maxValue, adaptiveMethod, thresholdType, blockSize, C);";
    return ss.str();
}

std::string AdaptiveThresholdOp::GeneratePythonCode(const std::string& inVar, const std::string& outVar) const
{
    double maxval = _params[0].GetDouble();
    int methodIdx = _params[1].GetInt();
    int blockSize = _params[2].GetInt();
    if (blockSize % 2 == 0) blockSize += 1;
    double C = _params[3].GetDouble();
    std::string methodStr = (methodIdx == 0) ? "cv2.ADAPTIVE_THRESH_MEAN_C" : "cv2.ADAPTIVE_THRESH_GAUSSIAN_C";

    std::stringstream ss;
    ss << "# 局部自适应阈值 (解决光照不均问题)\n"
       << "max_value = " << maxval << "\n"
       << "adaptive_method = " << methodStr << "\n"
       << "threshold_type = cv2.THRESH_BINARY\n"
       << "block_size = " << blockSize << "            # 局部窗口大小 (奇数)\n"
       << "c = " << C << "                     # 常数偏移量\n"
       << outVar << " = cv2.adaptiveThreshold(" << inVar << ", max_value, adaptive_method, threshold_type, block_size, c)";
    return ss.str();
}

// ---------------- InRangeOp ----------------
InRangeOp::InRangeOp()
{
    OpParameter hMinParam;
    hMinParam.Name = "hMin";
    hMinParam.DisplayName = "H 色调下限 (0~180)";
    hMinParam.Type = ParamType::IntSlider;
    hMinParam.Value = 35;
    hMinParam.MinValue = 0;
    hMinParam.MaxValue = 180;
    hMinParam.Step = 1;
    _params.push_back(hMinParam);

    OpParameter hMaxParam;
    hMaxParam.Name = "hMax";
    hMaxParam.DisplayName = "H 色调上限 (0~180)";
    hMaxParam.Type = ParamType::IntSlider;
    hMaxParam.Value = 85;
    hMaxParam.MinValue = 0;
    hMaxParam.MaxValue = 180;
    hMaxParam.Step = 1;
    _params.push_back(hMaxParam);

    OpParameter sMinParam;
    sMinParam.Name = "sMin";
    sMinParam.DisplayName = "S 饱和度下限 (0~255)";
    sMinParam.Type = ParamType::IntSlider;
    sMinParam.Value = 43;
    sMinParam.MinValue = 0;
    sMinParam.MaxValue = 255;
    sMinParam.Step = 1;
    _params.push_back(sMinParam);

    OpParameter sMaxParam;
    sMaxParam.Name = "sMax";
    sMaxParam.DisplayName = "S 饱和度上限 (0~255)";
    sMaxParam.Type = ParamType::IntSlider;
    sMaxParam.Value = 255;
    sMaxParam.MinValue = 0;
    sMaxParam.MaxValue = 255;
    sMaxParam.Step = 1;
    _params.push_back(sMaxParam);
}

std::string InRangeOp::GetPrincipleDoc() const
{
    return "### 📘 算子原理：颜色区间二值化 (inRange)\n\n"
           "基于 HSV 颜色空间提取特定颜色物体（如提取绿色目标）：\n"
           "$$dst(x, y) = \\begin{cases} 255 & \\text{if } lower(x,y) \\le src(x,y) \\le upper(x,y) \\\\ 0 & \\text{otherwise} \\end{cases}$$";
}

bool InRangeOp::Process(const cv::Mat& input, cv::Mat& output, std::string& errorMsg)
{
    if (input.empty()) { errorMsg = "输入图像为空"; return false; }
    cv::Mat hsv;
    if (input.channels() == 3)
    {
        cv::cvtColor(input, hsv, cv::COLOR_BGR2HSV);
    }
    else
    {
        errorMsg = "InRange 要求输入必须为 3 通道 BGR 图像";
        return false;
    }

    int hMin = _params[0].GetInt();
    int hMax = _params[1].GetInt();
    int sMin = _params[2].GetInt();
    int sMax = _params[3].GetInt();

    try
    {
        cv::inRange(hsv, cv::Scalar(hMin, sMin, 46), cv::Scalar(hMax, sMax, 255), output);
        return true;
    }
    catch (const cv::Exception& e)
    {
        errorMsg = e.what();
        return false;
    }
}

std::string InRangeOp::GenerateCppCode(const std::string& inVar, const std::string& outVar) const
{
    int hMin = _params[0].GetInt();
    int hMax = _params[1].GetInt();
    int sMin = _params[2].GetInt();
    int sMax = _params[3].GetInt();

    std::stringstream ss;
    ss << "// HSV颜色空间区间过滤 (提取特定颜色目标)\n"
       << "cv::Mat hsv_" << outVar << ";\n"
       << "cv::cvtColor(" << inVar << ", hsv_" << outVar << ", cv::COLOR_BGR2HSV);\n"
       << "cv::Scalar lowerBound(" << hMin << ", " << sMin << ", 46);\n"
       << "cv::Scalar upperBound(" << hMax << ", " << sMax << ", 255);\n"
       << "cv::inRange(hsv_" << outVar << ", lowerBound, upperBound, " << outVar << ");";
    return ss.str();
}

std::string InRangeOp::GeneratePythonCode(const std::string& inVar, const std::string& outVar) const
{
    int hMin = _params[0].GetInt();
    int hMax = _params[1].GetInt();
    int sMin = _params[2].GetInt();
    int sMax = _params[3].GetInt();

    std::stringstream ss;
    ss << "# HSV颜色空间区间过滤 (提取特定颜色目标)\n"
       << "hsv_" << outVar << " = cv2.cvtColor(" << inVar << ", cv2.COLOR_BGR2HSV)\n"
       << "lower_bound = np.array([" << hMin << ", " << sMin << ", 46], dtype=np.uint8)\n"
       << "upper_bound = np.array([" << hMax << ", " << sMax << ", 255], dtype=np.uint8)\n"
       << outVar << " = cv2.inRange(hsv_" << outVar << ", lower_bound, upper_bound)";
    return ss.str();
}
