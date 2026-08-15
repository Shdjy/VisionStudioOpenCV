#pragma once

#include "PipelineManager.h"
#include <QString>

/**
 * @brief 工程项目序列化与反序列化管理器
 */
class ProjectManager
{
public:
    /**
     * @brief 保存流水线工程到 JSON 文件
     */
    static bool SaveProject(const PipelineManager& pipeline, const QString& filePath, QString& errorMsg);

    /**
     * @brief 从 JSON 文件加载流水线工程
     */
    static bool LoadProject(PipelineManager& pipeline, const QString& filePath, QString& errorMsg);
};
