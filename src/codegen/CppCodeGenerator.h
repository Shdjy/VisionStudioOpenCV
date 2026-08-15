#pragma once

#include "ICodeGenerator.h"

/**
 * @brief C++ (OpenCV) 生产级代码生成器
 */
class CppCodeGenerator : public ICodeGenerator
{
public:
    CppCodeGenerator() = default;
    ~CppCodeGenerator() override = default;

    std::string Generate(const PipelineManager& pipeline, const std::string& inputImagePath = "test.jpg") const override;
    std::string GetLanguageName() const override { return "C++ (OpenCV)"; }
};
