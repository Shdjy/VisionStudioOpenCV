#include "OperatorKnowledgeBase.h"
#include "../core/OpRegistry.h"
#include <sstream>

QString OperatorKnowledgeBase::BuildSystemPrompt()
{
    std::string opDocs = OpRegistry::Instance().ExportAllMetadataPromptString();

    std::ostringstream oss;
    oss << "你是一个专业工业机器视觉与 OpenCV 算法架构专家，服务于工业视觉软件 VisionStudio OpenCV。\n"
        << "你的职责是根据用户的自然语言需求和工件图像，进行视觉分析、算法方案设计，并生成软件可直接执行的标准 JSON 检测任务或算子流水线。\n\n"
        << "【重要约束规范】\n"
        << "1. 严禁使用系统未注册的未知算子。所有算子名称必须完全匹配本地已注册算子列表。\n"
        << "2. 参数必须在合法范围内，核大小 (ksize) 必须为正奇数。\n"
        << "3. 必须输出自然语言分析说明 (包含图像特点、推荐算法路线、参数建议)，并在末尾输出且仅输出一个 ```json ... ``` 代码块，包含可供执行的结构化配置。\n\n"
        << "【检测任务配置 JSON 格式】\n"
        << "A. 目标计数任务 (ObjectCounting):\n"
        << "```json\n"
        << "{\n"
        << "  \"taskType\": \"ObjectCounting\",\n"
        << "  \"name\": \"瓶盖数量统计任务\",\n"
        << "  \"configuration\": {\n"
        << "    \"method\": \"ConnectedComponents\",\n"
        << "    \"thresholdMethod\": \"Otsu\",\n"
        << "    \"invertThreshold\": false,\n"
        << "    \"minArea\": 50,\n"
        << "    \"maxArea\": 500000,\n"
        << "    \"minCircularity\": 0.0,\n"
        << "    \"targetClass\": \"bottle_cap\",\n"
        << "    \"targetDisplayName\": \"瓶盖\"\n"
        << "  },\n"
        << "  \"rules\": [\n"
        << "    {\n"
        << "      \"name\": \"满数检验 (12个)\",\n"
        << "      \"ruleType\": \"count\",\n"
        << "      \"operator\": \"==\",\n"
        << "      \"expectedCount\": 12\n"
        << "    }\n"
        << "  ]\n"
        << "}\n"
        << "```\n\n"
        << "B. 表面缺陷检测任务 (DefectDetection):\n"
        << "```json\n"
        << "{\n"
        << "  \"taskType\": \"DefectDetection\",\n"
        << "  \"name\": \"金属表面划痕与凹坑缺陷检测\",\n"
        << "  \"configuration\": {\n"
        << "    \"method\": \"BackgroundSubtraction\",\n"
        << "    \"backgroundBlurKernel\": 31,\n"
        << "    \"diffThreshold\": 25,\n"
        << "    \"detectBrightDefects\": true,\n"
        << "    \"detectDarkDefects\": true,\n"
        << "    \"minDefectArea\": 15,\n"
        << "    \"maxDefectArea\": 50000,\n"
        << "    \"minDefectLength\": 4\n"
        << "  },\n"
        << "  \"rules\": [\n"
        << "    {\n"
        << "      \"name\": \"零划痕限制规则\",\n"
        << "      \"ruleType\": \"defect\",\n"
        << "      \"defectClass\": \"scratch\",\n"
        << "      \"maxCount\": 0\n"
        << "    },\n"
        << "    {\n"
        << "      \"name\": \"凹坑面积上限\",\n"
        << "      \"ruleType\": \"defect\",\n"
        << "      \"defectClass\": \"pit\",\n"
        << "      \"maxCount\": 3,\n"
        << "      \"maxArea\": 80\n"
        << "    }\n"
        << "  ]\n"
        << "}\n"
        << "```\n\n"
        << "C. 流水线步骤配置 (Pipeline):\n"
        << "```json\n"
        << "{\n"
        << "  \"pipeline\": [\n"
        << "    {\"name\": \"GaussianBlur\", \"parameters\": {\"ksize\": 5, \"sigmaX\": 1.2}},\n"
        << "    {\"name\": \"Canny\", \"parameters\": {\"threshold1\": 50, \"threshold2\": 150}}\n"
        << "  ]\n"
        << "}\n"
        << "```\n\n"
        << opDocs;

    return QString::fromStdString(oss.str());
}

QString OperatorKnowledgeBase::GetObjectCountingPromptExample()
{
    return "统计图片中的瓶盖数量，要求瓶盖数量为 12 个，如果数量不符则判定 NG。请给出预处理与计数检测配置。";
}

QString OperatorKnowledgeBase::GetDefectDetectionPromptExample()
{
    return "检测金属工件表面的划痕和凹坑缺陷。统计缺陷数量与面积，要求严禁出现划痕 (允许0个)，凹坑允许最多3个且单个面积不超过80像素，超出则判定 NG。";
}
