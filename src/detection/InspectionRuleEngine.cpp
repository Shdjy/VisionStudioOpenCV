#include "InspectionRuleEngine.h"
#include "../utils/Logger.h"
#include <chrono>
#include <sstream>

bool InspectionRuleEngine::EvaluateCountRule(
    int actualCount,
    const QJsonObject& rule,
    std::string& outReason)
{
    QString op = rule["operator"].toString("==");
    int expected = rule["expectedCount"].toInt(0);

    if (op == "==")
    {
        if (actualCount != expected)
        {
            outReason = "数量不符: 实际检测到 " + std::to_string(actualCount) +
                        " 个目标，要求正好为 " + std::to_string(expected) + " 个 (差额: " +
                        std::to_string(actualCount - expected) + ")";
            return false;
        }
    }
    else if (op == ">=")
    {
        int minCount = rule.contains("minCount") ? rule["minCount"].toInt() : expected;
        if (actualCount < minCount)
        {
            outReason = "数量不足: 实际检测到 " + std::to_string(actualCount) +
                        " 个，要求至少 " + std::to_string(minCount) + " 个";
            return false;
        }
    }
    else if (op == "<=")
    {
        int maxCount = rule.contains("maxCount") ? rule["maxCount"].toInt() : expected;
        if (actualCount > maxCount)
        {
            outReason = "数量超出: 实际检测到 " + std::to_string(actualCount) +
                        " 个，允许最多 " + std::to_string(maxCount) + " 个";
            return false;
        }
    }
    else if (op == "in_range")
    {
        int minCount = rule["minCount"].toInt(0);
        int maxCount = rule["maxCount"].toInt(100000);
        if (actualCount < minCount || actualCount > maxCount)
        {
            outReason = "数量不在合格区间: 实际 " + std::to_string(actualCount) +
                        " 个，区间要求 [" + std::to_string(minCount) + ", " +
                        std::to_string(maxCount) + "]";
            return false;
        }
    }

    return true;
}

bool InspectionRuleEngine::EvaluateDefectRule(
    std::vector<DefectInfo>& defects,
    const QJsonObject& rule,
    std::string& outReason)
{
    QString ruleName = rule["name"].toString("缺陷规则");
    QString targetClass = rule["defectClass"].toString("any");
    int maxAllowedCount = rule.contains("maxCount") ? rule["maxCount"].toInt(0) : 0;
    double maxArea = rule.contains("maxArea") ? rule["maxArea"].toDouble(0.0) : 0.0;
    double maxLength = rule.contains("maxLength") ? rule["maxLength"].toDouble(0.0) : 0.0;

    int matchedCount = 0;
    for (auto& defect : defects)
    {
        bool classMatches = (targetClass == "any" ||
                             QString::fromStdString(defect.DefectType).compare(targetClass, Qt::CaseInsensitive) == 0);
        if (!classMatches)
        {
            continue;
        }

        matchedCount++;

        // 检查面积限制
        if (maxArea > 0.0 && defect.Area > maxArea)
        {
            defect.IsNG = true;
            defect.RuleTriggered = ruleName.toStdString();
            outReason = "缺陷面积超标: " + defect.DefectDisplayName + " 面积 " +
                        std::to_string(defect.Area) + " px² 超过上限 " + std::to_string(maxArea) + " px²";
            return false;
        }

        // 检查长度限制
        if (maxLength > 0.0 && defect.Length > maxLength)
        {
            defect.IsNG = true;
            defect.RuleTriggered = ruleName.toStdString();
            outReason = "缺陷长度超标: " + defect.DefectDisplayName + " 长度 " +
                        std::to_string(defect.Length) + " px 超过上限 " + std::to_string(maxLength) + " px";
            return false;
        }
    }

    if (matchedCount > maxAllowedCount)
    {
        for (auto& defect : defects)
        {
            if (targetClass == "any" || QString::fromStdString(defect.DefectType).compare(targetClass, Qt::CaseInsensitive) == 0)
            {
                defect.IsNG = true;
                defect.RuleTriggered = ruleName.toStdString();
            }
        }
        outReason = "缺陷数量超标: 检出 " + std::to_string(matchedCount) + " 处" +
                    (targetClass == "any" ? "缺陷" : targetClass.toStdString()) +
                    "，允许最大数量为 " + std::to_string(maxAllowedCount);
        return false;
    }

    return true;
}

bool InspectionRuleEngine::EvaluateMeasurementRule(
    const QJsonObject& measurements,
    const QJsonObject& rule,
    std::string& outReason)
{
    QString paramName = rule["paramName"].toString();
    if (!measurements.contains(paramName))
    {
        return true;
    }

    double val = measurements[paramName].toDouble();
    if (rule.contains("minValue") && val < rule["minValue"].toDouble())
    {
        outReason = "测量值超下差: " + paramName.toStdString() + " 测量为 " +
                    std::to_string(val) + "，低于下限 " + std::to_string(rule["minValue"].toDouble());
        return false;
    }
    if (rule.contains("maxValue") && val > rule["maxValue"].toDouble())
    {
        outReason = "测量值超上差: " + paramName.toStdString() + " 测量为 " +
                    std::to_string(val) + "，高于上限 " + std::to_string(rule["maxValue"].toDouble());
        return false;
    }
    return true;
}

InspectionDecision InspectionRuleEngine::Evaluate(DetectionResult& result, const QJsonArray& rules)
{
    auto startTime = std::chrono::high_resolution_clock::now();

    InspectionDecision decision;
    decision.Status = InspectionStatus::Pass; // 默认合格，触发违规规则后置为 Fail

    QJsonObject ruleResults;
    bool anyRuleFailed = false;

    if (rules.isEmpty())
    {
        // 若未配置具体规则，默认检查缺陷数量是否为 0
        if (result.DefectCount > 0)
        {
            decision.Status = InspectionStatus::Fail;
            decision.Reasons.push_back("检测到 " + std::to_string(result.DefectCount) + " 处缺陷");
            for (auto& d : result.Defects)
            {
                d.IsNG = true;
                d.RuleTriggered = "默认零缺陷规则";
            }
        }
        else
        {
            decision.Reasons.push_back("所有项目检测正常，未发现异常缺陷");
        }
    }
    else
    {
        for (int i = 0; i < rules.size(); ++i)
        {
            QJsonObject r = rules[i].toObject();
            QString ruleType = r["ruleType"].toString();
            QString ruleName = r["name"].toString(QString("规则_%1").arg(i + 1));
            std::string failReason;
            bool rulePassed = true;

            if (ruleType == "count" || r.contains("expectedCount") || r.contains("minCount"))
            {
                rulePassed = EvaluateCountRule(result.ObjectCount, r, failReason);
            }
            else if (ruleType == "defect" || r.contains("defectClass") || r.contains("maxTotalDefects"))
            {
                rulePassed = EvaluateDefectRule(result.Defects, r, failReason);
            }
            else if (ruleType == "measurement" || r.contains("paramName"))
            {
                rulePassed = EvaluateMeasurementRule(result.Measurements, r, failReason);
            }

            QJsonObject singleResult;
            singleResult["name"] = ruleName;
            singleResult["passed"] = rulePassed;
            if (!rulePassed)
            {
                anyRuleFailed = true;
                singleResult["reason"] = QString::fromStdString(failReason);
                decision.Reasons.push_back(failReason);
            }
            ruleResults[ruleName] = singleResult;
        }

        if (anyRuleFailed)
        {
            decision.Status = InspectionStatus::Fail;
        }
        else
        {
            decision.Status = InspectionStatus::Pass;
            if (decision.Reasons.empty())
            {
                decision.Reasons.push_back("全部判定规则通过，产品合格");
            }
        }
    }

    auto endTime = std::chrono::high_resolution_clock::now();
    decision.EvaluationTimeMs = std::chrono::duration<double, std::milli>(endTime - startTime).count();
    decision.RuleResults = ruleResults;

    Logger::Info("InspectionRuleEngine", "规则评估完成，判定状态: " +
                 InspectionStatusToString(decision.Status) +
                 ", 耗时: " + std::to_string(decision.EvaluationTimeMs) + " ms");

    return decision;
}
