#pragma once

#include <string>
#include <vector>
#include <opencv2/opencv.hpp>
#include <QJsonObject>
#include <QJsonArray>

/**
 * @brief 工业检测任务类型枚举
 */
enum class DetectionTaskType
{
    ObjectCounting,         // 目标计数 (如瓶盖、药片、工件数量统计)
    DefectDetection,        // 表面缺陷检测 (划痕、凹坑、污渍、裂纹)
    GeometryMeasurement,    // 几何尺寸测量 (长度、宽度、直径、面积)
    ObjectDetection,        // 目标定位检测
    PresenceCheck           // 存在性/装配到位检测
};

/**
 * @brief 检测任务类型转中文描述
 */
inline std::string DetectionTaskTypeToString(DetectionTaskType type)
{
    switch (type)
    {
        case DetectionTaskType::ObjectCounting:
            return "目标计数 (Object Counting)";
        case DetectionTaskType::DefectDetection:
            return "缺陷检测 (Defect Detection)";
        case DetectionTaskType::GeometryMeasurement:
            return "几何测量 (Geometry Measurement)";
        case DetectionTaskType::ObjectDetection:
            return "目标检测 (Object Detection)";
        case DetectionTaskType::PresenceCheck:
            return "存在性检测 (Presence Check)";
        default:
            return "未知任务";
    }
}

/**
 * @brief 字符串转检测任务类型
 */
inline DetectionTaskType StringToDetectionTaskType(const std::string& str)
{
    if (str.find("Defect") != std::string::npos || str.find("缺陷") != std::string::npos)
    {
        return DetectionTaskType::DefectDetection;
    }
    if (str.find("Measurement") != std::string::npos || str.find("测量") != std::string::npos)
    {
        return DetectionTaskType::GeometryMeasurement;
    }
    if (str.find("ObjectDetection") != std::string::npos)
    {
        return DetectionTaskType::ObjectDetection;
    }
    if (str.find("Presence") != std::string::npos || str.find("到位") != std::string::npos)
    {
        return DetectionTaskType::PresenceCheck;
    }
    return DetectionTaskType::ObjectCounting;
}

/**
 * @brief 检测任务类型转英文标准化 ID 字符串
 */
inline std::string DetectionTaskTypeToIdString(DetectionTaskType type)
{
    switch (type)
    {
        case DetectionTaskType::ObjectCounting:
            return "ObjectCounting";
        case DetectionTaskType::DefectDetection:
            return "DefectDetection";
        case DetectionTaskType::GeometryMeasurement:
            return "GeometryMeasurement";
        case DetectionTaskType::ObjectDetection:
            return "ObjectDetection";
        case DetectionTaskType::PresenceCheck:
            return "PresenceCheck";
        default:
            return "ObjectCounting";
    }
}

/**
 * @brief 目标检测包围框与语义信息
 */
struct BoundingBox
{
    cv::Rect2f Rect;                            // 边界框 (x, y, width, height)
    int ClassId = 0;                            // 类别 ID
    float Confidence = 1.0f;                    // 置信度 (0.0 ~ 1.0)
    std::string Label;                          // 类别标签名 (如 "bottle_cap", "part")
    cv::Point2f Center;                         // 中心点坐标
    double Area = 0.0;                          // 像素面积
    double AspectRatio = 1.0;                   // 宽高比
    double Circularity = 1.0;                   // 圆度

    QJsonObject ToJson() const
    {
        QJsonObject obj;
        obj["x"] = Rect.x;
        obj["y"] = Rect.y;
        obj["width"] = Rect.width;
        obj["height"] = Rect.height;
        obj["classId"] = ClassId;
        obj["confidence"] = Confidence;
        obj["label"] = QString::fromStdString(Label);
        obj["centerX"] = Center.x;
        obj["centerY"] = Center.y;
        obj["area"] = Area;
        obj["aspectRatio"] = AspectRatio;
        obj["circularity"] = Circularity;
        return obj;
    }
};

/**
 * @brief 工件表面缺陷详细信息
 */
struct DefectInfo
{
    std::string DefectType;                     // 缺陷类型 (如 "scratch", "pit", "dent", "stain", "crack")
    std::string DefectDisplayName;              // 中文名称 (如 "划痕", "凹坑", "污渍")
    float Confidence = 1.0f;                    // 异常置信度
    cv::Rect BoundingBox;                       // 缺陷外接矩形
    cv::Mat Mask;                               // 缺陷精确像素级二值掩膜 (可为空)
    double Area = 0.0;                          // 缺陷面积 (像素或 mm²)
    double Length = 0.0;                        // 缺陷最大长度 (像素或 mm)
    double Width = 0.0;                         // 缺陷最大宽度 (像素或 mm)
    double AspectRatio = 1.0;                   // 缺陷长宽比
    double Angle = 0.0;                         // 缺陷倾斜主角度 (度)
    bool IsNG = false;                          // 该缺陷是否触发了 NG 判定
    std::string RuleTriggered;                  // 触发的具体判定规则名称

    QJsonObject ToJson() const
    {
        QJsonObject obj;
        obj["defectType"] = QString::fromStdString(DefectType);
        obj["displayName"] = QString::fromStdString(DefectDisplayName);
        obj["confidence"] = Confidence;
        obj["x"] = BoundingBox.x;
        obj["y"] = BoundingBox.y;
        obj["width"] = BoundingBox.width;
        obj["height"] = BoundingBox.height;
        obj["area"] = Area;
        obj["length"] = Length;
        obj["width_dim"] = Width;
        obj["aspectRatio"] = AspectRatio;
        obj["angle"] = Angle;
        obj["isNG"] = IsNG;
        obj["ruleTriggered"] = QString::fromStdString(RuleTriggered);
        return obj;
    }
};

/**
 * @brief 结构化算法检测输出结果 (与业务规则判定解耦)
 */
struct DetectionResult
{
    std::vector<BoundingBox> Objects;           // 检测到的目标集合
    std::vector<DefectInfo> Defects;            // 检测到的缺陷集合
    int ObjectCount = 0;                        // 目标总数
    int DefectCount = 0;                        // 缺陷总数
    cv::Mat Visualization;                      // 叠加绘制后的可视化结果图
    QJsonObject Measurements;                   // 几何测量项字典
    QJsonObject Statistics;                     // 统计指标
    double ExecutionTimeMs = 0.0;               // 算法执行耗时 (毫秒)

    QJsonObject ToJson() const
    {
        QJsonObject obj;
        obj["objectCount"] = ObjectCount;
        obj["defectCount"] = DefectCount;
        obj["executionTimeMs"] = ExecutionTimeMs;

        QJsonArray objsArr;
        for (const auto& o : Objects)
        {
            objsArr.append(o.ToJson());
        }
        obj["objects"] = objsArr;

        QJsonArray defsArr;
        for (const auto& d : Defects)
        {
            defsArr.append(d.ToJson());
        }
        obj["defects"] = defsArr;
        obj["measurements"] = Measurements;
        obj["statistics"] = Statistics;
        return obj;
    }
};

/**
 * @brief 工业检测最终业务判定状态
 */
enum class InspectionStatus
{
    Unknown,    // 未判定或处理中
    Pass,       // 合格 (OK)
    Fail,       // 不合格 (NG)
    Error       // 异常中断
};

/**
 * @brief 判定状态转中文描述
 */
inline std::string InspectionStatusToString(InspectionStatus status)
{
    switch (status)
    {
        case InspectionStatus::Pass:
            return "OK (合格)";
        case InspectionStatus::Fail:
            return "NG (不合格)";
        case InspectionStatus::Error:
            return "ERROR (执行异常)";
        case InspectionStatus::Unknown:
        default:
            return "UNKNOWN (未知)";
    }
}

/**
 * @brief 工业检验决策判定结果 (由 InspectionRuleEngine 产生)
 */
struct InspectionDecision
{
    InspectionStatus Status = InspectionStatus::Unknown;
    std::vector<std::string> Reasons;           // 判定为 NG 或 Pass 的原因详情列表
    QJsonObject RuleResults;                    // 各规则计算明细
    double EvaluationTimeMs = 0.0;              // 规则计算耗时 (毫秒)

    bool IsOK() const
    {
        return Status == InspectionStatus::Pass;
    }

    QJsonObject ToJson() const
    {
        QJsonObject obj;
        obj["status"] = QString::fromStdString(InspectionStatusToString(Status));
        obj["isOK"] = IsOK();
        obj["evaluationTimeMs"] = EvaluationTimeMs;

        QJsonArray reasonsArr;
        for (const auto& r : Reasons)
        {
            reasonsArr.append(QString::fromStdString(r));
        }
        obj["reasons"] = reasonsArr;
        obj["ruleResults"] = RuleResults;
        return obj;
    }
};

/**
 * @brief 工业检测任务定义
 */
struct DetectionTask
{
    std::string Id;                             // 任务唯一标识 ID (如 "task_count_001")
    std::string Name;                           // 任务中文名称 (如 "瓶盖计数与完整性检测")
    DetectionTaskType Type = DetectionTaskType::ObjectCounting; // 任务分类
    bool Enabled = true;                        // 是否启用
    QJsonObject Configuration;                  // 算法参数配置 (阈值、滤波、ROI 等)
    QJsonArray Rules;                           // 业务判定规则 (OK/NG 阈值限制)

    QJsonObject ToJson() const
    {
        QJsonObject obj;
        obj["id"] = QString::fromStdString(Id);
        obj["name"] = QString::fromStdString(Name);
        obj["type"] = QString::fromStdString(DetectionTaskTypeToIdString(Type));
        obj["displayName"] = QString::fromStdString(DetectionTaskTypeToString(Type));
        obj["enabled"] = Enabled;
        obj["configuration"] = Configuration;
        obj["rules"] = Rules;
        return obj;
    }

    static DetectionTask FromJson(const QJsonObject& obj)
    {
        DetectionTask task;
        task.Id = obj["id"].toString().toStdString();
        task.Name = obj["name"].toString().toStdString();
        task.Type = StringToDetectionTaskType(obj["type"].toString().toStdString());
        task.Enabled = obj["enabled"].toBool(true);
        task.Configuration = obj["configuration"].toObject();
        task.Rules = obj["rules"].toArray();
        return task;
    }
};
