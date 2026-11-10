#pragma once

#include <QString>
#include <QStringList>
#include <QJsonArray>
#include <QJsonObject>
#include <functional>

/**
 * @brief AI 服务提供商统一抽象接口 (DIP / ISP)
 * 
 * 屏蔽不同云端与本地大模型 (OpenAI, DeepSeek, Local Ollama 等) 的底层协议差异，
 * 支持异步获取在线模型清单、自然语言/多模态视觉对话及算法规划生成。
 */
class IAiProvider
{
public:
    virtual ~IAiProvider() = default;

    /**
     * @brief 异步获取服务商当前支持的模型列表 (对应 GET /models)
     * @param callback 回调函数: (成功标志, 模型ID列表, 错误信息)
     */
    virtual void FetchModelsAsync(
        std::function<void(bool success, const QStringList& models, const QString& error)> callback) = 0;

    /**
     * @brief 异步发送聊天补全与视觉分析请求 (对应 POST /chat/completions)
     * @param messages 多轮对话上下文或包含 Base64 图像的多模态消息数组
     * @param model 使用的模型标识名 (如 "gpt-6.1-sol")
     * @param temperature 采样随机度
     * @param maxTokens 最大生成令牌数
     * @param callback 回调函数: (成功标志, AI 回复文本, 错误信息)
     */
    virtual void ChatCompletionAsync(
        const QJsonArray& messages,
        const QString& model,
        double temperature,
        int maxTokens,
        std::function<void(bool success, const QString& reply, const QString& error)> callback) = 0;
};
