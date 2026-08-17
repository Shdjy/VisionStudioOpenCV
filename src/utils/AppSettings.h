#pragma once

#include <QString>

/**
 * @brief 应用程序全局配置与路径持久化管理器 (单例/静态服务)
 * @details 统一管理工业软件中各类文件对话框的最近工作目录，通过 QSettings 持久化存储
 */
class AppSettings
{
public:
    // 分类键名常量
    static inline const QString KeyImageDir = "Paths/LastImageDir";
    static inline const QString KeyProjectDir = "Paths/LastProjectDir";
    static inline const QString KeyTemplateDir = "Paths/LastTemplateDir";
    static inline const QString KeyExportDir = "Paths/LastExportDir";

    /**
     * @brief 获取指定业务分类的上次访问目录
     * @param key 配置键名 (例如 KeyImageDir)
     * @param fallbackDir 若历史记录不存在或无效时的回退目录 (留空时自动回退为系统推荐目录)
     * @return 绝对目录路径
     */
    static QString GetLastDirectory(const QString& key, const QString& fallbackDir = QString());

    /**
     * @brief 记录并持久化保存指定分类的访问目录
     * @param key 配置键名
     * @param pathOrFilePath 目录路径或所选文件的绝对路径 (自动提取其父目录)
     */
    static void SetLastDirectory(const QString& key, const QString& pathOrFilePath);

    /**
     * @brief 获取带有默认文件名的推荐完整保存路径
     * @param key 配置键名
     * @param defaultFileName 默认文件名 (例如 "pipeline_project.json")
     * @return 拼接后的完整文件路径
     */
    static QString GetDefaultFilePath(const QString& key, const QString& defaultFileName);
};
