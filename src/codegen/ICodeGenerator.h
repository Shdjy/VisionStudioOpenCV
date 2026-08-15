#pragma once

#include "../bll/PipelineManager.h"
#include <string>

/**
 * @brief 代码生成器抽象接口
 */
class ICodeGenerator
{
public:
    virtual ~ICodeGenerator() = default;

    /**
     * @brief 生成目标语言的完整可运行代码
     * @param pipeline 流水线管理器
     * @param inputImagePath 输入测试图像路径
     * @return 源码字符串
     */
    virtual std::string Generate(const PipelineManager& pipeline, const std::string& inputImagePath = "test.jpg") const = 0;

    /**
     * @brief 获取语言名称 (如 "C++ (OpenCV)", "Python (cv2)")
     */
    virtual std::string GetLanguageName() const = 0;
};
