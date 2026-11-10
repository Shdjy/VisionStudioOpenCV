#include "DetectionExecutor.h"
#include "../utils/Logger.h"

void DetectionExecutor::OverlayVerdictBadge(cv::Mat& canvas, const InspectionDecision& decision)
{
    if (canvas.empty())
    {
        return;
    }

    if (canvas.channels() == 1)
    {
        cv::cvtColor(canvas, canvas, cv::COLOR_GRAY2BGR);
    }

    bool isOk = decision.IsOK();
    std::string verdictText = isOk ? "PASS [ OK ]" : "FAIL [ NG ]";
    cv::Scalar bgColor = isOk ? cv::Scalar(0, 160, 0) : cv::Scalar(0, 0, 200);
    cv::Scalar textColor(255, 255, 255);

    int baseLine = 0;
    cv::Size textSize = cv::getTextSize(verdictText, cv::FONT_HERSHEY_DUPLEX, 1.0, 2, &baseLine);

    // 位于右上角
    int pad = 12;
    int x2 = canvas.cols - pad;
    int x1 = x2 - textSize.width - 24;
    int y1 = pad;
    int y2 = y1 + textSize.height + 20;

    if (x1 > 0 && y2 < canvas.rows)
    {
        cv::rectangle(canvas, cv::Point(x1, y1), cv::Point(x2, y2), bgColor, -1);
        cv::rectangle(canvas, cv::Point(x1, y1), cv::Point(x2, y2), cv::Scalar(255, 255, 255), 2);
        cv::putText(canvas, verdictText, cv::Point(x1 + 12, y1 + textSize.height + 6),
                    cv::FONT_HERSHEY_DUPLEX, 1.0, textColor, 2, cv::LINE_AA);

        // 若 NG 且有具体原因，在下方增加简要原因说明条
        if (!isOk && !decision.Reasons.empty())
        {
            std::string reason = decision.Reasons[0];
            if (reason.length() > 36)
            {
                reason = reason.substr(0, 33) + "...";
            }
            int rBase = 0;
            cv::Size rSize = cv::getTextSize(reason, cv::FONT_HERSHEY_SIMPLEX, 0.45, 1, &rBase);
            int rx1 = x2 - rSize.width - 16;
            int ry1 = y2 + 4;
            int ry2 = ry1 + rSize.height + 10;
            if (rx1 > 0 && ry2 < canvas.rows)
            {
                cv::rectangle(canvas, cv::Point(rx1, ry1), cv::Point(x2, ry2), cv::Scalar(20, 20, 20), -1);
                cv::rectangle(canvas, cv::Point(rx1, ry1), cv::Point(x2, ry2), cv::Scalar(0, 0, 200), 1);
                cv::putText(canvas, reason, cv::Point(rx1 + 8, ry1 + rSize.height + 3),
                            cv::FONT_HERSHEY_SIMPLEX, 0.45, cv::Scalar(0, 180, 255), 1, cv::LINE_AA);
            }
        }
    }
}

bool DetectionExecutor::ExecuteTask(
    const cv::Mat& input,
    const DetectionTask& task,
    DetectionResult& outResult,
    InspectionDecision& outDecision,
    std::string& errorMsg)
{
    if (input.empty())
    {
        errorMsg = "输入图像为空，无法执行检测任务";
        return false;
    }

    bool execOk = false;
    switch (task.Type)
    {
        case DetectionTaskType::ObjectCounting:
        {
            ObjectCountConfig cfg = ObjectCountConfig::FromJson(task.Configuration);
            execOk = ObjectCounter::Execute(input, cfg, outResult, errorMsg);
            break;
        }
        case DetectionTaskType::DefectDetection:
        {
            DefectDetectConfig cfg = DefectDetectConfig::FromJson(task.Configuration);
            execOk = DefectDetector::Execute(input, cfg, outResult, errorMsg);
            break;
        }
        case DetectionTaskType::GeometryMeasurement:
        {
            MeasurementConfig cfg = MeasurementConfig::FromJson(task.Configuration);
            execOk = GeometryMeasurement::Execute(input, cfg, outResult, errorMsg);
            break;
        }
        default:
        {
            // 兼容降级为目标计数
            ObjectCountConfig cfg = ObjectCountConfig::FromJson(task.Configuration);
            execOk = ObjectCounter::Execute(input, cfg, outResult, errorMsg);
            break;
        }
    }

    if (!execOk)
    {
        outDecision.Status = InspectionStatus::Error;
        outDecision.Reasons.push_back("检测算法执行失败: " + errorMsg);
        Logger::Error("DetectionExecutor", "任务 " + task.Name + " 执行失败: " + errorMsg);
        return false;
    }

    // 运行业务规则判定引擎
    outDecision = InspectionRuleEngine::Evaluate(outResult, task.Rules);

    // 在最终可视化画布上添加评定徽标
    OverlayVerdictBadge(outResult.Visualization, outDecision);

    Logger::Info("DetectionExecutor", "检测任务 [" + task.Name + "] 执行并判定完成，最终结果: " +
                 InspectionStatusToString(outDecision.Status));
    return true;
}
