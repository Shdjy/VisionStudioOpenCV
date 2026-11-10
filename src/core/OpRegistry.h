#pragma once

#include "IOpNode.h"
#include <string>
#include <vector>
#include <map>
#include <functional>
#include <memory>

/**
 * @brief 算子元数据信息结构
 */
struct OpMetadata
{
    std::string Name;
    std::string DisplayName;
    std::string Description;
    OpCategory Category;
};

/**
 * @brief 算子反射与单例注册工厂中心
 */
class OpRegistry
{
public:
    using CreatorFunc = std::function<std::unique_ptr<IOpNode>()>;

    static OpRegistry& Instance()
    {
        static OpRegistry instance;
        return instance;
    }

    /**
     * @brief 注册新算子
     */
    template <typename T>
    void Register()
    {
        auto temp = std::make_unique<T>();
        std::string name = temp->GetName();
        OpMetadata meta;
        meta.Name = name;
        meta.DisplayName = temp->GetDisplayName();
        meta.Description = temp->GetDescription();
        meta.Category = temp->GetCategory();

        _metadataMap[name] = meta;
        _creators[name] = []() -> std::unique_ptr<IOpNode> {
            return std::make_unique<T>();
        };
    }

    /**
     * @brief 根据算子名称创建实例
     */
    std::unique_ptr<IOpNode> Create(const std::string& name) const;

    /**
     * @brief 获取所有已注册算子的元数据列表
     */
    std::vector<OpMetadata> GetAllOperators() const;

    /**
     * @brief 按类别获取算子元数据列表
     */
    std::vector<OpMetadata> GetOperatorsByCategory(OpCategory category) const;

    /**
     * @brief 关键字搜索算子 (匹配名称、中文名或功能描述)
     */
    std::vector<OpMetadata> SearchOperators(const std::string& keyword) const;

    /**
     * @brief 注册系统中所有内建算子
     */
    void RegisterAllBuiltinOperators();

    /**
     * @brief 获取指定算子的详细标准元数据 (包含参数约束与类型要求)
     */
    OperatorMetadata GetOperatorMetadata(const std::string& name) const;

    /**
     * @brief 导出所有已注册算子的元数据为 JSON 数组 (供 AI 知识库与外部工具使用)
     */
    QJsonArray ExportAllMetadataJson() const;

    /**
     * @brief 生成供大模型 Prompt 使用的算子清单简要摘要文本
     */
    std::string ExportAllMetadataPromptString() const;

private:
    OpRegistry() = default;
    ~OpRegistry() = default;
    OpRegistry(const OpRegistry&) = delete;
    OpRegistry& operator=(const OpRegistry&) = delete;

    std::map<std::string, OpMetadata> _metadataMap;
    std::map<std::string, CreatorFunc> _creators;
};
