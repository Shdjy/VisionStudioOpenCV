#pragma once

#include "ICodeGenerator.h"

/**
 * @brief Python (cv2) 生产级代码生成器
 */
class PythonCodeGenerator : public ICodeGenerator
{
public:
    PythonCodeGenerator() = default;
    ~PythonCodeGenerator() override = default;

    std::string Generate(const PipelineManager& pipeline, const std::string& inputImagePath = "test.jpg") const override;
    std::string GetLanguageName() const override { return "Python (cv2)"; }
};
