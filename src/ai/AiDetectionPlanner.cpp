#include "AiDetectionPlanner.h"
#include "OperatorKnowledgeBase.h"
#include "../utils/Logger.h"
#include <QByteArray>
#include <vector>

AiDetectionPlanner::AiDetectionPlanner(IAiProvider* provider, QObject* parent)
    : QObject(parent), _provider(provider)
{
}

QString AiDetectionPlanner::EncodeImageToBase64(const cv::Mat& img) const
{
    if (img.empty())
    {
        return QString();
    }

    // 若图像分辨率过大，适当按比例等比缩放至最长边 1024 像素，减少 token 消耗与网络开销
    cv::Mat resized;
    int maxDim = std::max(img.cols, img.rows);
    if (maxDim > 1024)
    {
        double scale = 1024.0 / maxDim;
        cv::resize(img, resized, cv::Size(), scale, scale, cv::INTER_AREA);
    }
    else
    {
        resized = img;
    }

    std::vector<uchar> buf;
    std::vector<int> params = { cv::IMWRITE_JPEG_QUALITY, 85 };
    cv::imencode(".jpg", resized, buf, params);

    QByteArray b64 = QByteArray(reinterpret_cast<const char*>(buf.data()), static_cast<int>(buf.size())).toBase64();
    return "data:image/jpeg;base64," + QString::fromLatin1(b64);
}

void AiDetectionPlanner::PlanDetectionAsync(
    const QString& userRequirement,
    const cv::Mat& image,
    const QString& model,
    double temperature,
    std::function<void(const ValidatedAiPlan& plan)> callback)
{
    if (!_provider)
    {
        ValidatedAiPlan failPlan;
        failPlan.IsValid = false;
        failPlan.ErrorMessage = "AI Provider 未配置或为空";
        if (callback)
        {
            callback(failPlan);
        }
        return;
    }

    QJsonArray messages;

    // 1. 系统提示词 (包含算子知识库和 JSON 强制约束)
    QJsonObject sysMsg;
    sysMsg["role"] = "system";
    sysMsg["content"] = OperatorKnowledgeBase::BuildSystemPrompt();
    messages.append(sysMsg);

    // 2. 强化用户需求：显式指示在末尾生成 ```json { "pipeline": [...] } ``` 或检测任务
    QString promptWithInstruction = userRequirement.trimmed();
    promptWithInstruction +=
        "\n\n【强制输出要求】：请对上述视觉需求进行专业分析，并在回答末尾输出一个严格规范的 ```json ... ``` 代码块！"
        "若包含算子处理流程，请输出包含具体算子与推荐参数的 {\"pipeline\": [ ... ]}；"
        "若为质检判定，输出包含 {\"taskType\": ...} 的任务配置。以便本软件直接载入流水线或检测面板执行。";

    // 3. 用户需求消息 (支持多模态图像)
    QJsonObject userMsg;
    userMsg["role"] = "user";

    QString imageBase64 = EncodeImageToBase64(image);
    if (!imageBase64.isEmpty())
    {
        QJsonArray contentArr;

        QJsonObject textPart;
        textPart["type"] = "text";
        textPart["text"] = promptWithInstruction;
        contentArr.append(textPart);

        QJsonObject imagePart;
        imagePart["type"] = "image_url";
        QJsonObject urlObj;
        urlObj["url"] = imageBase64;
        imagePart["image_url"] = urlObj;
        contentArr.append(imagePart);

        userMsg["content"] = contentArr;
        Logger::Info("AiDetectionPlanner", "已组装包含当前图像 (Base64) 的多模态分析请求");
    }
    else
    {
        userMsg["content"] = promptWithInstruction;
    }

    messages.append(userMsg);

    Logger::Info("AiDetectionPlanner", "开始发起 AI 算法规划请求，模型: " + model.toStdString());

    _provider->ChatCompletionAsync(messages, model, temperature, 2048, [callback](bool success, const QString& reply, const QString& error) {
        if (!success)
        {
            ValidatedAiPlan failPlan;
            failPlan.IsValid = false;
            failPlan.ErrorMessage = "AI 请求失败: " + error;
            Logger::Error("AiDetectionPlanner", failPlan.ErrorMessage.toStdString());
            if (callback)
            {
                callback(failPlan);
            }
            return;
        }

        // 本地严格安全校验与解析
        ValidatedAiPlan plan = AiResponseValidator::ValidateAndParse(reply);
        if (callback)
        {
            callback(plan);
        }
    });
}
