#pragma once

#include <opencv2/opencv.hpp>
#include <string>
#include <vector>
#include <variant>
#include <memory>
#include <cstdint>
#include "OpCategory.h"
#include "OperatorMetadata.h"

/**
 * @brief 参数控件类型枚举
 */
enum class ParamType
{
    IntSlider,      // 整数滑动条 + 微调框
    DoubleSlider,   // 浮点滑动条 + 微调框
    ComboBox,       // 下拉选择框
    CheckBox,       // 布尔勾选框
    FilePath        // 本地文件路径选择器 (带浏览按钮)
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
    std::variant<int, double, bool, std::string> Value; // 当前值
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

    std::string GetString() const
    {
        if (std::holds_alternative<std::string>(Value))
        {
            return std::get<std::string>(Value);
        }
        return "";
    }
};

/**
 * @brief 算子输出的通用结构化结果项
 *
 * 用于承载统计值、几何测量值和解码文本等纯 OpenCV 结果，避免只能把结果绘制到图像上。
 */
struct OpResultItem
{
    std::string Name;
    std::string DisplayName;
    std::variant<std::int64_t, double, bool, std::string> Value;
    std::string Unit;
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
    virtual bool SetParameter(const std::string& name, const std::variant<int, double, bool, std::string>& value);

    /**
     * @brief 获取指定参数的元数据指针
     */
    virtual OpParameter* FindParameter(const std::string& name);

    /**
     * @brief 获取当前算子的标准化完整元数据
     * @details 提供输入输出类型、参数约束、场景与前置条件描述，供 UI、校验器和 AI 知识库使用
     * @return 标准算子元数据结构
     */
    virtual OperatorMetadata GetMetadata() const;

    /**
     * @brief 获取最近一次执行产生的结构化结果
     */
    const std::vector<OpResultItem>& GetResults() const { return _results; }

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

protected:
    void ClearResults() { _results.clear(); }
    void AddResult(const std::string& name,
                   const std::string& displayName,
                   const std::variant<std::int64_t, double, bool, std::string>& value,
                   const std::string& unit = "");

private:
    std::vector<OpResultItem> _results;
};
