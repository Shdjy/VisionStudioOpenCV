#include "OpenAiCompatibleProvider.h"
#include "../utils/Logger.h"
#include <QNetworkRequest>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QTimer>
#include <QUrl>
#include <QElapsedTimer>

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

void OpenAiCompatibleProvider::TestConnectivityAsync(
    std::function<void(bool success, qint64 latencyMs, const QStringList& models, const QString& error)> callback)
{
    QString urlStr = NormalizeUrl("/models");
    QUrl url(urlStr);

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    if (!_apiKey.isEmpty())
    {
        request.setRawHeader("Authorization", ("Bearer " + _apiKey.trimmed()).toUtf8());
    }

    QElapsedTimer* timerWatch = new QElapsedTimer();
    timerWatch->start();

    QNetworkReply* reply = _netManager.get(request);

    QTimer* timer = new QTimer(reply);
    timer->setSingleShot(true);
    connect(timer, &QTimer::timeout, reply, [reply]() {
        if (reply->isRunning())
        {
            reply->abort();
        }
    });
    timer->start(_timeoutMs);

    connect(reply, &QNetworkReply::finished, this, [reply, timer, timerWatch, callback]() {
        qint64 latencyMs = timerWatch->elapsed();
        delete timerWatch;
        timer->stop();
        reply->deleteLater();

        if (reply->error() != QNetworkReply::NoError)
        {
            QString err = reply->errorString();
            int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
            QByteArray errorBody = reply->readAll();
            QString fullMsg = QString("连接失败 (HTTP %1): %2 %3")
                                  .arg(statusCode)
                                  .arg(err)
                                  .arg(QString::fromUtf8(errorBody).trimmed());
            if (callback)
            {
                callback(false, latencyMs, QStringList(), fullMsg);
            }
            return;
        }

        QByteArray data = reply->readAll();
        QJsonParseError parseErr;
        QJsonDocument doc = QJsonDocument::fromJson(data, &parseErr);
        if (parseErr.error != QJsonParseError::NoError || !doc.isObject())
        {
            QString msg = "解析响应 JSON 失败: " + parseErr.errorString();
            if (callback)
            {
                callback(false, latencyMs, QStringList(), msg);
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
        else if (root.contains("models") && root["models"].isArray())
        {
            QJsonArray dataArr = root["models"].toArray();
            for (const auto& item : dataArr)
            {
                if (item.isObject())
                {
                    QString name = item.toObject()["name"].toString();
                    if (!name.isEmpty())
                    {
                        modelList.append(name);
                    }
                }
            }
        }

        if (callback)
        {
            callback(true, latencyMs, modelList, QString());
        }
    });
}

void OpenAiCompatibleProvider::FetchModelsAsync(
    std::function<void(bool success, const QStringList& models, const QString& error)> callback)
{
    TestConnectivityAsync([callback](bool success, qint64 /*latencyMs*/, const QStringList& models, const QString& error) {
        if (callback)
        {
            callback(success, models, error);
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

    QJsonObject body;
    body["model"] = model;
    body["messages"] = messages;
    body["temperature"] = temperature;
    body["max_tokens"] = maxTokens;
    body["stream"] = false;

    QJsonDocument doc(body);
    QByteArray postData = doc.toJson(QJsonDocument::Compact);

    Logger::Info("OpenAiCompatibleProvider", "发起 ChatCompletion: " + urlStr.toStdString() +
                 " | 模型: " + model.toStdString());

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
        QString reasoningContent;

        if (root.contains("choices") && root["choices"].isArray())
        {
            QJsonArray choices = root["choices"].toArray();
            if (!choices.isEmpty() && choices[0].isObject())
            {
                QJsonObject choiceObj = choices[0].toObject();
                if (choiceObj.contains("message") && choiceObj["message"].isObject())
                {
                    QJsonObject msgObj = choiceObj["message"].toObject();
                    replyContent = msgObj["content"].toString();
                    if (msgObj.contains("reasoning_content"))
                    {
                        reasoningContent = msgObj["reasoning_content"].toString().trimmed();
                    }
                }
            }
        }

        // 如果包含单独的 reasoning_content 且正文中未自带 <think> 标签，将其整合成标准 <think> 格式
        if (!reasoningContent.isEmpty() && !replyContent.contains("<think>"))
        {
            replyContent = QString("<think>\n%1\n</think>\n\n%2").arg(reasoningContent, replyContent);
        }

        Logger::Info("OpenAiCompatibleProvider", "收到 AI 响应，字符数: " +
                     std::to_string(replyContent.length()) +
                     (reasoningContent.isEmpty() ? "" : " (包含推理链)"));

        if (callback)
        {
            callback(true, replyContent, QString());
        }
    });
}
