#pragma once

#include "DetectionTask.h"
#include "InspectionRuleEngine.h"
#include "ObjectCounter.h"
#include "DefectDetector.h"
#include "GeometryMeasurement.h"
#include <opencv2/opencv.hpp>
#include <string>

/**
 * @brief 检测任务执行与调度器 (DetectionExecutor)
 * 
 * 协调执行检测任务算法引擎、调用业务规则判定引擎、并在最终输出图上叠加 OK/NG 判定水印。
 */
class DetectionExecutor
{
public:
    /**
     * @brief 执行指定的检测任务
     * @param input 输入待检工件图像
     * @param task 检测任务定义 (包含算法参数与规则)
     * @param outResult 输出检测数据与叠加图
     * @param outDecision 输出业务判定决策
     * @param errorMsg 错误输出
     * @return 成功返回 true，算法异常返回 false
     */
    static bool ExecuteTask(
        const cv::Mat& input,
        const DetectionTask& task,
        DetectionResult& outResult,
        InspectionDecision& outDecision,
        std::string& errorMsg);

private:
    static void OverlayVerdictBadge(cv::Mat& canvas, const InspectionDecision& decision);
};
