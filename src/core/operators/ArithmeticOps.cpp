#include "ArithmeticOps.h"
#include "../PipelineContext.h"
#include <sstream>

// ============================================================================
// 1. BlendWithSourceOp (双图与原图算术逻辑复合运算)
// ============================================================================
BlendWithSourceOp::BlendWithSourceOp()
{
    // 0. 第二操作数来源
    OpParameter srcParam;
    srcParam.Name = "secondSource";
    srcParam.DisplayName = "第二操作数图像源 (Second Image Source)";
    srcParam.Type = ParamType::ComboBox;
    srcParam.Value = 0; // 原始输入图像
    srcParam.EnumOptions = {
        "原始输入图像 (Original Source Image)",
        "纯白色底板 (Solid White Canvas)",
        "纯黑色底板 (Solid Black Canvas)"
    };
    _params.push_back(srcParam);

    // 1. 运算模式
    OpParameter modeParam;
    modeParam.Name = "opMode";
    modeParam.DisplayName = "运算模式 (Operation Mode)";
    modeParam.Type = ParamType::ComboBox;
    modeParam.Value = 0; // 加权融合
    modeParam.EnumOptions = {
        "线性加权融合 (AddWeighted: α·A + β·B + γ)",
        "绝对值差分 (AbsDiff: |A - B| / 缺陷检测)",
        "按位与掩码相交 (Bitwise AND)",
        "按位或特征叠加 (Bitwise OR)",
        "按位异或边缘差异 (Bitwise XOR)",
        "饱和加法叠加 (cv::add)",
        "饱和减法剔除 (cv::subtract: A - B)",
        "取亮最大值融合 (cv::max)",
        "取暗最小值融合 (cv::min)"
    };
    _params.push_back(modeParam);

    // 2. Alpha
    OpParameter alphaParam;
    alphaParam.Name = "alpha";
    alphaParam.DisplayName = "当前图权重 (Alpha α)";
    alphaParam.Type = ParamType::DoubleSlider;
    alphaParam.Value = 0.5;
    alphaParam.MinValue = 0.0;
    alphaParam.MaxValue = 1.0;
    alphaParam.Step = 0.05;
    _params.push_back(alphaParam);

    // 3. Beta
    OpParameter betaParam;
    betaParam.Name = "beta";
    betaParam.DisplayName = "第二图权重 (Beta β)";
    betaParam.Type = ParamType::DoubleSlider;
    betaParam.Value = 0.5;
    betaParam.MinValue = 0.0;
    betaParam.MaxValue = 1.0;
    betaParam.Step = 0.05;
    _params.push_back(betaParam);

    // 4. Gamma 偏置
    OpParameter gammaParam;
    gammaParam.Name = "gamma";
    gammaParam.DisplayName = "亮度偏置增益 (Gamma γ)";
    gammaParam.Type = ParamType::IntSlider;
    gammaParam.Value = 0;
    gammaParam.MinValue = -100;
    gammaParam.MaxValue = 100;
    gammaParam.Step = 1;
    _params.push_back(gammaParam);
}

std::string BlendWithSourceOp::GetPrincipleDoc() const
{
    return "### 📘 算子原理：多图与原图算术逻辑复合运算 (Image Arithmetic)\n\n"
           "工业机器视觉中极度关键的复合分析运算：\n"
           "* **绝对值差分 (`cv::absdiff`)**：$$dst(x, y) = |src_1(x, y) - src_2(x, y)|$$\n"
           "  广泛用于工业印刷瑕疵检测、金样比对（Golden Template Comparison）与定位形变检测；\n"
           "* **加权融合 (`cv::addWeighted`)**：$$dst = \\alpha \\cdot src_1 + \\beta \\cdot src_2 + \\gamma$$\n"
           "  实现多波段打光图像的无缝合成与热力图/结果半透明叠加；\n"
           "* **按位运算 (`bitwise_and/or/xor`)**：实现掩码 ROI 提取与轮廓边界交并差集合运算。";
}

bool BlendWithSourceOp::Process(const cv::Mat& input, cv::Mat& output, std::string& errorMsg)
{
    if (input.empty()) { errorMsg = "输入图像为空"; return false; }

    int secSource = _params[0].GetInt();
    int opMode = _params[1].GetInt();
    double alpha = _params[2].GetDouble();
    double beta = _params[3].GetDouble();
    int gamma = _params[4].GetInt();

    cv::Mat secondImg;
    if (secSource == 0) // 原始输入图像
    {
        secondImg = PipelineContext::Instance().GetSourceImage();
        if (secondImg.empty()) secondImg = input.clone();
    }
    else if (secSource == 1) // 纯白
    {
        secondImg = cv::Mat(input.size(), input.type(), cv::Scalar::all(255));
    }
    else // 纯黑
    {
        secondImg = cv::Mat(input.size(), input.type(), cv::Scalar::all(0));
    }

    // 尺寸与通道一致性对齐
    if (secondImg.size() != input.size())
    {
        cv::resize(secondImg, secondImg, input.size());
    }
    if (secondImg.channels() != input.channels())
    {
        if (input.channels() == 3 && secondImg.channels() == 1)
        {
            cv::cvtColor(secondImg, secondImg, cv::COLOR_GRAY2BGR);
        }
        else if (input.channels() == 1 && secondImg.channels() == 3)
        {
            cv::cvtColor(secondImg, secondImg, cv::COLOR_BGR2GRAY);
        }
    }

    try
    {
        switch (opMode)
        {
            case 0: // AddWeighted
                cv::addWeighted(input, alpha, secondImg, beta, gamma, output);
                break;
            case 1: // AbsDiff
                cv::absdiff(input, secondImg, output);
                break;
            case 2: // Bitwise AND
                cv::bitwise_and(input, secondImg, output);
                break;
            case 3: // Bitwise OR
                cv::bitwise_or(input, secondImg, output);
                break;
            case 4: // Bitwise XOR
                cv::bitwise_xor(input, secondImg, output);
                break;
            case 5: // Add
                cv::add(input, secondImg, output);
                break;
            case 6: // Subtract
                cv::subtract(input, secondImg, output);
                break;
            case 7: // Max
                cv::max(input, secondImg, output);
                break;
            case 8: // Min
                cv::min(input, secondImg, output);
                break;
            default:
                output = input.clone();
                break;
        }
        return true;
    }
    catch (const cv::Exception& e)
    {
        errorMsg = e.what();
        return false;
    }
}

std::string BlendWithSourceOp::GenerateCppCode(const std::string& inVar, const std::string& outVar) const
{
    int opMode = _params[1].GetInt();
    double alpha = _params[2].GetDouble();
    double beta = _params[3].GetDouble();
    int gamma = _params[4].GetInt();

    std::stringstream ss;
    ss << "// 双图与原图算术复合运算 (模式: " << opMode << ")\n"
       << "cv::Mat secondImg = source_image; // 原始基准图/模板图\n"
       << "if (secondImg.size() != " << inVar << ".size()) cv::resize(secondImg, secondImg, " << inVar << ".size());\n";

    if (opMode == 0)
    {
        ss << "double alpha = " << alpha << ", beta = " << beta << ", gamma = " << gamma << ";\n"
           << "cv::addWeighted(" << inVar << ", alpha, secondImg, beta, gamma, " << outVar << ");";
    }
    else if (opMode == 1)
    {
        ss << "cv::absdiff(" << inVar << ", secondImg, " << outVar << "); // 绝对值差分缺陷检测";
    }
    else if (opMode == 2)
    {
        ss << "cv::bitwise_and(" << inVar << ", secondImg, " << outVar << "); // 按位与掩码相交";
    }
    else if (opMode == 3)
    {
        ss << "cv::bitwise_or(" << inVar << ", secondImg, " << outVar << ");";
    }
    else if (opMode == 4)
    {
        ss << "cv::bitwise_xor(" << inVar << ", secondImg, " << outVar << ");";
    }
    else if (opMode == 5)
    {
        ss << "cv::add(" << inVar << ", secondImg, " << outVar << ");";
    }
    else if (opMode == 6)
    {
        ss << "cv::subtract(" << inVar << ", secondImg, " << outVar << ");";
    }
    else if (opMode == 7)
    {
        ss << "cv::max(" << inVar << ", secondImg, " << outVar << ");";
    }
    else
    {
        ss << "cv::min(" << inVar << ", secondImg, " << outVar << ");";
    }
    return ss.str();
}

std::string BlendWithSourceOp::GeneratePythonCode(const std::string& inVar, const std::string& outVar) const
{
    int opMode = _params[1].GetInt();
    double alpha = _params[2].GetDouble();
    double beta = _params[3].GetDouble();
    int gamma = _params[4].GetInt();

    std::stringstream ss;
    ss << "# 双图与原图算术复合运算 (模式: " << opMode << ")\n"
       << "second_img = source_image\n"
       << "if second_img.shape[:2] != " << inVar << ".shape[:2]:\n"
       << "    second_img = cv2.resize(second_img, (" << inVar << ".shape[1], " << inVar << ".shape[0]))\n";

    if (opMode == 0)
    {
        ss << "alpha, beta, gamma = " << alpha << ", " << beta << ", " << gamma << "\n"
           << outVar << " = cv2.addWeighted(" << inVar << ", alpha, second_img, beta, gamma)";
    }
    else if (opMode == 1)
    {
        ss << outVar << " = cv2.absdiff(" << inVar << ", second_img) # 绝对值差分缺陷检测";
    }
    else if (opMode == 2)
    {
        ss << outVar << " = cv2.bitwise_and(" << inVar << ", second_img)";
    }
    else if (opMode == 3)
    {
        ss << outVar << " = cv2.bitwise_or(" << inVar << ", second_img)";
    }
    else if (opMode == 4)
    {
        ss << outVar << " = cv2.bitwise_xor(" << inVar << ", second_img)";
    }
    else if (opMode == 5)
    {
        ss << outVar << " = cv2.add(" << inVar << ", second_img)";
    }
    else if (opMode == 6)
    {
        ss << outVar << " = cv2.subtract(" << inVar << ", second_img)";
    }
    else if (opMode == 7)
    {
        ss << outVar << " = cv2.max(" << inVar << ", second_img)";
    }
    else
    {
        ss << outVar << " = cv2.min(" << inVar << ", second_img)";
    }
    return ss.str();
}

// ============================================================================
// 2. MaskApplyOp (掩码抠图与原图ROI提取)
// ============================================================================
MaskApplyOp::MaskApplyOp()
{
    OpParameter threshParam;
    threshParam.Name = "maskThresh";
    threshParam.DisplayName = "掩码生效判定阈值 (Mask Cutoff)";
    threshParam.Type = ParamType::IntSlider;
    threshParam.Value = 128;
    threshParam.MinValue = 1;
    threshParam.MaxValue = 254;
    threshParam.Step = 1;
    _params.push_back(threshParam);

    OpParameter invertParam;
    invertParam.Name = "invertMask";
    invertParam.DisplayName = "掩码反转 (Invert Mask / 提取背景)";
    invertParam.Type = ParamType::CheckBox;
    invertParam.Value = false;
    _params.push_back(invertParam);

    OpParameter bgParam;
    bgParam.Name = "bgMode";
    bgParam.DisplayName = "非目标背景填充 (Background Fill)";
    bgParam.Type = ParamType::ComboBox;
    bgParam.Value = 0;
    bgParam.EnumOptions = {
        "纯黑背景 (Pure Black)",
        "纯白背景 (Pure White)",
        "原图暗化压暗背景 (Darkened Context)"
    };
    _params.push_back(bgParam);
}

std::string MaskApplyOp::GetPrincipleDoc() const
{
    return "### 📘 算子原理：掩码抠图与原图ROI高保真提取 (Mask Apply)\n\n"
           "将二值分割图转换为 8 位单通道 Mask 掩码：\n"
           "$$dst(x, y) = \\begin{cases} src(x, y) & \\text{if } mask(x, y) > \\text{Thresh} \\\\ \\text{BackgroundColor} & \\text{otherwise} \\end{cases}$$\n"
           "实现对分割目标区域的彩色高保真提取，同时清除复杂的生产背景杂乱噪点。";
}

bool MaskApplyOp::Process(const cv::Mat& input, cv::Mat& output, std::string& errorMsg)
{
    if (input.empty()) { errorMsg = "输入图像为空"; return false; }

    cv::Mat orig = PipelineContext::Instance().GetSourceImage();
    if (orig.empty()) orig = input.clone();
    if (orig.size() != input.size()) cv::resize(orig, orig, input.size());

    int cutoff = _params[0].GetInt();
    bool inv = _params[1].GetBool();
    int bgMode = _params[2].GetInt();

    try
    {
        // 构造单通道 0/255 二值掩码
        cv::Mat mask;
        if (input.channels() > 1) cv::cvtColor(input, mask, cv::COLOR_BGR2GRAY);
        else mask = input.clone();

        cv::threshold(mask, mask, cutoff, 255, inv ? cv::THRESH_BINARY_INV : cv::THRESH_BINARY);

        if (bgMode == 0) // 纯黑
        {
            output = cv::Mat::zeros(orig.size(), orig.type());
            orig.copyTo(output, mask);
        }
        else if (bgMode == 1) // 纯白
        {
            output = cv::Mat(orig.size(), orig.type(), cv::Scalar::all(255));
            orig.copyTo(output, mask);
        }
        else // 暗化背景
        {
            cv::Mat darkOrig;
            orig.convertTo(darkOrig, -1, 0.25, 0);
            output = darkOrig.clone();
            orig.copyTo(output, mask);
        }
        return true;
    }
    catch (const cv::Exception& e)
    {
        errorMsg = e.what();
        return false;
    }
}

std::string MaskApplyOp::GenerateCppCode(const std::string& inVar, const std::string& outVar) const
{
    int cutoff = _params[0].GetInt();
    bool inv = _params[1].GetBool();
    int bgMode = _params[2].GetInt();

    std::stringstream ss;
    ss << "// 掩码抠图与原图ROI提取\n"
       << "cv::Mat mask;\n"
       << "if (" << inVar << ".channels() > 1) cv::cvtColor(" << inVar << ", mask, cv::COLOR_BGR2GRAY);\n"
       << "else mask = " << inVar << ".clone();\n"
       << "cv::threshold(mask, mask, " << cutoff << ", 255, " << (inv ? "cv::THRESH_BINARY_INV" : "cv::THRESH_BINARY") << ");\n";
    if (bgMode == 0)
    {
        ss << outVar << " = cv::Mat::zeros(source_image.size(), source_image.type());\n"
           << "source_image.copyTo(" << outVar << ", mask);";
    }
    else if (bgMode == 1)
    {
        ss << outVar << " = cv::Mat(source_image.size(), source_image.type(), cv::Scalar::all(255));\n"
           << "source_image.copyTo(" << outVar << ", mask);";
    }
    else
    {
        ss << "source_image.convertTo(" << outVar << ", -1, 0.25, 0);\n"
           << "source_image.copyTo(" << outVar << ", mask);";
    }
    return ss.str();
}

std::string MaskApplyOp::GeneratePythonCode(const std::string& inVar, const std::string& outVar) const
{
    int cutoff = _params[0].GetInt();
    bool inv = _params[1].GetBool();
    int bgMode = _params[2].GetInt();

    std::stringstream ss;
    ss << "# 掩码抠图与原图ROI提取\n"
       << "mask = " << inVar << " if len(" << inVar << ".shape) == 2 else cv2.cvtColor(" << inVar << ", cv2.COLOR_BGR2GRAY)\n"
       << "_, mask = cv2.threshold(mask, " << cutoff << ", 255, " << (inv ? "cv2.THRESH_BINARY_INV" : "cv2.THRESH_BINARY") << ")\n";
    if (bgMode == 0)
    {
        ss << outVar << " = np.zeros_like(source_image)\n"
           << outVar << " = cv2.bitwise_and(source_image, source_image, mask=mask)";
    }
    else
    {
        ss << outVar << " = np.full_like(source_image, 255)\n"
           << "np.copyto(" << outVar << ", source_image, where=(mask[:, :, None] > 0))";
    }
    return ss.str();
}

// ============================================================================
// 3. ThresholdTruncateOp (阈值截断与区间截取)
// ============================================================================
ThresholdTruncateOp::ThresholdTruncateOp()
{
    OpParameter modeParam;
    modeParam.Name = "truncMode";
    modeParam.DisplayName = "截断模式 (Truncate Type)";
    modeParam.Type = ParamType::ComboBox;
    modeParam.Value = 0; // THRESH_TRUNC
    modeParam.EnumOptions = {
        "THRESH_TRUNC (大于阈值截平至阈值)",
        "THRESH_TOZERO (低于阈值置零/保留高亮)",
        "THRESH_TOZERO_INV (高于阈值置零/保留暗部)"
    };
    _params.push_back(modeParam);

    OpParameter threshParam;
    threshParam.Name = "threshold";
    threshParam.DisplayName = "截断阈值数值 (Cutoff Threshold)";
    threshParam.Type = ParamType::DoubleSlider;
    threshParam.Value = 128.0;
    threshParam.MinValue = 0.0;
    threshParam.MaxValue = 255.0;
    threshParam.Step = 1.0;
    _params.push_back(threshParam);
}

std::string ThresholdTruncateOp::GetPrincipleDoc() const
{
    return "### 📘 算子原理：像素级非二值化阈值截断 (threshold)\n\n"
           "不同于二值化（输出仅 0 或 255），阈值截断保留连续灰度级：\n"
           "* **THRESH_TRUNC**：$dst(x, y) = \\begin{cases} \\text{thresh} & \\text{if } src(x, y) > \\text{thresh} \\\\ src(x, y) & \\text{otherwise} \\end{cases}$ (削平过曝高光点)；\n"
           "* **THRESH_TOZERO**：低于阈值直接归零，保留原图高亮细节。";
}

bool ThresholdTruncateOp::Process(const cv::Mat& input, cv::Mat& output, std::string& errorMsg)
{
    if (input.empty()) { errorMsg = "输入图像为空"; return false; }
    int modeIdx = _params[0].GetInt();
    double thresh = _params[1].GetDouble();

    int cvType = cv::THRESH_TRUNC;
    if (modeIdx == 1) cvType = cv::THRESH_TOZERO;
    else if (modeIdx == 2) cvType = cv::THRESH_TOZERO_INV;

    try
    {
        cv::threshold(input, output, thresh, 255, cvType);
        return true;
    }
    catch (const cv::Exception& e)
    {
        errorMsg = e.what();
        return false;
    }
}

std::string ThresholdTruncateOp::GenerateCppCode(const std::string& inVar, const std::string& outVar) const
{
    int modeIdx = _params[0].GetInt();
    double thresh = _params[1].GetDouble();
    std::string typeStr = (modeIdx == 1 ? "cv::THRESH_TOZERO" : (modeIdx == 2 ? "cv::THRESH_TOZERO_INV" : "cv::THRESH_TRUNC"));

    std::stringstream ss;
    ss << "// 像素级非二值化阈值截断\n"
       << "double threshValue = " << thresh << ";\n"
       << "int threshType = " << typeStr << ";\n"
       << "cv::threshold(" << inVar << ", " << outVar << ", threshValue, 255, threshType);";
    return ss.str();
}

std::string ThresholdTruncateOp::GeneratePythonCode(const std::string& inVar, const std::string& outVar) const
{
    int modeIdx = _params[0].GetInt();
    double thresh = _params[1].GetDouble();
    std::string typeStr = (modeIdx == 1 ? "cv2.THRESH_TOZERO" : (modeIdx == 2 ? "cv2.THRESH_TOZERO_INV" : "cv2.THRESH_TRUNC"));

    std::stringstream ss;
    ss << "# 像素级非二值化阈值截断\n"
       << "thresh_val = " << thresh << "\n"
       << "thresh_type = " << typeStr << "\n"
       << "_, " << outVar << " = cv2.threshold(" << inVar << ", thresh_val, 255, thresh_type)";
    return ss.str();
}
