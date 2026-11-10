#pragma once

#include <string>
#include <vector>
#include <QJsonObject>
#include <QJsonArray>

/**
 * @brief 数据类型枚举 (用于算子输入输出、检测结果和元数据描述)
 */
enum class DataType
{
    Image,              // 通用图像 (cv::Mat)
    GrayImage,          // 单通道灰度图像
    BinaryImage,        // 单通道二值化图像
    Mask,               // 掩膜图像
    Contours,           // 轮廓数据
    BoundingBoxes,      // 目标边界框集合
    DetectionResults,   // 结构化检测结果
    Measurements,       // 几何测量结果
    Numeric,            // 纯数值结果
    Unknown             // 未知或通用类型
};

/**
 * @brief 数据类型转中文描述字符串
 * @param type 数据类型
 * @return 描述字符串
 */
inline std::string DataTypeToString(DataType type)
{
    switch (type)
    {
        case DataType::Image:
            return "Image (通用图像)";
        case DataType::GrayImage:
            return "GrayImage (灰度图像)";
        case DataType::BinaryImage:
            return "BinaryImage (二值图像)";
        case DataType::Mask:
            return "Mask (掩膜)";
        case DataType::Contours:
            return "Contours (轮廓)";
        case DataType::BoundingBoxes:
            return "BoundingBoxes (检测框)";
        case DataType::DetectionResults:
            return "DetectionResults (检测结果)";
        case DataType::Measurements:
            return "Measurements (测量值)";
        case DataType::Numeric:
            return "Numeric (数值)";
        case DataType::Unknown:
        default:
            return "Unknown";
    }
}

/**
 * @brief 字符串转数据类型
 * @param str 类型字符串
 * @return 数据类型
 */
inline DataType StringToDataType(const std::string& str)
{
    if (str == "GrayImage" || str == "gray")
    {
        return DataType::GrayImage;
    }
    if (str == "BinaryImage" || str == "binary")
    {
        return DataType::BinaryImage;
    }
    if (str == "Mask" || str == "mask")
    {
        return DataType::Mask;
    }
    if (str == "Contours" || str == "contours")
    {
        return DataType::Contours;
    }
    if (str == "BoundingBoxes" || str == "boxes")
    {
        return DataType::BoundingBoxes;
    }
    if (str == "DetectionResults")
    {
        return DataType::DetectionResults;
    }
    if (str == "Measurements")
    {
        return DataType::Measurements;
    }
    if (str == "Numeric")
    {
        return DataType::Numeric;
    }
    if (str == "Image" || str == "image")
    {
        return DataType::Image;
    }
    return DataType::Unknown;
}

/**
 * @brief 参数元数据详细描述结构
 */
struct ParameterMetadata
{
    std::string Name;                                   // 参数内部英文标识名
    std::string DisplayName;                            // UI 显示名称
    std::string Description;                            // 参数说明与建议
    std::string Type;                                   // 参数类型: "int", "double", "bool", "string", "file"
    std::string DefaultValue;                           // 默认值字符串表示
    bool Required = false;                              // 是否为必填参数
    double MinValue = 0.0;                              // 最小值
    double MaxValue = 0.0;                              // 最大值
    double Step = 1.0;                                  // 调节步长
    std::string Unit;                                   // 物理或算法单位 (如 "px", "mm", "ms")
    std::vector<std::string> EnumValues;                // 枚举可选项

    /**
     * @brief 转换为 QJsonObject 方便序列化与 AI 知识库导出
     */
    QJsonObject ToJson() const
    {
        QJsonObject obj;
        obj["name"] = QString::fromStdString(Name);
        obj["displayName"] = QString::fromStdString(DisplayName);
        obj["description"] = QString::fromStdString(Description);
        obj["type"] = QString::fromStdString(Type);
        obj["defaultValue"] = QString::fromStdString(DefaultValue);
        obj["required"] = Required;
        obj["minValue"] = MinValue;
        obj["maxValue"] = MaxValue;
        obj["step"] = Step;
        obj["unit"] = QString::fromStdString(Unit);

        if (!EnumValues.empty())
        {
            QJsonArray arr;
            for (const auto& item : EnumValues)
            {
                arr.append(QString::fromStdString(item));
            }
            obj["enumValues"] = arr;
        }

        return obj;
    }
};

/**
 * @brief 算子标准元数据结构 (供 UI、PipelineValidator 与 AI 知识库共享使用)
 */
struct OperatorMetadata
{
    std::string Name;                                   // 算子英文唯一名 (如 "GaussianBlur")
    std::string DisplayName;                            // 算子中文展示名 (如 "高斯滤波 (Gaussian Blur)")
    std::string Category;                               // 算子所属大类
    std::string Description;                            // 算子简明功能说明
    std::vector<DataType> InputTypes;                   // 兼容的输入数据类型列表
    std::vector<DataType> OutputTypes;                  // 产生的数据类型列表
    std::vector<ParameterMetadata> Parameters;          // 参数元数据列表
    std::vector<std::string> UseCases;                  // 典型应用场景
    std::vector<std::string> Prerequisites;              // 执行前置条件 (如 "输入必须为灰度图")
    std::vector<std::string> Limitations;                // 局限与已知限制
    bool SupportsMultiThread = true;                     // 是否线程安全
    std::string Version = "1.0";                        // 算子版本

    /**
     * @brief 序列化为 QJsonObject 导出
     */
    QJsonObject ToJson() const
    {
        QJsonObject obj;
        obj["name"] = QString::fromStdString(Name);
        obj["displayName"] = QString::fromStdString(DisplayName);
        obj["category"] = QString::fromStdString(Category);
        obj["description"] = QString::fromStdString(Description);
        obj["version"] = QString::fromStdString(Version);
        obj["supportsMultiThread"] = SupportsMultiThread;

        QJsonArray inputsArr;
        for (auto t : InputTypes)
        {
            inputsArr.append(QString::fromStdString(DataTypeToString(t)));
        }
        obj["inputTypes"] = inputsArr;

        QJsonArray outputsArr;
        for (auto t : OutputTypes)
        {
            outputsArr.append(QString::fromStdString(DataTypeToString(t)));
        }
        obj["outputTypes"] = outputsArr;

        QJsonArray paramsArr;
        for (const auto& p : Parameters)
        {
            paramsArr.append(p.ToJson());
        }
        obj["parameters"] = paramsArr;

        if (!UseCases.empty())
        {
            QJsonArray arr;
            for (const auto& uc : UseCases)
            {
                arr.append(QString::fromStdString(uc));
            }
            obj["useCases"] = arr;
        }

        if (!Prerequisites.empty())
        {
            QJsonArray arr;
            for (const auto& pr : Prerequisites)
            {
                arr.append(QString::fromStdString(pr));
            }
            obj["prerequisites"] = arr;
        }

        if (!Limitations.empty())
        {
            QJsonArray arr;
            for (const auto& lm : Limitations)
            {
                arr.append(QString::fromStdString(lm));
            }
            obj["limitations"] = arr;
        }

        return obj;
    }
};
