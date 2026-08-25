#include "ArithmeticOps.h"
#include "../PipelineContext.h"
#include <sstream>

namespace
{
// 辅助函数：根据分支选项获取第二操作数图像
cv::Mat ResolveBranchImage(int secSourceCode, const cv::Mat& currentInput, const cv::Size& targetSize, int targetChannels)
{
    cv::Mat result;
    if (secSourceCode == 0) // 主流水线最终输出
    {
        result = PipelineContext::Instance().GetBranch("主流水线");
        if (result.empty()) result = PipelineContext::Instance().GetBranch("Main");
        if (result.empty()) result = PipelineContext::Instance().GetSourceImage();
    }
    else if (secSourceCode >= 1 && secSourceCode <= 4) // 分支 1 ~ 4
    {
        std::string bName = "分支 " + std::to_string(secSourceCode);
        result = PipelineContext::Instance().GetBranch(bName);
        if (result.empty())
        {
            result = PipelineContext::Instance().GetBranch("Branch " + std::to_string(secSourceCode));
        }
        if (result.empty())
        {
            result = currentInput.clone();
        }
    }
    else if (secSourceCode == 5) // 原始输入图像
    {
        result = PipelineContext::Instance().GetSourceImage();
        if (result.empty()) result = currentInput.clone();
    }
    else if (secSourceCode == 6) // 纯白底板
    {
        cv::Size sz = (targetSize.width > 0 && targetSize.height > 0) ? targetSize : (currentInput.empty() ? cv::Size(640, 480) : currentInput.size());
        int ch = (targetChannels > 0) ? targetChannels : (currentInput.empty() ? 3 : currentInput.channels());
        result = cv::Mat(sz, ch == 1 ? CV_8UC1 : CV_8UC3, cv::Scalar::all(255));
    }
    else if (secSourceCode == 7) // 纯黑底板
    {
        cv::Size sz = (targetSize.width > 0 && targetSize.height > 0) ? targetSize : (currentInput.empty() ? cv::Size(640, 480) : currentInput.size());
        int ch = (targetChannels > 0) ? targetChannels : (currentInput.empty() ? 3 : currentInput.channels());
        result = cv::Mat(sz, ch == 1 ? CV_8UC1 : CV_8UC3, cv::Scalar::all(0));
    }
    else
    {
        result = currentInput.clone();
    }

    if (result.empty())
    {
        result = currentInput.clone();
    }

    // 几何尺寸对齐
    if (targetSize.width > 0 && targetSize.height > 0 && result.size() != targetSize)
    {
        cv::resize(result, result, targetSize);
    }

    // 通道数对齐
    if (targetChannels > 0 && result.channels() != targetChannels)
    {
        if (targetChannels == 3 && result.channels() == 1)
        {
            cv::cvtColor(result, result, cv::COLOR_GRAY2BGR);
        }
        else if (targetChannels == 1 && result.channels() == 3)
        {
            cv::cvtColor(result, result, cv::COLOR_BGR2GRAY);
        }
    }

    return result;
}

std::string GetBranchCppExpr(int secSourceCode, const std::string& inVar)
{
    if (secSourceCode == 0) return "main_pipeline_out";
    if (secSourceCode >= 1 && secSourceCode <= 4) return "branch_" + std::to_string(secSourceCode) + "_out";
    if (secSourceCode == 5) return "source_image";
    if (secSourceCode == 6) return "cv::Mat(" + inVar + ".size(), " + inVar + ".type(), cv::Scalar::all(255))";
    if (secSourceCode == 7) return "cv::Mat::zeros(" + inVar + ".size(), " + inVar + ".type())";
    return inVar;
}

std::string GetBranchPythonExpr(int secSourceCode, const std::string& inVar)
{
    if (secSourceCode == 0) return "main_pipeline_out";
    if (secSourceCode >= 1 && secSourceCode <= 4) return "branch_" + std::to_string(secSourceCode) + "_out";
    if (secSourceCode == 5) return "source_image";
    if (secSourceCode == 6) return "np.full_like(" + inVar + ", 255)";
    if (secSourceCode == 7) return "np.zeros_like(" + inVar + ")";
    return inVar;
}
}

// ============================================================================
// 1. BlendWithSourceOp (双图与分支复合运算)
// ============================================================================
BlendWithSourceOp::BlendWithSourceOp()
{
    // 0. 第二操作数来源 (分支合并目标)
    OpParameter secSrcParam;
    secSrcParam.Name = "secondSource";
    secSrcParam.DisplayName = "合并目标分支/图像源 (Merge Target Branch)";
    secSrcParam.Type = ParamType::ComboBox;
    secSrcParam.Value = 0; // 主流水线
    secSrcParam.EnumOptions = {
        "主流水线最终输出 (Main Pipeline Output)",
        "分支 1 最终输出 (Branch 1 Output)",
        "分支 2 最终输出 (Branch 2 Output)",
        "分支 3 最终输出 (Branch 3 Output)",
        "分支 4 最终输出 (Branch 4 Output)",
        "原始输入图像 (Original Source Image)",
        "纯白色底板 (Solid White Canvas)",
        "纯黑色底板 (Solid Black Canvas)"
    };
    secSrcParam.Tooltip = "选择与当前流程进行合并/复合运算的目标分支或图像源";
    _params.push_back(secSrcParam);

    // 1. 运算模式
    OpParameter modeParam;
    modeParam.Name = "opMode";
    modeParam.DisplayName = "复合运算模式 (Operation Mode)";
    modeParam.Type = ParamType::ComboBox;
    modeParam.Value = 0; // 加权融合
    modeParam.EnumOptions = {
        "线性加权融合 (AddWeighted: α·当前 + β·目标 + γ)",
        "绝对值差分 (AbsDiff: |当前 - 目标| / 缺陷比对)",
        "按位与掩码相交 (Bitwise AND)",
        "按位或特征叠加 (Bitwise OR)",
        "按位异或边缘差异 (Bitwise XOR)",
        "饱和加法叠加 (cv::add)",
        "饱和减法剔除 (cv::subtract: 当前 - 目标)",
        "取亮最大值融合 (cv::max)",
        "取暗最小值融合 (cv::min)",
        "逐像素点乘缩放 (cv::multiply / 255)",
        "逐像素点除校正 (cv::divide * 255)"
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
    betaParam.DisplayName = "目标图权重 (Beta β)";
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
    return "### 📘 算子原理：多分支双图合并复合运算 (Branch Merge & Blending)\n\n"
           "**【核心思想】**\n"
           "将当前流程的处理结果与指定的**分支流水线输出**（或主流水线、原始图像）进行逐像素点对点的代数加减、乘除融合、差分与布尔逻辑运算，完成多分支视觉工作流的最终汇合。\n\n"
           "**【主要运算模型与数学公式】**\n"
           "```\n"
           "   1. 线性加权融合 (addWeighted):\n"
           "        dst(x, y) = saturate( α·current(x, y) + β·target(x, y) + γ )\n"
           "\n"
           "   2. 绝对值差分 (absdiff - 工业标准缺陷检测):\n"
           "        dst(x, y) = | current(x, y) - target(x, y) |\n"
           "\n"
           "   3. 饱和代数加/减法 (add / subtract):\n"
           "        dst_add(x, y) = min(current + target, 255)\n"
           "        dst_sub(x, y) = max(current - target, 0)\n"
           "\n"
           "   4. 按位布尔运算 (bitwise_and / bitwise_or / bitwise_xor):\n"
           "        dst_and(x, y) = current(x, y) & target(x, y)  (掩码 ROI 提取)\n"
           "        dst_xor(x, y) = current(x, y) ^ target(x, y)  (差异特征提取)\n"
           "```\n\n"
           "**【典型分支使用流】**\n"
           "1. 主流水线处理完预处理后，点击【🌿 创建分支】生成“分支 1”；\n"
           "2. 在“分支 1”中进行 Sobel/Canny 边缘提取或形态学运算；\n"
           "3. 在主流水线（或另一分支）中添加本算子，目标分支选【分支 1】，模式选【绝对值差分】或【按位与】，即可直接将两个分支的处理结果进行合并！";
}

bool BlendWithSourceOp::Process(const cv::Mat& input, cv::Mat& output, std::string& errorMsg)
{
    if (input.empty())
    {
        errorMsg = "输入图像为空";
        return false;
    }

    int secSource = _params[0].GetInt();
    int opMode = _params[1].GetInt();
    double alpha = _params[2].GetDouble();
    double beta = _params[3].GetDouble();
    int gamma = _params[4].GetInt();

    cv::Mat targetImg = ResolveBranchImage(secSource, input, input.size(), input.channels());
    if (targetImg.empty())
    {
        targetImg = input.clone();
    }

    try
    {
        switch (opMode)
        {
            case 0: // AddWeighted
                cv::addWeighted(input, alpha, targetImg, beta, gamma, output);
                break;
            case 1: // AbsDiff
                cv::absdiff(input, targetImg, output);
                break;
            case 2: // Bitwise AND
                cv::bitwise_and(input, targetImg, output);
                break;
            case 3: // Bitwise OR
                cv::bitwise_or(input, targetImg, output);
                break;
            case 4: // Bitwise XOR
                cv::bitwise_xor(input, targetImg, output);
                break;
            case 5: // Add
                cv::add(input, targetImg, output);
                break;
            case 6: // Subtract
                cv::subtract(input, targetImg, output);
                break;
            case 7: // Max
                cv::max(input, targetImg, output);
                break;
            case 8: // Min
                cv::min(input, targetImg, output);
                break;
            case 9: // Multiply
                cv::multiply(input, targetImg, output, 1.0 / 255.0);
                break;
            case 10: // Divide
            {
                cv::Mat safeTarget;
                cv::max(targetImg, cv::Scalar::all(1), safeTarget);
                cv::divide(input, safeTarget, output, 255.0);
                break;
            }
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
    int secSource = _params[0].GetInt();
    int opMode = _params[1].GetInt();
    double alpha = _params[2].GetDouble();
    double beta = _params[3].GetDouble();
    int gamma = _params[4].GetInt();

    std::string targetExpr = GetBranchCppExpr(secSource, inVar);

    std::stringstream ss;
    ss << "// 双图与分支复合运算 (模式: " << opMode << ")\n"
       << "cv::Mat merge_target = " << targetExpr << ";\n"
       << "if (merge_target.size() != " << inVar << ".size()) cv::resize(merge_target, merge_target, " << inVar << ".size());\n"
       << "if (merge_target.channels() != " << inVar << ".channels()) {\n"
       << "    if (" << inVar << ".channels() == 3) cv::cvtColor(merge_target, merge_target, cv::COLOR_GRAY2BGR);\n"
       << "    else cv::cvtColor(merge_target, merge_target, cv::COLOR_BGR2GRAY);\n"
       << "}\n";

    switch (opMode)
    {
        case 0:
            ss << "cv::addWeighted(" << inVar << ", " << alpha << ", merge_target, " << beta << ", " << gamma << ", " << outVar << ");";
            break;
        case 1:
            ss << "cv::absdiff(" << inVar << ", merge_target, " << outVar << "); // 绝对值差分比对";
            break;
        case 2:
            ss << "cv::bitwise_and(" << inVar << ", merge_target, " << outVar << ");";
            break;
        case 3:
            ss << "cv::bitwise_or(" << inVar << ", merge_target, " << outVar << ");";
            break;
        case 4:
            ss << "cv::bitwise_xor(" << inVar << ", merge_target, " << outVar << ");";
            break;
        case 5:
            ss << "cv::add(" << inVar << ", merge_target, " << outVar << ");";
            break;
        case 6:
            ss << "cv::subtract(" << inVar << ", merge_target, " << outVar << ");";
            break;
        case 7:
            ss << "cv::max(" << inVar << ", merge_target, " << outVar << ");";
            break;
        case 8:
            ss << "cv::min(" << inVar << ", merge_target, " << outVar << ");";
            break;
        case 9:
            ss << "cv::multiply(" << inVar << ", merge_target, " << outVar << ", 1.0 / 255.0);";
            break;
        case 10:
            ss << "cv::Mat safe_b; cv::max(merge_target, cv::Scalar::all(1), safe_b);\n"
               << "cv::divide(" << inVar << ", safe_b, " << outVar << ", 255.0);";
            break;
        default:
            ss << outVar << " = " << inVar << ".clone();";
            break;
    }
    return ss.str();
}

std::string BlendWithSourceOp::GeneratePythonCode(const std::string& inVar, const std::string& outVar) const
{
    int secSource = _params[0].GetInt();
    int opMode = _params[1].GetInt();
    double alpha = _params[2].GetDouble();
    double beta = _params[3].GetDouble();
    int gamma = _params[4].GetInt();

    std::string targetExpr = GetBranchPythonExpr(secSource, inVar);

    std::stringstream ss;
    ss << "# 双图与分支复合运算 (模式: " << opMode << ")\n"
       << "merge_target = " << targetExpr << "\n"
       << "if merge_target.shape[:2] != " << inVar << ".shape[:2]:\n"
       << "    merge_target = cv2.resize(merge_target, (" << inVar << ".shape[1], " << inVar << ".shape[0]))\n";

    switch (opMode)
    {
        case 0:
            ss << outVar << " = cv2.addWeighted(" << inVar << ", " << alpha << ", merge_target, " << beta << ", " << gamma << ")";
            break;
        case 1:
            ss << outVar << " = cv2.absdiff(" << inVar << ", merge_target) # 绝对值差分比对";
            break;
        case 2:
            ss << outVar << " = cv2.bitwise_and(" << inVar << ", merge_target)";
            break;
        case 3:
            ss << outVar << " = cv2.bitwise_or(" << inVar << ", merge_target)";
            break;
        case 4:
            ss << outVar << " = cv2.bitwise_xor(" << inVar << ", merge_target)";
            break;
        case 5:
            ss << outVar << " = cv2.add(" << inVar << ", merge_target)";
            break;
        case 6:
            ss << outVar << " = cv2.subtract(" << inVar << ", merge_target)";
            break;
        case 7:
            ss << outVar << " = cv2.max(" << inVar << ", merge_target)";
            break;
        case 8:
            ss << outVar << " = cv2.min(" << inVar << ", merge_target)";
            break;
        case 9:
            ss << outVar << " = cv2.multiply(" << inVar << ", merge_target, scale=1.0/255.0)";
            break;
        case 10:
            ss << "safe_b = np.maximum(merge_target, 1)\n"
               << outVar << " = cv2.divide(" << inVar << ".astype(np.float32), safe_b.astype(np.float32), scale=255.0).astype(np.uint8)";
            break;
        default:
            ss << outVar << " = " << inVar << ".copy()";
            break;
    }
    return ss.str();
}

// ============================================================================
// 2. MaskApplyOp (掩码抠图与ROI提取)
// ============================================================================
MaskApplyOp::MaskApplyOp()
{
    // 0. 目标图源
    OpParameter targetSrcParam;
    targetSrcParam.Name = "targetSource";
    targetSrcParam.DisplayName = "目标底图图像源 (Target Image Source)";
    targetSrcParam.Type = ParamType::ComboBox;
    targetSrcParam.Value = 0; // 原始输入图像
    targetSrcParam.EnumOptions = {
        "原始输入图像 (Original Source Image)",
        "主流水线最终输出 (Main Pipeline Output)",
        "分支 1 (Branch 1 Output)",
        "分支 2 (Branch 2 Output)",
        "分支 3 (Branch 3 Output)",
        "分支 4 (Branch 4 Output)"
    };
    targetSrcParam.Tooltip = "选择要从中提取彩色/灰度目标区域的底图";
    _params.push_back(targetSrcParam);

    // 1. 掩码判定阈值
    OpParameter threshParam;
    threshParam.Name = "maskThresh";
    threshParam.DisplayName = "掩码生效判定阈值 (Mask Cutoff)";
    threshParam.Type = ParamType::IntSlider;
    threshParam.Value = 128;
    threshParam.MinValue = 1;
    threshParam.MaxValue = 254;
    threshParam.Step = 1;
    _params.push_back(threshParam);

    // 2. 反转掩码
    OpParameter invertParam;
    invertParam.Name = "invertMask";
    invertParam.DisplayName = "掩码反转 (Invert Mask / 提取背景)";
    invertParam.Type = ParamType::CheckBox;
    invertParam.Value = false;
    _params.push_back(invertParam);

    // 3. 背景填充模式
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
    return "### 📘 算子原理：掩码抠图与目标ROI高保真提取 (Mask Apply)\n\n"
           "**【核心思想】**\n"
           "将当前流程的二值分割结果转换为 8 位单通道 Mask 掩码，并精准作用回目标彩色/灰度图像（如原始输入图、主流水线或指定分支）：\n"
           "$$dst(x, y) = \\begin{cases} target(x, y) & \\text{if } mask(x, y) > \\text{Thresh} \\\\ \\text{BackgroundColor} & \\text{otherwise} \\end{cases}$$\n"
           "实现对分割目标区域的彩色高保真无损提取，同时清除复杂的工业背景杂乱噪点。";
}

bool MaskApplyOp::Process(const cv::Mat& input, cv::Mat& output, std::string& errorMsg)
{
    if (input.empty())
    {
        errorMsg = "输入图像为空";
        return false;
    }

    int targetSrc = _params[0].GetInt();
    int cutoff = _params[1].GetInt();
    bool inv = _params[2].GetBool();
    int bgMode = _params[3].GetInt();

    cv::Mat targetMat;
    if (targetSrc == 0)
    {
        targetMat = PipelineContext::Instance().GetSourceImage();
    }
    else if (targetSrc == 1)
    {
        targetMat = PipelineContext::Instance().GetBranch("主流水线");
        if (targetMat.empty()) targetMat = PipelineContext::Instance().GetSourceImage();
    }
    else
    {
        targetMat = PipelineContext::Instance().GetBranch("分支 " + std::to_string(targetSrc - 1));
        if (targetMat.empty()) targetMat = PipelineContext::Instance().GetSourceImage();
    }

    if (targetMat.empty()) targetMat = input.clone();
    if (targetMat.size() != input.size()) cv::resize(targetMat, targetMat, input.size());

    try
    {
        cv::Mat mask;
        if (input.channels() > 1) cv::cvtColor(input, mask, cv::COLOR_BGR2GRAY);
        else mask = input.clone();

        cv::threshold(mask, mask, cutoff, 255, inv ? cv::THRESH_BINARY_INV : cv::THRESH_BINARY);

        if (bgMode == 0) // 纯黑
        {
            output = cv::Mat::zeros(targetMat.size(), targetMat.type());
            targetMat.copyTo(output, mask);
        }
        else if (bgMode == 1) // 纯白
        {
            output = cv::Mat(targetMat.size(), targetMat.type(), cv::Scalar::all(255));
            targetMat.copyTo(output, mask);
        }
        else // 暗化背景
        {
            cv::Mat darkOrig;
            targetMat.convertTo(darkOrig, -1, 0.25, 0);
            output = darkOrig.clone();
            targetMat.copyTo(output, mask);
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
    int targetSrc = _params[0].GetInt();
    int cutoff = _params[1].GetInt();
    bool inv = _params[2].GetBool();
    int bgMode = _params[3].GetInt();

    std::string targetExpr = (targetSrc == 0) ? "source_image" : ((targetSrc == 1) ? "main_pipeline_out" : ("branch_" + std::to_string(targetSrc - 1) + "_out"));

    std::stringstream ss;
    ss << "// 掩码抠图与目标ROI高保真提取\n"
       << "cv::Mat target_img = " << targetExpr << ";\n"
       << "if (target_img.size() != " << inVar << ".size()) cv::resize(target_img, target_img, " << inVar << ".size());\n"
       << "cv::Mat mask;\n"
       << "if (" << inVar << ".channels() > 1) cv::cvtColor(" << inVar << ", mask, cv::COLOR_BGR2GRAY);\n"
       << "else mask = " << inVar << ".clone();\n"
       << "cv::threshold(mask, mask, " << cutoff << ", 255, " << (inv ? "cv::THRESH_BINARY_INV" : "cv::THRESH_BINARY") << ");\n";
    if (bgMode == 0)
    {
        ss << outVar << " = cv::Mat::zeros(target_img.size(), target_img.type());\n"
           << "target_img.copyTo(" << outVar << ", mask);";
    }
    else if (bgMode == 1)
    {
        ss << outVar << " = cv::Mat(target_img.size(), target_img.type(), cv::Scalar::all(255));\n"
           << "target_img.copyTo(" << outVar << ", mask);";
    }
    else
    {
        ss << "target_img.convertTo(" << outVar << ", -1, 0.25, 0);\n"
           << "target_img.copyTo(" << outVar << ", mask);";
    }
    return ss.str();
}

std::string MaskApplyOp::GeneratePythonCode(const std::string& inVar, const std::string& outVar) const
{
    int targetSrc = _params[0].GetInt();
    int cutoff = _params[1].GetInt();
    bool inv = _params[2].GetBool();
    int bgMode = _params[3].GetInt();

    std::string targetExpr = (targetSrc == 0) ? "source_image" : ((targetSrc == 1) ? "main_pipeline_out" : ("branch_" + std::to_string(targetSrc - 1) + "_out"));

    std::stringstream ss;
    ss << "# 掩码抠图与目标ROI高保真提取\n"
       << "target_img = " << targetExpr << "\n"
       << "if target_img.shape[:2] != " << inVar << ".shape[:2]:\n"
       << "    target_img = cv2.resize(target_img, (" << inVar << ".shape[1], " << inVar << ".shape[0]))\n"
       << "mask = " << inVar << " if len(" << inVar << ".shape) == 2 else cv2.cvtColor(" << inVar << ", cv2.COLOR_BGR2GRAY)\n"
       << "_, mask = cv2.threshold(mask, " << cutoff << ", 255, " << (inv ? "cv2.THRESH_BINARY_INV" : "cv2.THRESH_BINARY") << ")\n";
    if (bgMode == 0)
    {
        ss << outVar << " = np.zeros_like(target_img)\n"
           << "if len(target_img.shape) == 3:\n"
           << "    np.copyto(" << outVar << ", target_img, where=(mask[:, :, None] > 0))\n"
           << "else:\n"
           << "    np.copyto(" << outVar << ", target_img, where=(mask > 0))";
    }
    else if (bgMode == 1)
    {
        ss << outVar << " = np.full_like(target_img, 255)\n"
           << "if len(target_img.shape) == 3:\n"
           << "    np.copyto(" << outVar << ", target_img, where=(mask[:, :, None] > 0))\n"
           << "else:\n"
           << "    np.copyto(" << outVar << ", target_img, where=(mask > 0))";
    }
    else
    {
        ss << "dark = (target_img * 0.25).astype(np.uint8)\n"
           << outVar << " = dark.copy()\n"
           << "if len(target_img.shape) == 3:\n"
           << "    np.copyto(" << outVar << ", target_img, where=(mask[:, :, None] > 0))\n"
           << "else:\n"
           << "    np.copyto(" << outVar << ", target_img, where=(mask > 0))";
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

