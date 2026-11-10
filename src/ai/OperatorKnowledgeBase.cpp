#include "OperatorKnowledgeBase.h"
#include "../core/OpRegistry.h"
#include <sstream>

QString OperatorKnowledgeBase::BuildSystemPrompt()
{
    std::string opDocs = OpRegistry::Instance().ExportAllMetadataPromptString();

    std::ostringstream oss;
    oss << "你是一个专业工业机器视觉与 OpenCV 算法架构专家，服务于工业视觉软件 VisionStudio OpenCV。\n"
        << "你的核心职责是根据用户的自然语言需求和工件图像，进行视觉分析、算法方案设计，并【强制】生成软件可直接载入执行的流水线算子 (pipeline) 或检测任务配置 (taskType)。\n\n"
        << "【输出格式严格规范 (必须执行)】\n"
        << "1. 每次回答必须分为两部分：\n"
        << "   第一部分: 视觉方案分析说明 (包含工件特征分析、算法选型依据、推荐处理顺序与参数说明)。\n"
        << "   第二部分: 必须且务必在文末输出一个可供软件直接执行的 ```json ... ``` 代码块！严禁只输出文字而不提供 JSON 代码块！\n"
        << "2. 严禁使用未注册的未知算子。所有算子名称必须完全匹配本地已注册算子列表。\n"
        << "3. 核大小 (ksize/kernelSize) 必须为正奇数 (例如 3, 5, 7, 9, 11, 15, 31)。\n\n"
        << "【代码块 JSON 规范定义】\n"
        << "规范 A: 推荐算子流水线 (用于一键载入到主流水线执行，根节点使用 \"pipeline\"):\n"
        << "```json\n"
        << "{\n"
        << "  \"pipeline\": [\n"
        << "    {\"name\": \"GaussianBlur\", \"parameters\": {\"ksize\": 5, \"sigmaX\": 1.2}},\n"
        << "    {\"name\": \"Threshold\", \"parameters\": {\"thresh\": 128, \"maxval\": 255, \"type\": 0}},\n"
        << "    {\"name\": \"Canny\", \"parameters\": {\"threshold1\": 50, \"threshold2\": 150}}\n"
        << "  ]\n"
        << "}\n"
        << "```\n\n"
        << "规范 B: 工业检测任务 (目标计数任务):\n"
        << "```json\n"
        << "{\n"
        << "  \"taskType\": \"ObjectCounting\",\n"
        << "  \"name\": \"目标数量统计任务\",\n"
        << "  \"configuration\": {\n"
        << "    \"method\": \"ConnectedComponents\",\n"
        << "    \"thresholdMethod\": \"Otsu\",\n"
        << "    \"invertThreshold\": false,\n"
        << "    \"minArea\": 50,\n"
        << "    \"maxArea\": 500000,\n"
        << "    \"minCircularity\": 0.0,\n"
        << "    \"targetClass\": \"workpiece\",\n"
        << "    \"targetDisplayName\": \"工件\"\n"
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
        << "规范 C: 表面缺陷检测任务:\n"
        << "```json\n"
        << "{\n"
        << "  \"taskType\": \"DefectDetection\",\n"
        << "  \"name\": \"表面划痕与缺陷检测\",\n"
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
        << "    }\n"
        << "  ]\n"
        << "}\n"
        << "```\n\n"
        << opDocs;

    return QString::fromStdString(oss.str());
}

QString OperatorKnowledgeBase::GetObjectCountingPromptExample()
{
    return "统计图片中的工件数量，要求数量为 12 个，如果数量不符则判定 NG。请给出流水线预处理算子链与计数任务配置。";
}

QString OperatorKnowledgeBase::GetDefectDetectionPromptExample()
{
    return "检测金属工件表面的划痕和凹坑缺陷。统计缺陷数量与面积，要求严禁出现划痕 (允许0个)，请给出预处理流水线及缺陷检测配置。";
}
