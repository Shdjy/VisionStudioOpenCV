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
