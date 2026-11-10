#pragma once

#include "DetectionTask.h"
#include <vector>
#include <string>

/**
 * @brief 工业检测业务判定规则引擎 (SRP / DIP)
 * 
 * 职责：
 * 将底层计算机视觉检测结果 (发现的轮廓/框/缺陷) 与工业质检业务标准 (是否允许缺陷、数量是否合格) 彻底解耦。
 * 根据配置规则判定工件最终状态 (OK / NG)，输出明确的判定理由与规则明细。
 */
class InspectionRuleEngine
{
public:
    /**
     * @brief 对检测结果执行规则评估并生成最终判定决策
     * @param result 结构化检测结果引用 (算法检测结果)
     * @param rules 规则数组配置 (QJsonArray)
     * @return 最终检验决策 (包含 Pass/Fail、详细原因及规则命中信息)
     */
    static InspectionDecision Evaluate(DetectionResult& result, const QJsonArray& rules);

    /**
     * @brief 评估目标计数规则
     */
    static bool EvaluateCountRule(
        int actualCount,
        const QJsonObject& rule,
        std::string& outReason);

    /**
     * @brief 评估缺陷规则
     */
    static bool EvaluateDefectRule(
        std::vector<DefectInfo>& defects,
        const QJsonObject& rule,
        std::string& outReason);

    /**
     * @brief 评估几何测量公差规则
     */
    static bool EvaluateMeasurementRule(
        const QJsonObject& measurements,
        const QJsonObject& rule,
        std::string& outReason);
};
