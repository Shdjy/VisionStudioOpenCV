#pragma once

#include <QString>
#include <string>

/**
 * @brief 算子与检测能力知识库构建器
 * 
 * 动态提取本地 OpRegistry 算子元数据与检测任务标准 Schema，
 * 组装为结构化 System Prompt 提供给大语言模型，确保模型仅推荐本地已支持的能力。
 */
class OperatorKnowledgeBase
{
public:
    /**
     * @brief 构建检测任务与流水线规划专用的完整 System Prompt
     */
    static QString BuildSystemPrompt();

    /**
     * @brief 构建目标计数示范提示词
     */
    static QString GetObjectCountingPromptExample();

    /**
     * @brief 构建表面缺陷检测示范提示词
     */
    static QString GetDefectDetectionPromptExample();
};
