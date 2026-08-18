#pragma once

#include <QColor>
#include <QIcon>

/**
 * @brief 矢量 UI 图标生成工具类
 * @details 提供符合工业设计规范的高清、抗锯齿矢量图标生成，确保深色主题与高 DPI 显示下的视觉一致性
 */
class IconUtils
{
public:
    /**
     * @brief 创建标准项目/数据导入 (Import) 托盘图标 (向下箭头+U型托盘)
     * @param color 图标前景色 (默认浅灰 #DCDCDC)
     * @param size 渲染基准分辨率 (默认 64px)
     * @return 高清矢量 QIcon
     */
    static QIcon CreateImportIcon(const QColor& color = QColor(220, 220, 220), int size = 64);

    /**
     * @brief 创建标准项目/数据导出 (Export) 托盘图标 (向上箭头+U型托盘)
     * @param color 图标前景色 (默认浅灰 #DCDCDC)
     * @param size 渲染基准分辨率 (默认 64px)
     * @return 高清矢量 QIcon
     */
    static QIcon CreateExportIcon(const QColor& color = QColor(220, 220, 220), int size = 64);

    /**
     * @brief 创建醒目抗锯齿红色 ❌ 删除图标
     * @param color 图标前景色 (默认工业红 #EB3C3C)
     * @param size 渲染基准分辨率 (默认 64px)
     * @return 高清矢量 QIcon
     */
    static QIcon CreateRedCrossIcon(const QColor& color = QColor(235, 60, 60), int size = 64);

    /**
     * @brief 创建一键全选/全部启用图标 (带勾选框)
     * @param color 图标前景色 (默认嫩绿 #4CAF50)
     * @param size 渲染基准分辨率 (默认 64px)
     * @return 高清矢量 QIcon
     */
    static QIcon CreateSelectAllIcon(const QColor& color = QColor(76, 175, 80), int size = 64);

    /**
     * @brief 创建一键取消/全部禁用图标 (空框或带斜杠方框)
     * @param color 图标前景色 (默认浅灰 #B0B0B0)
     * @param size 渲染基准分辨率 (默认 64px)
     * @return 高清矢量 QIcon
     */
    static QIcon CreateDeselectAllIcon(const QColor& color = QColor(176, 176, 176), int size = 64);
};
