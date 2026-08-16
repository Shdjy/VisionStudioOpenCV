#include "IOpNode.h"

bool IOpNode::SetParameter(const std::string& name, const std::variant<int, double, bool, std::string>& value)
{
    OpParameter* param = FindParameter(name);
    if (param != nullptr)
    {
        param->Value = value;
        return true;
    }
    return false;
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
