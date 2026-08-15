#include "ThresholdOps.h"
#include <sstream>

// ---------------- ThresholdOp ----------------
ThresholdOp::ThresholdOp()
{
    OpParameter threshParam;
    threshParam.Name = "thresh";
    threshParam.DisplayName = "阈值 (Thresh)";
    threshParam.Tooltip = "分割阈值大小 (0~255)";
    threshParam.Type = ParamType::IntSlider;
    threshParam.Value = 127;
    threshParam.MinValue = 0;
    threshParam.MaxValue = 255;
    threshParam.Step = 1;
    _params.push_back(threshParam);

    OpParameter maxValParam;
    maxValParam.Name = "maxval";
    maxValParam.DisplayName = "最大值 (MaxVal)";
    maxValParam.Tooltip = "二值化时的上限像素值 (通常为 255)";
    maxValParam.Type = ParamType::IntSlider;
    maxValParam.Value = 255;
    maxValParam.MinValue = 0;
    maxValParam.MaxValue = 255;
    maxValParam.Step = 1;
    _params.push_back(maxValParam);

    OpParameter typeParam;
    typeParam.Name = "type";
    typeParam.DisplayName = "阈值类型 (Type)";
    typeParam.Tooltip = "选择阈值比较与赋值规则";
    typeParam.Type = ParamType::ComboBox;
    typeParam.Value = 0;
    typeParam.EnumOptions = {
        "THRESH_BINARY (大于阈值设为MaxVal，否则设为0)",
        "THRESH_BINARY_INV (大于阈值设为0，否则设为MaxVal)",
        "THRESH_TRUNC (大于阈值设为阈值，否则保持不变)",
        "THRESH_TOZERO (大于阈值保持不变，否则设为0)",
        "THRESH_TOZERO_INV (大于阈值设为0，否则保持不变)"
    };
    _params.push_back(typeParam);
}

std::string ThresholdOp::GetPrincipleDoc() const
{
    return "### 📘 算子原理：固定阈值二值化 (threshold)\n\n"
           "固定阈值对单通道灰度图每个像素进行条件判断：\n"
           "* **BINARY**：$\\text{dst}(x,y) = \\begin{cases} \\text{maxval}, & \\text{src}(x,y) > \\text{thresh} \\\\ 0, & \\text{otherwise} \\end{cases}$\n"
           "* **BINARY_INV**：$\\text{dst}(x,y) = \\begin{cases} 0, & \\text{src}(x,y) > \\text{thresh} \\\\ \\text{maxval}, & \\text{otherwise} \\end{cases}$\n"
           "* **TRUNC**：截断超出阈值的高亮像素。\n"
           "* **TOZERO**：将低于阈值的暗部直接置黑。";
}

bool ThresholdOp::Process(const cv::Mat& input, cv::Mat& output, std::string& errorMsg)
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

    double thresh = _params[0].GetInt();
    double maxval = _params[1].GetInt();
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
    double thresh = _params[0].GetInt();
    double maxval = _params[1].GetInt();
    int typeIdx = _params[2].GetInt();
    std::string typeStr = "cv::THRESH_BINARY";
    if (typeIdx == 1) typeStr = "cv::THRESH_BINARY_INV";
    else if (typeIdx == 2) typeStr = "cv::THRESH_TRUNC";
    else if (typeIdx == 3) typeStr = "cv::THRESH_TOZERO";
    else if (typeIdx == 4) typeStr = "cv::THRESH_TOZERO_INV";

    std::stringstream ss;
    ss << "cv::Mat gray_" << outVar << ";\n"
       << "if (" << inVar << ".channels() > 1) cv::cvtColor(" << inVar << ", gray_" << outVar << ", cv::COLOR_BGR2GRAY);\n"
       << "else gray_" << outVar << " = " << inVar << ";\n"
       << "cv::threshold(gray_" << outVar << ", " << outVar << ", " << thresh << ", " << maxval << ", " << typeStr << ");";
    return ss.str();
}

std::string ThresholdOp::GeneratePythonCode(const std::string& inVar, const std::string& outVar) const
{
    double thresh = _params[0].GetInt();
    double maxval = _params[1].GetInt();
    int typeIdx = _params[2].GetInt();
    std::string typeStr = "cv2.THRESH_BINARY";
    if (typeIdx == 1) typeStr = "cv2.THRESH_BINARY_INV";
    else if (typeIdx == 2) typeStr = "cv2.THRESH_TRUNC";
    else if (typeIdx == 3) typeStr = "cv2.THRESH_TOZERO";
    else if (typeIdx == 4) typeStr = "cv2.THRESH_TOZERO_INV";

    std::stringstream ss;
    ss << "gray_" << outVar << " = cv2.cvtColor(" << inVar << ", cv2.COLOR_BGR2GRAY) if len(" << inVar << ".shape) == 3 else " << inVar << "\n"
       << "_, " << outVar << " = cv2.threshold(gray_" << outVar << ", " << thresh << ", " << maxval << ", " << typeStr << ")";
    return ss.str();
}

// ---------------- OtsuThresholdOp ----------------
OtsuThresholdOp::OtsuThresholdOp()
{
    OpParameter maxValParam;
    maxValParam.Name = "maxval";
    maxValParam.DisplayName = "最大值 (MaxVal)";
    maxValParam.Tooltip = "二值化时的上限像素值 (通常为 255)";
    maxValParam.Type = ParamType::IntSlider;
    maxValParam.Value = 255;
    maxValParam.MinValue = 0;
    maxValParam.MaxValue = 255;
    maxValParam.Step = 1;
    _params.push_back(maxValParam);

    OpParameter invParam;
    invParam.Name = "invert";
    invParam.DisplayName = "反转二值化 (Invert)";
    invParam.Tooltip = "是否使用 THRESH_BINARY_INV 模式";
    invParam.Type = ParamType::CheckBox;
    invParam.Value = false;
    _params.push_back(invParam);
}

std::string OtsuThresholdOp::GetPrincipleDoc() const
{
    return "### 📘 算子原理：Otsu 大津算法 (THRESH_OTSU)\n\n"
           "大津算法（最大类间方差法）是一种**自适应确定全局二值化阈值**的无监督方法：\n"
           "1. 统计图像的灰度直方图；\n"
           "2. 遍历所有可能阈值 $T$ (0~255)，将图像分成前景 $\\omega_0$ 和背景 $\\omega_1$ 两组；\n"
           "3. 计算类间方差 $\\sigma_B^2(T) = \\omega_0 \\omega_1 (\\mu_0 - \\mu_1)^2$；\n"
           "4. 找到使类间方差最大的阈值作为最佳分割点。\n\n"
           "适用于**双峰直方图**图像（目标与背景灰度对比鲜明）。";
}

bool OtsuThresholdOp::Process(const cv::Mat& input, cv::Mat& output, std::string& errorMsg)
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

    double maxval = _params[0].GetInt();
    bool invert = _params[1].GetBool();
    int flag = cv::THRESH_OTSU | (invert ? cv::THRESH_BINARY_INV : cv::THRESH_BINARY);

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
    double maxval = _params[0].GetInt();
    bool invert = _params[1].GetBool();
    std::string flagStr = invert ? "cv::THRESH_BINARY_INV | cv::THRESH_OTSU" : "cv::THRESH_BINARY | cv::THRESH_OTSU";

    std::stringstream ss;
    ss << "cv::Mat gray_" << outVar << ";\n"
       << "if (" << inVar << ".channels() > 1) cv::cvtColor(" << inVar << ", gray_" << outVar << ", cv::COLOR_BGR2GRAY);\n"
       << "else gray_" << outVar << " = " << inVar << ";\n"
       << "cv::threshold(gray_" << outVar << ", " << outVar << ", 0, " << maxval << ", " << flagStr << ");";
    return ss.str();
}

std::string OtsuThresholdOp::GeneratePythonCode(const std::string& inVar, const std::string& outVar) const
{
    double maxval = _params[0].GetInt();
    bool invert = _params[1].GetBool();
    std::string flagStr = invert ? "cv2.THRESH_BINARY_INV + cv2.THRESH_OTSU" : "cv2.THRESH_BINARY + cv2.THRESH_OTSU";

    std::stringstream ss;
    ss << "gray_" << outVar << " = cv2.cvtColor(" << inVar << ", cv2.COLOR_BGR2GRAY) if len(" << inVar << ".shape) == 3 else " << inVar << "\n"
       << "_, " << outVar << " = cv2.threshold(gray_" << outVar << ", 0, " << maxval << ", " << flagStr << ")";
    return ss.str();
}

// ---------------- AdaptiveThresholdOp ----------------
AdaptiveThresholdOp::AdaptiveThresholdOp()
{
    OpParameter maxValParam;
    maxValParam.Name = "maxval";
    maxValParam.DisplayName = "最大值 (MaxVal)";
    maxValParam.Tooltip = "满足条件时赋予的最大像素值";
    maxValParam.Type = ParamType::IntSlider;
    maxValParam.Value = 255;
    maxValParam.MinValue = 0;
    maxValParam.MaxValue = 255;
    maxValParam.Step = 1;
    _params.push_back(maxValParam);

    OpParameter methodParam;
    methodParam.Name = "adaptiveMethod";
    methodParam.DisplayName = "自适应方法 (Method)";
    methodParam.Tooltip = "局部阈值计算算法";
    methodParam.Type = ParamType::ComboBox;
    methodParam.Value = 1; // Gaussian
    methodParam.EnumOptions = {
        "ADAPTIVE_THRESH_MEAN_C (邻域均值)",
        "ADAPTIVE_THRESH_GAUSSIAN_C (邻域高斯加权均值)"
    };
    _params.push_back(methodParam);

    OpParameter blockSizeParam;
    blockSizeParam.Name = "blockSize";
    blockSizeParam.DisplayName = "邻域块大小 (BlockSize)";
    blockSizeParam.Tooltip = "计算局部阈值的邻域尺寸 (必须为奇数，如 3, 5, 7, 11, 21...)";
    blockSizeParam.Type = ParamType::IntSlider;
    blockSizeParam.Value = 11;
    blockSizeParam.MinValue = 3;
    blockSizeParam.MaxValue = 99;
    blockSizeParam.Step = 2;
    _params.push_back(blockSizeParam);

    OpParameter cParam;
    cParam.Name = "C";
    cParam.DisplayName = "常数偏置 (C)";
    cParam.Tooltip = "从均值或加权均值中减去的常数 (微调阈值灵敏度)";
    cParam.Type = ParamType::DoubleSlider;
    cParam.Value = 2.0;
    cParam.MinValue = -20.0;
    cParam.MaxValue = 20.0;
    cParam.Step = 0.5;
    _params.push_back(cParam);
}

std::string AdaptiveThresholdOp::GetPrincipleDoc() const
{
    return "### 📘 算子原理：自适应局部阈值 (adaptiveThreshold)\n\n"
           "当图像各区域光照不均匀、阴影严重时，单一全局阈值往往无法兼顾全局。\n"
           "自适应阈值为每个像素计算其 `BlockSize x BlockSize` 邻域内的统计量 $T(x,y)$，再减去常数 $C$ 作为该像素的专属阈值：\n"
           "* **MEAN_C**：邻域所有像素的简单算术平均值；\n"
           "* **GAUSSIAN_C**：邻域高斯加权平均值（越靠近中心权重越高，抗噪性能更好）。";
}

bool AdaptiveThresholdOp::Process(const cv::Mat& input, cv::Mat& output, std::string& errorMsg)
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

    double maxval = _params[0].GetInt();
    int methodIdx = _params[1].GetInt();
    int blockSize = _params[2].GetInt();
    if (blockSize % 2 == 0) { blockSize += 1; }
    if (blockSize < 3) { blockSize = 3; }
    double c = _params[3].GetDouble();

    int cvMethod = (methodIdx == 0) ? cv::ADAPTIVE_THRESH_MEAN_C : cv::ADAPTIVE_THRESH_GAUSSIAN_C;

    try
    {
        cv::adaptiveThreshold(gray, output, maxval, cvMethod, cv::THRESH_BINARY, blockSize, c);
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
    double maxval = _params[0].GetInt();
    int methodIdx = _params[1].GetInt();
    int blockSize = _params[2].GetInt();
    if (blockSize % 2 == 0) { blockSize += 1; }
    double c = _params[3].GetDouble();
    std::string methodStr = (methodIdx == 0) ? "cv::ADAPTIVE_THRESH_MEAN_C" : "cv::ADAPTIVE_THRESH_GAUSSIAN_C";

    std::stringstream ss;
    ss << "cv::Mat gray_" << outVar << ";\n"
       << "if (" << inVar << ".channels() > 1) cv::cvtColor(" << inVar << ", gray_" << outVar << ", cv::COLOR_BGR2GRAY);\n"
       << "else gray_" << outVar << " = " << inVar << ";\n"
       << "cv::adaptiveThreshold(gray_" << outVar << ", " << outVar << ", " << maxval << ", " << methodStr << ", cv::THRESH_BINARY, " << blockSize << ", " << c << ");";
    return ss.str();
}

std::string AdaptiveThresholdOp::GeneratePythonCode(const std::string& inVar, const std::string& outVar) const
{
    double maxval = _params[0].GetInt();
    int methodIdx = _params[1].GetInt();
    int blockSize = _params[2].GetInt();
    if (blockSize % 2 == 0) { blockSize += 1; }
    double c = _params[3].GetDouble();
    std::string methodStr = (methodIdx == 0) ? "cv2.ADAPTIVE_THRESH_MEAN_C" : "cv2.ADAPTIVE_THRESH_GAUSSIAN_C";

    std::stringstream ss;
    ss << "gray_" << outVar << " = cv2.cvtColor(" << inVar << ", cv2.COLOR_BGR2GRAY) if len(" << inVar << ".shape) == 3 else " << inVar << "\n"
       << outVar << " = cv2.adaptiveThreshold(gray_" << outVar << ", " << maxval << ", " << methodStr << ", cv2.THRESH_BINARY, " << blockSize << ", " << c << ")";
    return ss.str();
}

// ---------------- InRangeOp ----------------
InRangeOp::InRangeOp()
{
    OpParameter lowH;
    lowH.Name = "lowerH";
    lowH.DisplayName = "通道1 下限 (Lower 1/H)";
    lowH.Tooltip = "第一个通道的下限阈值 (HSV下为色调H: 0~180)";
    lowH.Type = ParamType::IntSlider;
    lowH.Value = 35;
    lowH.MinValue = 0;
    lowH.MaxValue = 255;
    lowH.Step = 1;
    _params.push_back(lowH);

    OpParameter highH;
    highH.Name = "upperH";
    highH.DisplayName = "通道1 上限 (Upper 1/H)";
    highH.Tooltip = "第一个通道的上限阈值";
    highH.Type = ParamType::IntSlider;
    highH.Value = 85;
    highH.MinValue = 0;
    highH.MaxValue = 255;
    highH.Step = 1;
    _params.push_back(highH);

    OpParameter lowS;
    lowS.Name = "lowerS";
    lowS.DisplayName = "通道2 下限 (Lower 2/S)";
    lowS.Tooltip = "第二个通道的下限阈值 (HSV下为饱和度S: 0~255)";
    lowS.Type = ParamType::IntSlider;
    lowS.Value = 43;
    lowS.MinValue = 0;
    lowS.MaxValue = 255;
    lowS.Step = 1;
    _params.push_back(lowS);

    OpParameter highS;
    highS.Name = "upperS";
    highS.DisplayName = "通道2 上限 (Upper 2/S)";
    highS.Tooltip = "第二个通道的上限阈值";
    highS.Type = ParamType::IntSlider;
    highS.Value = 255;
    highS.MinValue = 0;
    highS.MaxValue = 255;
    highS.Step = 1;
    _params.push_back(highS);

    OpParameter lowV;
    lowV.Name = "lowerV";
    lowV.DisplayName = "通道3 下限 (Lower 3/V)";
    lowV.Tooltip = "第三个通道的下限阈值 (HSV下为明度V: 0~255)";
    lowV.Type = ParamType::IntSlider;
    lowV.Value = 46;
    lowV.MinValue = 0;
    lowV.MaxValue = 255;
    lowV.Step = 1;
    _params.push_back(lowV);

    OpParameter highV;
    highV.Name = "upperV";
    highV.DisplayName = "通道3 上限 (Upper 3/V)";
    highV.Tooltip = "第三个通道的上限阈值";
    highV.Type = ParamType::IntSlider;
    highV.Value = 255;
    highV.MinValue = 0;
    highV.MaxValue = 255;
    highV.Step = 1;
    _params.push_back(highV);
}

std::string InRangeOp::GetPrincipleDoc() const
{
    return "### 📘 算子原理：颜色区间提取 (inRange)\n\n"
           "对多通道图像检查每个像素是否落在设定的上下限范围之内：\n"
           "$$\\text{dst}(x,y) = \\begin{cases} 255, & \\text{lowerb} \\le \\text{src}(x,y) \\le \\text{upperb} \\\\ 0, & \\text{otherwise} \\end{cases}$$\n"
           "**常用技巧**：通常在前面先插入 `CvtColor (BGR->HSV)` 节点，然后利用此算子精确提取特定颜色（如红色、绿色、蓝色目标）生成 Mask 掩膜。";
}

bool InRangeOp::Process(const cv::Mat& input, cv::Mat& output, std::string& errorMsg)
{
    if (input.empty())
    {
        errorMsg = "输入图像为空";
        return false;
    }

    if (input.channels() == 1)
    {
        cv::Scalar lower(_params[0].GetInt());
        cv::Scalar upper(_params[1].GetInt());
        cv::inRange(input, lower, upper, output);
    }
    else if (input.channels() >= 3)
    {
        cv::Scalar lower(_params[0].GetInt(), _params[2].GetInt(), _params[4].GetInt());
        cv::Scalar upper(_params[1].GetInt(), _params[3].GetInt(), _params[5].GetInt());
        cv::inRange(input, lower, upper, output);
    }
    return true;
}

std::string InRangeOp::GenerateCppCode(const std::string& inVar, const std::string& outVar) const
{
    int l1 = _params[0].GetInt(), u1 = _params[1].GetInt();
    int l2 = _params[2].GetInt(), u2 = _params[3].GetInt();
    int l3 = _params[4].GetInt(), u3 = _params[5].GetInt();

    std::stringstream ss;
    ss << "cv::Scalar lower(" << l1 << ", " << l2 << ", " << l3 << ");\n"
       << "cv::Scalar upper(" << u1 << ", " << u2 << ", " << u3 << ");\n"
       << "cv::inRange(" << inVar << ", lower, upper, " << outVar << ");";
    return ss.str();
}

std::string InRangeOp::GeneratePythonCode(const std::string& inVar, const std::string& outVar) const
{
    int l1 = _params[0].GetInt(), u1 = _params[1].GetInt();
    int l2 = _params[2].GetInt(), u2 = _params[3].GetInt();
    int l3 = _params[4].GetInt(), u3 = _params[5].GetInt();

    std::stringstream ss;
    ss << "lower = np.array([" << l1 << ", " << l2 << ", " << l3 << "], dtype=np.uint8)\n"
       << "upper = np.array([" << u1 << ", " << u2 << ", " << u3 << "], dtype=np.uint8)\n"
       << outVar << " = cv2.inRange(" << inVar << ", lower, upper)";
    return ss.str();
}
