#include "BranchOps.h"
#include "../PipelineContext.h"
#include <sstream>

// ============================================================================
// 1. BranchSaveOp (分支暂存与锚点标记算子)
// ============================================================================
BranchSaveOp::BranchSaveOp()
{
    OpParameter slotParam;
    slotParam.Name = "branchSlot";
    slotParam.DisplayName = "分支存储槽位 (Branch Slot)";
    slotParam.Type = ParamType::ComboBox;
    slotParam.Value = 0; // 分支 1
    slotParam.EnumOptions = {
        "分支 1 (Branch 1)",
        "分支 2 (Branch 2)",
        "分支 3 (Branch 3)",
        "分支 4 (Branch 4)"
    };
    slotParam.Tooltip = "选择将当前步骤输出保存到的分支槽位，便于后续加载或双图运算引用";
    _params.push_back(slotParam);
}

std::string BranchSaveOp::GetPrincipleDoc() const
{
    return "### 📘 算子原理：分支暂存与锚点标记 (Branch Save / Fork)\n\n"
           "**【核心思想】**\n"
           "在复杂工业视觉检测中，通常需要将同一张图像分别进行多种不同算法流程的处理（例如：分支1进行高斯滤波+Canny边缘提取，分支2进行颜色空间转换+Otsu二值化），最后再汇合进行双图比对或掩码融合。\n\n"
           "**【工作机制】**\n"
           "- 本算子将当前步骤输入图像深拷贝一份并命名暂存至选定的分支槽位（Branch 1 ~ 4）；\n"
           "- 同时本算子将图像直通传递给下一步，不改变当前图像流；\n"
           "- 后续算子可通过【分支加载】算子重新开启新分支，或在【双图复合运算】中直接引用该分支槽位。\n\n"
           "**【典型应用场景】**\n"
           "1. **多特征提取**：原图 -> 分支1暂存 -> 提取边缘；切换至分支1 -> 提取斑点 -> 双图融合；\n"
           "2. **基准图对比**：标定处理完后暂存为标准金样模板，后续工件与该分支比对；\n"
           "3. **局部ROI掩码**：分支1生成二值Mask，分支2提取增强图，最终执行 Mask Apply ROI。";
}

bool BranchSaveOp::Process(const cv::Mat& input, cv::Mat& output, std::string& errorMsg)
{
    if (input.empty())
    {
        errorMsg = "输入图像为空";
        return false;
    }

    int slotIdx = _params[0].GetInt();
    std::string slotName = "Branch " + std::to_string(slotIdx + 1);

    // 暂存至流水线多图分支上下文
    PipelineContext::Instance().SetBranch(slotName, input);

    // 直通输出
    output = input.clone();
    return true;
}

std::string BranchSaveOp::GenerateCppCode(const std::string& inVar, const std::string& outVar) const
{
    int slotIdx = _params[0].GetInt();
    std::string branchVarName = "branch_" + std::to_string(slotIdx + 1);

    std::stringstream ss;
    ss << "// 暂存当前图像到分支 " << (slotIdx + 1) << " 槽位\n"
       << "cv::Mat " << branchVarName << " = " << inVar << ".clone();\n"
       << outVar << " = " << inVar << ".clone();";
    return ss.str();
}

std::string BranchSaveOp::GeneratePythonCode(const std::string& inVar, const std::string& outVar) const
{
    int slotIdx = _params[0].GetInt();
    std::string branchVarName = "branch_" + std::to_string(slotIdx + 1);

    std::stringstream ss;
    ss << "# 暂存当前图像到分支 " << (slotIdx + 1) << " 槽位\n"
       << branchVarName << " = " << inVar << ".copy()\n"
       << outVar << " = " << inVar << ".copy()";
    return ss.str();
}

// ============================================================================
// 2. BranchLoadOp (分支加载与源切换算子)
// ============================================================================
BranchLoadOp::BranchLoadOp()
{
    OpParameter srcParam;
    srcParam.Name = "sourceType";
    srcParam.DisplayName = "输入源切换目标 (Target Input Source)";
    srcParam.Type = ParamType::ComboBox;
    srcParam.Value = 0; // 原始输入图像
    srcParam.EnumOptions = {
        "原始输入图像 (Original Source Image)",
        "分支 1 (Branch 1)",
        "分支 2 (Branch 2)",
        "分支 3 (Branch 3)",
        "分支 4 (Branch 4)",
        "指定前置步骤输出 (Step Output)"
    };
    srcParam.Tooltip = "选择切换后续流程的输入来源，开辟独立的新分支处理流程";
    _params.push_back(srcParam);

    OpParameter stepParam;
    stepParam.Name = "stepIndex";
    stepParam.DisplayName = "目标步骤序号 (Step Index)";
    stepParam.Type = ParamType::IntSlider;
    stepParam.Value = 1;
    stepParam.MinValue = 1;
    stepParam.MaxValue = 50;
    stepParam.Step = 1;
    stepParam.Tooltip = "当源选择'指定前置步骤输出'时生效 (1-based)";
    _params.push_back(stepParam);
}

std::string BranchLoadOp::GetPrincipleDoc() const
{
    return "### 📘 算子原理：分支加载与源切换 (Branch Load / Switch Source)\n\n"
           "**【核心思想】**\n"
           "本算子用于在单一线性流水线中打断上一步输出的依赖链，将当前输入源重定向为原始输入图像、此前暂存的任一分支（Branch 1 ~ 4）、或任意前置历史步骤的输出。\n\n"
           "**【工作机制】**\n"
           "- 从全局多图上下文 `PipelineContext` 中取出所选来源图像；\n"
           "- 忽略上一步的处理结果，直接将提取到的源图像作为本步骤的输出；\n"
           "- 本步骤之后添加的所有算子，均会基于该源图像继续进行处理，从而开启崭新的分支处理进程。\n\n"
           "**【典型流水线范例】**\n"
           "```\n"
           "步骤 1: 灰度化 (Gray)\n"
           "步骤 2: 分支暂存 (BranchSave -> 保存到 Branch 1)\n"
           "步骤 3: 高斯滤波 -> Canny 边缘提取 (分支 1 的处理流程)\n"
           "步骤 4: 分支暂存 (BranchSave -> 保存到 Branch 2)\n"
           "步骤 5: 分支加载 (BranchLoad -> 切换源为 Branch 1)\n"
           "步骤 6: Otsu 阈值二值化 -> 形态学开运算 (分支 2 的处理流程)\n"
           "步骤 7: 双图复合运算 (操作数A=当前输入, 操作数B=Branch 2)\n"
           "```";
}

bool BranchLoadOp::Process(const cv::Mat& input, cv::Mat& output, std::string& errorMsg)
{
    (void)input;

    int srcType = _params[0].GetInt();
    int targetStep = _params[1].GetInt();

    cv::Mat selectedMat;
    if (srcType == 0) // 原始输入图像
    {
        selectedMat = PipelineContext::Instance().GetSourceImage();
    }
    else if (srcType >= 1 && srcType <= 4) // 分支 1 ~ 4
    {
        std::string branchName = "Branch " + std::to_string(srcType);
        if (!PipelineContext::Instance().HasBranch(branchName))
        {
            errorMsg = "所选 " + branchName + " 尚未暂存有效图像，请检查前置是否有 BranchSave 步骤";
            return false;
        }
        selectedMat = PipelineContext::Instance().GetBranch(branchName);
    }
    else if (srcType == 5) // 指定步骤输出
    {
        selectedMat = PipelineContext::Instance().GetStepOutput(targetStep - 1);
    }

    if (selectedMat.empty())
    {
        errorMsg = "未能从指定分支或源中获取到有效图像";
        return false;
    }

    output = selectedMat.clone();
    return true;
}

std::string BranchLoadOp::GenerateCppCode(const std::string& inVar, const std::string& outVar) const
{
    (void)inVar;
    int srcType = _params[0].GetInt();
    int targetStep = _params[1].GetInt();

    std::stringstream ss;
    if (srcType == 0)
    {
        ss << "// 切换输入源为: 原始输入图像\n"
           << outVar << " = source_image.clone();";
    }
    else if (srcType >= 1 && srcType <= 4)
    {
        std::string branchVarName = "branch_" + std::to_string(srcType);
        ss << "// 切换输入源为: " << branchVarName << "\n"
           << outVar << " = " << branchVarName << ".clone();";
    }
    else
    {
        std::string stepVarName = "step_" + std::to_string(targetStep) + "_out";
        ss << "// 切换输入源为前置步骤 " << targetStep << " 输出\n"
           << outVar << " = " << stepVarName << ".clone();";
    }
    return ss.str();
}

std::string BranchLoadOp::GeneratePythonCode(const std::string& inVar, const std::string& outVar) const
{
    (void)inVar;
    int srcType = _params[0].GetInt();
    int targetStep = _params[1].GetInt();

    std::stringstream ss;
    if (srcType == 0)
    {
        ss << "# 切换输入源为: 原始输入图像\n"
           << outVar << " = source_image.copy()";
    }
    else if (srcType >= 1 && srcType <= 4)
    {
        std::string branchVarName = "branch_" + std::to_string(srcType);
        ss << "# 切换输入源为: " << branchVarName << "\n"
           << outVar << " = " << branchVarName << ".copy()";
    }
    else
    {
        std::string stepVarName = "step_" + std::to_string(targetStep) + "_out";
        ss << "# 切换输入源为前置步骤 " << targetStep << " 输出\n"
           << outVar << " = " << stepVarName << ".copy()";
    }
    return ss.str();
}
