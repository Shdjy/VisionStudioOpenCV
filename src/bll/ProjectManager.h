#pragma once

#include "PipelineManager.h"
#include <QString>

class DetectionTaskManager;

/**
 * @brief 工程项目序列化与反序列化管理器 (升级支持 V2.0 工业检测与多分支架构，保持 1.0 完全兼容)
 */
class ProjectManager
{
public:
    /**
     * @brief 保存流水线与检测工程到 JSON 文件 (V2.0 格式)
     * @param pipeline 流水线调度管理器引用
     * @param taskManager 检测任务管理器指针 (可选)
     * @param filePath 目标保存文件绝对路径
     * @param errorMsg 错误输出信息
     * @return 成功返回 true，失败返回 false
     */
    static bool SaveProject(
        const PipelineManager& pipeline,
        const DetectionTaskManager* taskManager,
        const QString& filePath,
        QString& errorMsg);

    /**
     * @brief 兼容原有 2 参数签名保存工程
     */
    static bool SaveProject(
        const PipelineManager& pipeline,
        const QString& filePath,
        QString& errorMsg);

    /**
     * @brief 从 JSON 文件加载工程 (自适应兼容 V1.0 和 V2.0)
     * @param pipeline 流水线调度管理器引用
     * @param taskManager 检测任务管理器指针 (可选)
     * @param filePath 目标工程文件绝对路径
     * @param errorMsg 错误输出信息
     * @return 成功返回 true，失败返回 false
     */
    static bool LoadProject(
        PipelineManager& pipeline,
        DetectionTaskManager* taskManager,
        const QString& filePath,
        QString& errorMsg);

    /**
     * @brief 兼容原有 2 参数签名加载工程
     */
    static bool LoadProject(
        PipelineManager& pipeline,
        const QString& filePath,
        QString& errorMsg);
};
