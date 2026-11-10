#include "OpenAiCompatibleProvider.h"
#include "../utils/Logger.h"
#include <QNetworkRequest>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QTimer>
#include <QUrl>

OpenAiCompatibleProvider::OpenAiCompatibleProvider(
    const QString& baseUrl,
    const QString& apiKey,
    QObject* parent)
    : QObject(parent), _baseUrl(baseUrl), _apiKey(apiKey)
{
}

QString OpenAiCompatibleProvider::NormalizeUrl(const QString& endpoint) const
{
    QString base = _baseUrl.trimmed();
    while (base.endsWith('/'))
    {
        base.chop(1);
    }

    QString ep = endpoint.trimmed();
    if (!ep.startsWith('/'))
    {
        ep = "/" + ep;
    }

    return base + ep;
}

void OpenAiCompatibleProvider::FetchModelsAsync(
    std::function<void(bool success, const QStringList& models, const QString& error)> callback)
{
    QString urlStr = NormalizeUrl("/models");
    QUrl url(urlStr);

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    if (!_apiKey.isEmpty())
    {
        request.setRawHeader("Authorization", ("Bearer " + _apiKey.trimmed()).toUtf8());
    }

    Logger::Info("OpenAiCompatibleProvider", "开始异步获取模型列表: " + urlStr.toStdString());

    QNetworkReply* reply = _netManager.get(request);

    // 设置请求超时定时器
    QTimer* timer = new QTimer(reply);
    timer->setSingleShot(true);
    connect(timer, &QTimer::timeout, reply, [reply]() {
        if (reply->isRunning())
        {
            reply->abort();
        }
    });
    timer->start(_timeoutMs);

    connect(reply, &QNetworkReply::finished, this, [reply, timer, callback]() {
        timer->stop();
        reply->deleteLater();

        if (reply->error() != QNetworkReply::NoError)
        {
            QString err = reply->errorString();
            int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
            QByteArray errorBody = reply->readAll();
            QString fullMsg = QString("获取模型失败 (HTTP %1): %2 %3")
                                  .arg(statusCode)
                                  .arg(err)
                                  .arg(QString::fromUtf8(errorBody));
            Logger::Error("OpenAiCompatibleProvider", fullMsg.toStdString());
            if (callback)
            {
                callback(false, QStringList(), fullMsg);
            }
            return;
        }

        QByteArray data = reply->readAll();
        QJsonParseError parseErr;
        QJsonDocument doc = QJsonDocument::fromJson(data, &parseErr);
        if (parseErr.error != QJsonParseError::NoError || !doc.isObject())
        {
            QString msg = "解析模型响应 JSON 失败: " + parseErr.errorString();
            Logger::Error("OpenAiCompatibleProvider", msg.toStdString());
            if (callback)
            {
                callback(false, QStringList(), msg);
            }
            return;
        }

        QJsonObject root = doc.object();
        QStringList modelList;

        if (root.contains("data") && root["data"].isArray())
        {
            QJsonArray dataArr = root["data"].toArray();
            for (const auto& item : dataArr)
            {
                if (item.isObject())
                {
                    QString id = item.toObject()["id"].toString();
                    if (!id.isEmpty())
                    {
                        modelList.append(id);
                    }
                }
            }
        }

        Logger::Info("OpenAiCompatibleProvider", "成功自动获取到 " +
                     std::to_string(modelList.size()) + " 个在线模型");

        if (callback)
        {
            callback(true, modelList, QString());
        }
    });
}

void OpenAiCompatibleProvider::ChatCompletionAsync(
    const QJsonArray& messages,
    const QString& model,
    double temperature,
    int maxTokens,
    std::function<void(bool success, const QString& reply, const QString& error)> callback)
{
    QString urlStr = NormalizeUrl("/chat/completions");
    QUrl url(urlStr);

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    if (!_apiKey.isEmpty())
    {
        request.setRawHeader("Authorization", ("Bearer " + _apiKey.trimmed()).toUtf8());
    }

    QJsonObject reqObj;
    reqObj["model"] = model.trimmed();
    reqObj["messages"] = messages;
    reqObj["temperature"] = temperature;
    if (maxTokens > 0)
    {
        reqObj["max_tokens"] = maxTokens;
    }

    QJsonDocument doc(reqObj);
    QByteArray postData = doc.toJson(QJsonDocument::Compact);

    Logger::Info("OpenAiCompatibleProvider", "发送 Chat 请求至: " + urlStr.toStdString() +
                 ", 模型: " + model.toStdString());

    QNetworkReply* reply = _netManager.post(request, postData);

    QTimer* timer = new QTimer(reply);
    timer->setSingleShot(true);
    connect(timer, &QTimer::timeout, reply, [reply]() {
        if (reply->isRunning())
        {
            reply->abort();
        }
    });
    timer->start(_timeoutMs);

    connect(reply, &QNetworkReply::finished, this, [reply, timer, callback]() {
        timer->stop();
        reply->deleteLater();

        if (reply->error() != QNetworkReply::NoError)
        {
            QString err = reply->errorString();
            int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
            QByteArray errorBody = reply->readAll();
            QString fullMsg = QString("AI 服务请求失败 (HTTP %1): %2 %3")
                                  .arg(statusCode)
                                  .arg(err)
                                  .arg(QString::fromUtf8(errorBody));
            Logger::Error("OpenAiCompatibleProvider", fullMsg.toStdString());
            if (callback)
            {
                callback(false, QString(), fullMsg);
            }
            return;
        }

        QByteArray data = reply->readAll();
        QJsonParseError parseErr;
        QJsonDocument resDoc = QJsonDocument::fromJson(data, &parseErr);
        if (parseErr.error != QJsonParseError::NoError || !resDoc.isObject())
        {
            QString msg = "解析 AI 返回 JSON 失败: " + parseErr.errorString();
            Logger::Error("OpenAiCompatibleProvider", msg.toStdString());
            if (callback)
            {
                callback(false, QString(), msg);
            }
            return;
        }

        QJsonObject root = resDoc.object();
        QString replyContent;

        if (root.contains("choices") && root["choices"].isArray())
        {
            QJsonArray choices = root["choices"].toArray();
            if (!choices.isEmpty() && choices[0].isObject())
            {
                QJsonObject choiceObj = choices[0].toObject();
                if (choiceObj.contains("message") && choiceObj["message"].isObject())
                {
                    replyContent = choiceObj["message"].toObject()["content"].toString();
                }
            }
        }

        Logger::Info("OpenAiCompatibleProvider", "收到 AI 响应，字符数: " +
                     std::to_string(replyContent.length()));

        if (callback)
        {
            callback(true, replyContent, QString());
        }
    });
}
