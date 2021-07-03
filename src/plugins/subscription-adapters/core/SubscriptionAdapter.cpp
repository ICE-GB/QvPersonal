#include "SubscriptionAdapter.hpp"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QUrl>
#include <QUrlQuery>

const inline QStringList SplitLines(const QString &_string)
{
    return _string.split(QRegularExpression("[\r\n]"), Qt::SkipEmptyParts);
}

QString SafeBase64Decode(QString string)
{
    QByteArray ba = string.replace(QChar('-'), QChar('+')).replace(QChar('_'), QChar('/')).toUtf8();
    return QByteArray::fromBase64(ba, QByteArray::Base64Option::OmitTrailingEquals);
}

QString SafeBase64Encode(const QString &string)
{
    QString base64 = string.toUtf8().toBase64();
    return base64.replace(QChar('+'), QChar('-')).replace(QChar('/'), QChar('_'));
}

// Simple Base64 Decoder
SubscriptionDecodeResult SimpleBase64Decoder::DecodeData(const QByteArray &data) const
{
    auto source = QString::fromUtf8(data).trimmed();
    const auto resultList = source.contains("://") ? source : SafeBase64Decode(source);
    //
    SubscriptionDecodeResult result;
    result.links = SplitLines(resultList);
    return result;
}

// OOCv1 Decoder
SubscriptionDecodeResult OOCv1Decoder::DecodeData(const QByteArray &data) const
{
    const auto root = QJsonDocument::fromJson(data).object();
    //
    const auto version = root["version"].toInt();
    const auto username = root["username"].toString();
    const auto user_uuid = root["user_uuid"].toString();
    const auto servers = root["servers"].toArray();

    if (version == 1)
    {
        // ss://Y2hhY2hhMjAtaWV0Zi1wb2x5MTMwNTpwYXNzQGhvc3Q6MTIzNA/?plugin=plugin%3Bopt#sssip003
        SubscriptionDecodeResult result;
        for (const auto &servVal : servers)
        {
            const auto serverObj = servVal.toObject();
#define GetVal(x) const auto x = serverObj[#x].toString()
            GetVal(server);
            GetVal(password);
            GetVal(method);
            GetVal(plugin);
            GetVal(plugin_opts);
            GetVal(remarks);
            GetVal(id);
#undef GetVal
            const auto server_port = serverObj["server_port"].toInt();
            bool isSIP003 = !plugin.isEmpty();
            const auto userInfo = SafeBase64Encode(method + ":" + password);
#pragma message("TODO OOCv1")
            // ...
        }
        return result;
    }
    return {};
}
