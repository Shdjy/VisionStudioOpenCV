#pragma once

#include <opencv2/opencv.hpp>
#include <string>
#include <vector>
#include <variant>
#include <memory>
#include "OpCategory.h"

/**
 * @brief 参数控件类型枚举
 */
enum class ParamType
{
    IntSlider,      // 整数滑动条 + 微调框
    DoubleSlider,   // 浮点滑动条 + 微调框
    ComboBox,       // 下拉选择框
    CheckBox        // 布尔勾选框
};

/**
 * @brief 算子参数元数据定义结构体
 */
struct OpParameter
{
    std::string Name;                                   // 参数内部标识符 (如 "ksize")
    std::string DisplayName;                            // UI 界面显示名称 (如 "核大小 (ksize)")
    std::string Tooltip;                                // 鼠标悬浮详细提示与建议
    ParamType Type;                                     // 控件呈现类型
    std::variant<int, double, bool> Value;              // 当前值
    std::variant<int, double> MinValue;                 // 最小值
    std::variant<int, double> MaxValue;                 // 最大值
    std::variant<int, double> Step;                     // 调节步长
    std::vector<std::string> EnumOptions;               // ComboBox 枚举选项列表

    int GetInt() const
    {
        if (std::holds_alternative<int>(Value))
        {
            return std::get<int>(Value);
        }
        return 0;
    }

    double GetDouble() const
    {
        if (std::holds_alternative<double>(Value))
        {
            return std::get<double>(Value);
        }
        else if (std::holds_alternative<int>(Value))
        {
            return static_cast<double>(std::get<int>(Value));
        }
        return 0.0;
    }

    bool GetBool() const
    {
        if (std::holds_alternative<bool>(Value))
        {
            return std::get<bool>(Value);
        }
        return false;
    }
};

/**
 * @brief 机器视觉算子抽象基类接口
 * 
 * 所有具体的图像处理算法（滤波、二值化、边缘、几何变换等）均继承自此类。
 * 提供计算、参数元数据管理、中文教学原理解析以及多语言代码生成接口。
 */
class IOpNode
{
public:
    virtual ~IOpNode() = default;

    /**
     * @brief 获取算子所属大类
     */
    virtual OpCategory GetCategory() const = 0;

    /**
     * @brief 获取算子唯一英文标识名 (如 "GaussianBlur")
     */
    virtual std::string GetName() const = 0;

    /**
     * @brief 获取算子中文展示名 (如 "高斯滤波 (Gaussian Blur)")
     */
    virtual std::string GetDisplayName() const = 0;

    /**
     * @brief 获取算子简明功能一句话概述
     */
    virtual std::string GetDescription() const = 0;

    /**
     * @brief 获取算子中文原理、公式与参数推荐教学文档 (支持 Markdown / 富文本)
     */
    virtual std::string GetPrincipleDoc() const = 0;

    /**
     * @brief 执行图像计算处理
     * @param input 输入图像 Mat
     * @param output 输出图像 Mat
     * @param errorMsg 错误信息输出
     * @return 成功返回 true，失败返回 false
     */
    virtual bool Process(const cv::Mat& input, cv::Mat& output, std::string& errorMsg) = 0;

    /**
     * @brief 获取算子参数元数据列表引用
     */
    virtual std::vector<OpParameter>& GetParameters() = 0;
    virtual const std::vector<OpParameter>& GetParameters() const = 0;

    /**
     * @brief 设置指定参数的值
     * @param name 参数名
     * @param value 新值
     * @return 是否设置成功
     */
    virtual bool SetParameter(const std::string& name, const std::variant<int, double, bool>& value);

    /**
     * @brief 获取指定参数的元数据指针
     */
    virtual OpParameter* FindParameter(const std::string& name);

    /**
     * @brief 生成规范的 C++ (OpenCV) 代码段
     * @param inVar 输入图像变量名 (如 "img_0")
     * @param outVar 输出图像变量名 (如 "img_1")
     * @return C++ 源码字符串
     */
    virtual std::string GenerateCppCode(const std::string& inVar, const std::string& outVar) const = 0;

    /**
     * @brief 生成规范的 Python (cv2) 代码段
     * @param inVar 输入图像变量名 (如 "img_0")
     * @param outVar 输出图像变量名 (如 "img_1")
     * @return Python 源码字符串
     */
    virtual std::string GeneratePythonCode(const std::string& inVar, const std::string& outVar) const = 0;

    /**
     * @brief 复制克隆一个独立的算子节点实例
     */
    virtual std::unique_ptr<IOpNode> Clone() const = 0;
};
