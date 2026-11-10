#include "IOpNode.h"
#include <algorithm>

namespace
{
double GetNumericLimit(const std::variant<int, double>& value)
{
    if (std::holds_alternative<int>(value))
    {
        return static_cast<double>(std::get<int>(value));
    }
    return std::get<double>(value);
}
}

bool IOpNode::SetParameter(const std::string& name, const std::variant<int, double, bool, std::string>& value)
{
    OpParameter* param = FindParameter(name);
    if (param == nullptr)
    {
        return false;
    }

    switch (param->Type)
    {
        case ParamType::IntSlider:
        {
            if (!std::holds_alternative<int>(value))
            {
                return false;
            }
            const int intValue = std::get<int>(value);
            if (intValue < GetNumericLimit(param->MinValue) ||
                intValue > GetNumericLimit(param->MaxValue))
            {
                return false;
            }
            break;
        }
        case ParamType::DoubleSlider:
        {
            if (!std::holds_alternative<double>(value) && !std::holds_alternative<int>(value))
            {
                return false;
            }
            const double doubleValue = std::holds_alternative<double>(value)
                ? std::get<double>(value)
                : static_cast<double>(std::get<int>(value));
            const double minLimit = GetNumericLimit(param->MinValue);
            const double maxLimit = GetNumericLimit(param->MaxValue);
            if (doubleValue < minLimit - 1e-5 || doubleValue > maxLimit + 1e-5)
            {
                return false;
            }
            param->Value = std::clamp(doubleValue, minLimit, maxLimit);
            return true;
        }
        case ParamType::ComboBox:
        {
            if (!std::holds_alternative<int>(value))
            {
                return false;
            }
            const int index = std::get<int>(value);
            if (index < 0 || index >= static_cast<int>(param->EnumOptions.size()))
            {
                return false;
            }
            break;
        }
        case ParamType::CheckBox:
        {
            if (!std::holds_alternative<bool>(value))
            {
                return false;
            }
            break;
        }
        case ParamType::FilePath:
        {
            if (!std::holds_alternative<std::string>(value))
            {
                return false;
            }
            break;
        }
    }

    param->Value = value;
    return true;
}

void IOpNode::AddResult(
    const std::string& name,
    const std::string& displayName,
    const std::variant<std::int64_t, double, bool, std::string>& value,
    const std::string& unit)
{
    _results.push_back({ name, displayName, value, unit });
}

OpParameter* IOpNode::FindParameter(const std::string& name)
{
    for (auto& param : GetParameters())
    {
        if (param.Name == name)
        {
            return &param;
        }
    }
    return nullptr;
}

OperatorMetadata IOpNode::GetMetadata() const
{
    OperatorMetadata meta;
    meta.Name = GetName();
    meta.DisplayName = GetDisplayName();
    meta.Description = GetDescription();
    meta.Category = OpCategoryUtils::ToString(GetCategory());
    meta.Version = "1.0";
    meta.SupportsMultiThread = true;

    // 参数元数据映射
    for (const auto& p : GetParameters())
    {
        ParameterMetadata pm;
        pm.Name = p.Name;
        pm.DisplayName = p.DisplayName;
        pm.Description = p.Tooltip;
        pm.EnumValues = p.EnumOptions;

        switch (p.Type)
        {
            case ParamType::IntSlider:
            {
                pm.Type = "int";
                pm.MinValue = GetNumericLimit(p.MinValue);
                pm.MaxValue = GetNumericLimit(p.MaxValue);
                pm.Step = GetNumericLimit(p.Step);
                pm.DefaultValue = std::to_string(p.GetInt());
                break;
            }
            case ParamType::DoubleSlider:
            {
                pm.Type = "double";
                pm.MinValue = GetNumericLimit(p.MinValue);
                pm.MaxValue = GetNumericLimit(p.MaxValue);
                pm.Step = GetNumericLimit(p.Step);
                pm.DefaultValue = std::to_string(p.GetDouble());
                break;
            }
            case ParamType::ComboBox:
            {
                pm.Type = "enum";
                pm.MinValue = 0;
                pm.MaxValue = static_cast<double>(p.EnumOptions.size() > 0 ? p.EnumOptions.size() - 1 : 0);
                pm.Step = 1.0;
                pm.DefaultValue = std::to_string(p.GetInt());
                break;
            }
            case ParamType::CheckBox:
            {
                pm.Type = "bool";
                pm.DefaultValue = p.GetBool() ? "true" : "false";
                break;
            }
            case ParamType::FilePath:
            {
                pm.Type = "file";
                pm.DefaultValue = p.GetString();
                break;
            }
        }
        meta.Parameters.push_back(pm);
    }

    // 根据算子类别推导默认输入输出类型及前置条件
    switch (GetCategory())
    {
        case OpCategory::Thresholding:
        {
            meta.InputTypes = { DataType::GrayImage };
            meta.OutputTypes = { DataType::BinaryImage };
            meta.Prerequisites.push_back("输入图像建议为单通道灰度图");
            meta.UseCases.push_back("目标与背景二值化分割、缺陷粗提取");
            break;
        }
        case OpCategory::EdgeDetection:
        {
            meta.InputTypes = { DataType::GrayImage, DataType::Image };
            meta.OutputTypes = { DataType::BinaryImage, DataType::GrayImage };
            meta.Prerequisites.push_back("建议先进行高斯或中值滤波降噪");
            meta.UseCases.push_back("边界轮廓提取、几何边缘定位");
            break;
        }
        case OpCategory::Morphology:
        {
            meta.InputTypes = { DataType::BinaryImage, DataType::GrayImage };
            meta.OutputTypes = { DataType::BinaryImage, DataType::GrayImage };
            meta.UseCases.push_back("形态学膨胀/腐蚀、开闭运算消除毛刺和孤立噪点");
            break;
        }
        case OpCategory::ContourAnalysis:
        {
            meta.InputTypes = { DataType::BinaryImage };
            meta.OutputTypes = { DataType::Contours, DataType::Image };
            meta.Prerequisites.push_back("输入必须为二值化图像");
            meta.UseCases.push_back("工件轮廓查找、几何形状分析、面积周长测量");
            break;
        }
        case OpCategory::ColorAndChannels:
        {
            meta.InputTypes = { DataType::Image };
            meta.OutputTypes = { DataType::GrayImage, DataType::Image };
            meta.UseCases.push_back("颜色空间转换、提取灰度通道、HSV 色彩过滤");
            break;
        }
        case OpCategory::HoughTransform:
        {
            meta.InputTypes = { DataType::Image, DataType::GrayImage, DataType::BinaryImage };
            meta.OutputTypes = { DataType::Measurements, DataType::Image };
            meta.UseCases.push_back("工业尺寸测量、圆拟合、直线拟合、卡尺测距");
            break;
        }
        default:
        {
            meta.InputTypes = { DataType::Image };
            meta.OutputTypes = { DataType::Image };
            meta.UseCases.push_back("通用图像处理与变换");
            break;
        }
    }

    return meta;
}
