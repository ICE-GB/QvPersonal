#include "OutboundHandler.hpp"

#include "QvPlugin/Utils/QJsonIO.hpp"
#include "V2RayModels.hpp"

#include <QUrl>
#include <QUrlQuery>

QString SafeBase64Decode(QString string)
{
    QByteArray ba = string.replace(QChar('-'), QChar('+')).replace(QChar('_'), QChar('/')).toUtf8();
    return QByteArray::fromBase64(ba, QByteArray::Base64Option::OmitTrailingEquals);
}

using namespace Qv2rayPlugin;
using namespace Qv2ray::Models;

QString SerializeVLESS(const QString &name, const IOConnectionSettings &connection);
QString SerializeVMess(const QString &name, const IOConnectionSettings &connection);
QString SerializeSS(const QString &name, const IOConnectionSettings &connection);

std::optional<std::pair<QString, IOConnectionSettings>> DeserializeVLESS(const QString &link);
std::optional<std::pair<QString, IOConnectionSettings>> DeserializeVMess(const QString &link);
std::optional<std::pair<QString, IOConnectionSettings>> DeserializeSS(const QString &link);

std::optional<QString> BuiltinSerializer::Serialize(const QString &name, const IOConnectionSettings &outbound) const
{
    const auto protocol = outbound.protocol;
    const auto out = outbound.protocolSettings;
    const auto stream = outbound.streamSettings;

    if (protocol == QStringLiteral("http") || protocol == QStringLiteral("socks"))
    {
        QUrl url;
        url.setScheme(protocol);
        url.setHost(outbound.address);
        url.setPort(outbound.port.from);
        if (out.contains(QStringLiteral("user")))
            url.setUserName(out[QStringLiteral("user")].toString());
        if (out.contains(QStringLiteral("pass")))
            url.setUserName(out[QStringLiteral("pass")].toString());
        return url.toString();
    }
    if (protocol == QStringLiteral("vless"))
    {
        return SerializeVLESS(name, outbound);
    }
    if (protocol == QStringLiteral("vmess"))
    {
        return SerializeVMess(name, outbound);
    }
    if (protocol == QStringLiteral("shadowsocks"))
    {
        return SerializeSS(name, outbound);
    }

    return std::nullopt;
}

std::optional<std::pair<QString, IOConnectionSettings>> BuiltinSerializer::Deserialize(const QString &link) const
{
    if (link.startsWith(QStringLiteral("http://")) || link.startsWith(QStringLiteral("socks://")))
    {
        const QUrl url{ link };
        IOConnectionSettings out;
        out.protocol = url.scheme();
        out.address = url.host();
        out.port = url.port();
        out.protocolSettings["user"] = url.userName();
        out.protocolSettings["pass"] = url.password();
        return std::make_pair(url.fragment(), out);
    }

    if (link.startsWith(QStringLiteral("ss://")))
        return DeserializeSS(link);

    if (link.startsWith(QStringLiteral("vmess://")))
        return DeserializeVMess(link);

    if (link.startsWith(QStringLiteral("vless://")))
        return DeserializeVLESS(link);

    return std::nullopt;
}

QString SerializeVLESS(const QString &name, const IOConnectionSettings &conn)
{
    const auto out = conn.protocolSettings;
    const auto stream = conn.streamSettings;
    QUrl url;
    url.setFragment(QUrl::toPercentEncoding(name));
    url.setScheme(QStringLiteral("vless"));
    url.setHost(conn.address);
    url.setPort(conn.port.from);
    url.setUserName(out[QStringLiteral("id")].toString());

    // -------- COMMON INFORMATION --------
    QUrlQuery query;
    const auto enc = out[QStringLiteral("encryption")].toString(QStringLiteral("none"));
    if (enc != QStringLiteral("none"))
        query.addQueryItem(QStringLiteral("encryption"), enc);

    const auto network = QJsonIO::GetValue(stream, "network").toString(QStringLiteral("tcp"));
    if (network != QStringLiteral("tcp"))
        query.addQueryItem(QStringLiteral("type"), network);

    const auto security = QJsonIO::GetValue(stream, "security").toString(QStringLiteral("none"));
    if (security != QStringLiteral("none"))
        query.addQueryItem(QStringLiteral("security"), security);

    // -------- TRANSPORT RELATED --------
    if (network == QStringLiteral("kcp"))
    {
        const auto seed = QJsonIO::GetValue(stream, { "kcpSettings", "seed" }).toString();
        if (!seed.isEmpty())
            query.addQueryItem(QStringLiteral("seed"), QUrl::toPercentEncoding(seed));

        const auto headerType = QJsonIO::GetValue(stream, { "kcpSettings", "header", "type" }).toString(QStringLiteral("none"));
        if (headerType != QStringLiteral("none"))
            query.addQueryItem(QStringLiteral("headerType"), headerType);
    }
    else if (network == QStringLiteral("http"))
    {
        const auto path = QJsonIO::GetValue(stream, { "HTTPConfig", "path" }).toString(QStringLiteral("/"));
        query.addQueryItem(QStringLiteral("path"), QUrl::toPercentEncoding(path));

        const auto hosts = QJsonIO::GetValue(stream, { "HTTPConfig", "host" }).toArray();
        QStringList hostList;
        for (const auto item : hosts)
        {
            const auto host = item.toString();
            if (!host.isEmpty())
                hostList << host;
        }
        query.addQueryItem(QStringLiteral("host"), QUrl::toPercentEncoding(hostList.join(QStringLiteral(","))));
    }
    else if (network == QStringLiteral("ws"))
    {
        const auto path = QJsonIO::GetValue(stream, { "wsSettings", "path" }).toString(QStringLiteral("/"));
        query.addQueryItem(QStringLiteral("path"), QUrl::toPercentEncoding(path));

        const auto host = QJsonIO::GetValue(stream, { "wsSettings", "headers", "Host" }).toString();
        query.addQueryItem(QStringLiteral("host"), host);
    }
    else if (network == QStringLiteral("quic"))
    {
        const auto quicSecurity = QJsonIO::GetValue(stream, { "quicSettings", "security" }).toString(QStringLiteral("none"));
        if (quicSecurity != QStringLiteral("none"))
        {
            query.addQueryItem(QStringLiteral("quicSecurity"), quicSecurity);

            const auto key = QJsonIO::GetValue(stream, { "quicSettings", "key" }).toString();
            query.addQueryItem(QStringLiteral("key"), QUrl::toPercentEncoding(key));

            const auto headerType = QJsonIO::GetValue(stream, { "quicSettings", "header", "type" }).toString(QStringLiteral("none"));
            if (headerType != QStringLiteral("none"))
                query.addQueryItem(QStringLiteral("headerType"), headerType);
        }
    }
    else if (network == QStringLiteral("grpc"))
    {
        const auto serviceName = QJsonIO::GetValue(stream, { "grpcSettings", "serviceName" }).toString(QStringLiteral("GunService"));
        if (serviceName != QStringLiteral("GunService"))
            query.addQueryItem(QStringLiteral("serviceName"), QUrl::toPercentEncoding(serviceName));
    }
    // -------- TLS RELATED --------
    const auto tlsKey = security == QStringLiteral("xtls") ? "xtlsSettings" : "tlsSettings";

    const auto sni = QJsonIO::GetValue(stream, { tlsKey, "serverName" }).toString();
    if (!sni.isEmpty())
        query.addQueryItem(QStringLiteral("sni"), sni);

    // ALPN
    const auto alpnArray = QJsonIO::GetValue(stream, { tlsKey, QStringLiteral("alpn") }).toArray();
    QStringList alpnList;
    for (const auto v : alpnArray)
    {
        const auto alpn = v.toString();
        if (!alpn.isEmpty())
            alpnList << alpn;
    }
    query.addQueryItem(QStringLiteral("alpn"), QUrl::toPercentEncoding(alpnList.join(QStringLiteral(","))));

    // -------- XTLS Flow --------
    if (security == QStringLiteral("xtls"))
    {
        const auto flow = out[QStringLiteral("flow")].toString();
        query.addQueryItem(QStringLiteral("flow"), flow);
    }

    // ======== END OF QUERY ========
    url.setQuery(query);
    return url.toString(QUrl::FullyEncoded);
}

QString SerializeVMess(const QString &alias, const IOConnectionSettings &connection)
{
    Qv2ray::Models::VMessClientObject server;
    server.loadJson(connection.protocolSettings);

    Qv2ray::Models::StreamSettingsObject stream;
    stream.loadJson(connection.streamSettings);

    QUrl url;
    QUrlQuery query;
    url.setFragment(alias, QUrl::StrictMode);

    if (stream.network == QStringLiteral("tcp"))
    {
        if (!stream.tcpSettings->header->type->isEmpty() && stream.tcpSettings->header->type != QStringLiteral("none"))
            query.addQueryItem(QStringLiteral("type"), stream.tcpSettings->header->type);
    }
    else if (stream.network == QStringLiteral("http"))
    {
        if (!stream.httpSettings->host->isEmpty())
            query.addQueryItem(QStringLiteral("host"), stream.httpSettings->host->first());
        query.addQueryItem(QStringLiteral("path"), stream.httpSettings->path->isEmpty() ? QStringLiteral("/") : *stream.httpSettings->path);
    }
    else if (stream.network == QStringLiteral("ws"))
    {
        if (stream.wsSettings->headers->contains(QStringLiteral("Host")) && !stream.wsSettings->headers->value(QStringLiteral("Host")).isEmpty())
            query.addQueryItem(QStringLiteral("host"), stream.wsSettings->headers->value(QStringLiteral("Host")));
        if (!stream.wsSettings->path->isEmpty() && stream.wsSettings->path != QStringLiteral("/"))
            query.addQueryItem(QStringLiteral("path"), stream.wsSettings->path);
    }
    else if (stream.network == QStringLiteral("kcp"))
    {
        if (!stream.kcpSettings->seed->isEmpty())
            query.addQueryItem(QStringLiteral("seed"), stream.kcpSettings->seed);
        if (!stream.kcpSettings->header->type->isEmpty() && stream.kcpSettings->header->type != QStringLiteral("none"))
            query.addQueryItem(QStringLiteral("type"), stream.kcpSettings->header->type);
    }
    else if (stream.network == QStringLiteral("quic"))
    {
        if (!stream.quicSettings->security->isEmpty() && stream.quicSettings->security != QStringLiteral("none"))
            query.addQueryItem(QStringLiteral("security"), stream.quicSettings->security);
        if (!stream.quicSettings->key->isEmpty())
            query.addQueryItem(QStringLiteral("key"), stream.quicSettings->key);
        if (!stream.quicSettings->header->type->isEmpty() && stream.quicSettings->header->type != QStringLiteral("none"))
            query.addQueryItem(QStringLiteral("headers"), stream.quicSettings->header->type);
    }
    else
    {
        return {};
    }
    bool hasTLS = stream.security == QStringLiteral("tls");
    auto protocol = *stream.network;
    if (hasTLS)
    {
        // if (stream.tlsSettings.allowInsecure)
        //    query.addQueryItem("allowInsecure", "true");
        if (!stream.tlsSettings->serverName->isEmpty())
            query.addQueryItem(QStringLiteral("tlsServerName"), stream.tlsSettings->serverName);
        protocol += QStringLiteral("+tls");
    }
    url.setPath(QStringLiteral("/"));
    url.setScheme(QStringLiteral("vmess"));
    url.setPassword(server.id + "-" + QString::number(server.alterId));
    url.setHost(connection.address);
    url.setPort(connection.port.from);
    url.setUserName(protocol);
    url.setQuery(query);
    return url.toString();
}

QString SerializeSS(const QString &name, const IOConnectionSettings &connection)
{
    Qv2ray::Models::ShadowSocksClientObject server;
    server.loadJson(connection.protocolSettings);
    QUrl url;
    const auto plainUserInfo = server.method + ":" + server.password;
    const auto userinfo = plainUserInfo.toUtf8().toBase64(QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals);
    url.setUserInfo(userinfo);
    url.setScheme(QStringLiteral("ss"));
    url.setHost(connection.address);
    url.setPort(connection.port.from);
    url.setFragment(name);
    return url.toString(QUrl::ComponentFormattingOption::FullyEncoded);
}

const static QStringList NetworkType{ "tcp", "http", "ws", "kcp", "quic" };
const static QStringList QuicSecurityTypes{ "none", "aes-128-gcm", "chacha20-poly1305" };
const static QStringList QuicKcpHeaderTypes{ "none", "srtp", "utp", "wechat-video", "dtls", "wireguard" };
const static QStringList FalseTypes{ "false", "False", "No", "Off", "0" };

std::optional<std::pair<QString, IOConnectionSettings>> DeserializeVLESS(const QString &link)
{
    // parse url
    QUrl url(link);
    if (!url.isValid())
        return std::nullopt;

    // fetch host
    const auto hostRaw = url.host();
    if (hostRaw.isEmpty())
        return std::nullopt;

    const auto host = (hostRaw.startsWith('[') && hostRaw.endsWith(']')) ? hostRaw.mid(1, hostRaw.length() - 2) : hostRaw;

    // fetch port
    const auto port = url.port();
    if (port == -1)
        return std::nullopt;

    // fetch remarks
    const auto remarks = url.fragment();

    // fetch uuid
    const auto uuid = url.userInfo();
    if (uuid.isEmpty())
        return std::nullopt;

    // initialize QJsonObject with basic info
    QJsonObject outbound;
    QJsonObject stream;

    outbound[QStringLiteral("id")] = uuid;

    // parse query
    QUrlQuery query(url.query());

    // handle type
    const auto hasType = query.hasQueryItem(QStringLiteral("type"));
    const auto type = hasType ? query.queryItemValue(QStringLiteral("type")) : QStringLiteral("tcp");
    if (type != QStringLiteral("tcp"))
        QJsonIO::SetValue(stream, type, QStringLiteral("network"));

    // handle encryption
    const auto hasEncryption = query.hasQueryItem(QStringLiteral("encryption"));
    const auto encryption = hasEncryption ? query.queryItemValue(QStringLiteral("encryption")) : QStringLiteral("none");
    outbound[QStringLiteral("encryption")] = encryption;

    // type-wise settings
    if (type == QStringLiteral("kcp"))
    {
        const auto hasSeed = query.hasQueryItem(QStringLiteral("seed"));
        if (hasSeed)
            QJsonIO::SetValue(stream, query.queryItemValue(QStringLiteral("seed")), { QStringLiteral("kcpSettings"), QStringLiteral("seed") });

        const auto hasHeaderType = query.hasQueryItem(QStringLiteral("headerType"));
        const auto headerType = hasHeaderType ? query.queryItemValue(QStringLiteral("headerType")) : QStringLiteral("none");
        if (headerType != QStringLiteral("none"))
            QJsonIO::SetValue(stream, headerType, { QStringLiteral("kcpSettings"), QStringLiteral("header"), QStringLiteral("type") });
    }
    else if (type == QStringLiteral("http"))
    {
        const auto hasPath = query.hasQueryItem(QStringLiteral("path"));
        const auto path = hasPath ? QUrl::fromPercentEncoding(query.queryItemValue(QStringLiteral("path")).toUtf8()) : QStringLiteral("/");
        if (path != QStringLiteral("/"))
            QJsonIO::SetValue(stream, path, { QStringLiteral("httpSettings"), QStringLiteral("path") });

        const auto hasHost = query.hasQueryItem(QStringLiteral("host"));
        if (hasHost)
        {
            const auto hosts = QJsonArray::fromStringList(query.queryItemValue(QStringLiteral("host")).split(','));
            QJsonIO::SetValue(stream, hosts, { QStringLiteral("httpSettings"), QStringLiteral("host") });
        }
    }
    else if (type == QStringLiteral("ws"))
    {
        const auto hasPath = query.hasQueryItem(QStringLiteral("path"));
        const auto path = hasPath ? QUrl::fromPercentEncoding(query.queryItemValue(QStringLiteral("path")).toUtf8()) : QStringLiteral("/");
        if (path != QStringLiteral("/"))
            QJsonIO::SetValue(stream, path, { QStringLiteral("wsSettings"), QStringLiteral("path") });

        const auto hasHost = query.hasQueryItem(QStringLiteral("host"));
        if (hasHost)
        {
            QJsonIO::SetValue(stream, query.queryItemValue(QStringLiteral("host")), { QStringLiteral("wsSettings"), QStringLiteral("headers"), QStringLiteral("Host") });
        }
    }
    else if (type == QStringLiteral("quic"))
    {
        const auto hasQuicSecurity = query.hasQueryItem(QStringLiteral("quicSecurity"));
        if (hasQuicSecurity)
        {
            const auto quicSecurity = query.queryItemValue(QStringLiteral("quicSecurity"));
            QJsonIO::SetValue(stream, quicSecurity, { QStringLiteral("quicSettings"), QStringLiteral("security") });

            if (quicSecurity != QStringLiteral("none"))
            {
                const auto key = query.queryItemValue(QStringLiteral("key"));
                QJsonIO::SetValue(stream, key, { QStringLiteral("quicSettings"), QStringLiteral("key") });
            }

            const auto hasHeaderType = query.hasQueryItem(QStringLiteral("headerType"));
            const auto headerType = hasHeaderType ? query.queryItemValue(QStringLiteral("headerType")) : QStringLiteral("none");
            if (headerType != QStringLiteral("none"))
                QJsonIO::SetValue(stream, headerType, { QStringLiteral("quicSettings"), QStringLiteral("header"), QStringLiteral("type") });
        }
    }
    else if (type == QStringLiteral("grpc"))
    {
        const auto hasServiceName = query.hasQueryItem(QStringLiteral("serviceName"));
        if (hasServiceName)
        {
            const auto serviceName = QUrl::fromPercentEncoding(query.queryItemValue(QStringLiteral("serviceName")).toUtf8());
            if (serviceName != QStringLiteral("GunService"))
            {
                QJsonIO::SetValue(stream, serviceName, { QStringLiteral("grpcSettings"), QStringLiteral("serviceName") });
            }
        }
    }

    // tls-wise settings
    const auto hasSecurity = query.hasQueryItem(QStringLiteral("security"));
    const auto security = hasSecurity ? query.queryItemValue(QStringLiteral("security")) : QStringLiteral("none");
    const auto tlsKey = security == QStringLiteral("xtls") ? QStringLiteral("xtlsSettings") : QStringLiteral("tlsSettings");
    if (security != QStringLiteral("none"))
    {
        QJsonIO::SetValue(stream, security, QStringLiteral("security"));
    }
    // sni
    const auto hasSNI = query.hasQueryItem(QStringLiteral("sni"));
    if (hasSNI)
    {
        const auto sni = query.queryItemValue(QStringLiteral("sni"));
        QJsonIO::SetValue(stream, sni, { tlsKey, QStringLiteral("serverName") });
    }
    // alpn
    const auto hasALPN = query.hasQueryItem(QStringLiteral("alpn"));
    if (hasALPN)
    {
        const auto alpnRaw = QUrl::fromPercentEncoding(query.queryItemValue(QStringLiteral("alpn")).toUtf8());
        const auto alpnArray = QJsonArray::fromStringList(alpnRaw.split(','));
        QJsonIO::SetValue(stream, alpnArray, { tlsKey, QStringLiteral("alpn") });
    }

    IOConnectionSettings conn;
    conn.protocol = QStringLiteral("vless");
    conn.address = host;
    conn.port = port;
    conn.protocolSettings = IOProtocolSettings{ outbound };
    conn.streamSettings = IOStreamSettings{ stream };
    return std::make_pair(remarks, conn);
}

std::optional<std::pair<QString, IOConnectionSettings>> DeserializeVMess(const QString &link)
{
    IOConnectionSettings conn;
    conn.protocol = QStringLiteral("vmess");
    QUrl url{ link };
    QUrlQuery query{ url };
    if (!url.isValid())
        return std::nullopt;

    // If previous alias is empty, just the PS is needed, else, append a "_"
    const auto name = url.fragment(QUrl::FullyDecoded).trimmed();

    VMessClientObject client;

    StreamSettingsObject stream;
    QString net;
    bool tls = false;
    // Check streamSettings
    {
        for (const auto &_protocol : url.userName().split('+'))
        {
            if (_protocol == QStringLiteral("tls"))
                tls = true;
            else
                net = _protocol;
        }
        if (!NetworkType.contains(net))
        {
            return {};
        }
        // L("net: " << net.toStdString());
        // L("tls: " << tls);
        stream.network = net;
        stream.security = tls ? QStringLiteral("tls") : QStringLiteral("");
    }
    // Host Port UUID AlterID
    {
        const auto host = url.host();
        int port = url.port();
        QString uuid;
        int aid;
        {
            const auto pswd = url.password();
            const auto index = pswd.lastIndexOf('-');
            uuid = pswd.mid(0, index);
            aid = pswd.right(pswd.length() - index - 1).toInt();
        }

        conn.address = host;
        conn.port = port;
        client.id = uuid;
        client.alterId = aid;
        client.security = QStringLiteral("auto");
    }

    const static auto getQueryValue = [&query](const QString &key, const QString &defaultValue) {
        if (query.hasQueryItem(key))
            return query.queryItemValue(key, QUrl::FullyDecoded);
        else
            return defaultValue;
    };

    //
    // Begin transport settings parser
    {
        if (net == QStringLiteral("tcp"))
        {
            stream.tcpSettings->header->type = getQueryValue(QStringLiteral("type"), QStringLiteral("none"));
        }
        else if (net == QStringLiteral("http"))
        {
            stream.httpSettings->host->append(getQueryValue(QStringLiteral("host"), QStringLiteral("")));
            stream.httpSettings->path = getQueryValue(QStringLiteral("path"), QStringLiteral("/"));
        }
        else if (net == QStringLiteral("ws"))
        {
            stream.wsSettings->headers->insert(QStringLiteral("Host"), getQueryValue(QStringLiteral("host"), QStringLiteral("")));
            stream.wsSettings->path = getQueryValue(QStringLiteral("path"), QStringLiteral("/"));
        }
        else if (net == QStringLiteral("kcp"))
        {
            stream.kcpSettings->seed = getQueryValue(QStringLiteral("seed"), QStringLiteral(""));
            stream.kcpSettings->header->type = getQueryValue(QStringLiteral("type"), QStringLiteral("none"));
        }
        else if (net == QStringLiteral("quic"))
        {
            stream.quicSettings->security = getQueryValue(QStringLiteral("security"), QStringLiteral("none"));
            stream.quicSettings->key = getQueryValue(QStringLiteral("key"), QStringLiteral(""));
            stream.quicSettings->header->type = getQueryValue(QStringLiteral("type"), QStringLiteral("none"));
        }
    }

    if (tls)
    {
        // stream.tlsSettings.allowInsecure = !FalseTypes.contains(getQueryValue("allowInsecure", "false"));
        stream.tlsSettings->serverName = getQueryValue(QStringLiteral("tlsServerName"), QStringLiteral(""));
    }

    conn.protocolSettings = IOProtocolSettings{ client.toJson() };
    conn.streamSettings = IOStreamSettings{ stream.toJson() };
    return std::make_pair(name, conn);
}

std::optional<std::pair<QString, IOConnectionSettings>> DeserializeSS(const QString &link)
{
    IOConnectionSettings conn;
    ShadowSocksClientObject server;
    QString d_name;

    auto uri = link.mid(5);
    auto hashPos = uri.lastIndexOf('#');
    //    DEBUG("Hash sign position: " + QSTRN(hashPos));

    if (hashPos >= 0)
    {
        // Get the name/remark
        d_name = uri.mid(uri.lastIndexOf('#') + 1);
        uri.truncate(hashPos);
    }

    auto atPos = uri.indexOf('@');
    //    DEBUG("At sign position: " + QSTRN(atPos));

    if (atPos < 0)
    {
        // Old URI scheme
        QString decoded = QByteArray::fromBase64(uri.toUtf8(), QByteArray::Base64Option::OmitTrailingEquals);
        auto colonPos = decoded.indexOf(':');

        if (colonPos < 0)
        {
            return std::nullopt;
            //            *errMessage = QObject::tr("Can't find the colon separator between method and password");
        }

        server.method = decoded.left(colonPos);
        decoded.remove(0, colonPos + 1);
        atPos = decoded.lastIndexOf('@');
        //        DEBUG("At sign position: " + QSTRN(atPos));

        if (atPos < 0)
        {
            return std::nullopt;
            //            *errMessage = QObject::tr("Can't find the at separator between password and hostname");
        }

        server.password = decoded.mid(0, atPos);
        decoded.remove(0, atPos + 1);
        colonPos = decoded.lastIndexOf(':');

        if (colonPos < 0)
        {
            return std::nullopt;
            //            *errMessage = QObject::tr("Can't find the colon separator between hostname and port");
        }

        conn.address = decoded.mid(0, colonPos);
        conn.port = decoded.mid(colonPos + 1).toInt();
    }
    else
    {
        // SIP002 URI scheme
        auto x = QUrl::fromUserInput(uri);
        conn.address = x.host();
        conn.port = x.port();
        const auto userInfo = SafeBase64Decode(x.userName());
        const auto userInfoSp = userInfo.indexOf(':');
        //
        //        DEBUG("Userinfo splitter position: " + QSTRN(userInfoSp));

        if (userInfoSp < 0)
        {
            return std::nullopt;
        }

        const auto method = userInfo.mid(0, userInfoSp);
        server.method = method;
        server.password = userInfo.mid(userInfoSp + 1);
    }

    d_name = QUrl::fromPercentEncoding(d_name.toUtf8());
    conn.protocol = QStringLiteral("shadowsocks");
    conn.protocolSettings = IOProtocolSettings{ server.toJson() };
    return std::make_pair(d_name, conn);
}
