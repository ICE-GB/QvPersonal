#include "OutboundHandler.hpp"

#include "Utils/QJsonIO.hpp"
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

QString SerializeVLESS(const QString &name, const IOProtocolSettings &settings, const IOStreamSettings &stream);
QString SerializeVMess(const QString &name, const IOProtocolSettings &settings, const IOStreamSettings &stream);
QString SerializeSS(const QString &name, const IOProtocolSettings &settings, const IOStreamSettings &stream);

std::optional<std::tuple<IOProtocolSettings, IOStreamSettings, QString>> DeserializeVLESS(const QString &link);
std::optional<std::tuple<IOProtocolSettings, IOStreamSettings, QString>> DeserializeVMess(const QString &link);
std::optional<std::tuple<IOProtocolSettings, IOStreamSettings, QString>> DeserializeSS(const QString &link);

std::optional<PluginIOBoundData> BuiltinSerializer::GetOutboundInfo(const IOConnectionSettings &outbound) const
{
    PluginIOBoundData obj;
    obj[IOBOUND_DATA_TYPE::IO_PROTOCOL] = outbound.protocol;
    if (outbound.protocol == QStringLiteral("http"))
    {
        HttpClientObject http;
        http.loadJson(outbound.protocolSettings[QStringLiteral("servers")].toArray().first());
        obj[IOBOUND_DATA_TYPE::IO_ADDRESS] = *http.address;
        obj[IOBOUND_DATA_TYPE::IO_PORT] = *http.port;
        return obj;
    }
    else if (outbound.protocol == QStringLiteral("socks"))
    {
        SocksClientObject socks;
        socks.loadJson(outbound.protocolSettings[QStringLiteral("servers")].toArray().first());
        obj[IOBOUND_DATA_TYPE::IO_ADDRESS] = *socks.address;
        obj[IOBOUND_DATA_TYPE::IO_PORT] = *socks.port;
        return obj;
    }
    else if (outbound.protocol == QStringLiteral("vmess"))
    {
        VMessClientObject vmess;
        vmess.loadJson(outbound.protocolSettings[QStringLiteral("vnext")].toArray().first());
        obj[IOBOUND_DATA_TYPE::IO_ADDRESS] = *vmess.address;
        obj[IOBOUND_DATA_TYPE::IO_PORT] = *vmess.port;
        return obj;
    }
    else if (outbound.protocol == QStringLiteral("vless"))
    {
        VLESSClientObject vless;
        vless.loadJson(outbound.protocolSettings[QStringLiteral("vnext")].toArray().first());
        obj[IOBOUND_DATA_TYPE::IO_ADDRESS] = *vless.address;
        obj[IOBOUND_DATA_TYPE::IO_PORT] = *vless.port;
        return obj;
    }
    else if (outbound.protocol == QStringLiteral("shadowsocks"))
    {
        ShadowSocksClientObject ss;
        ss.loadJson(outbound.protocolSettings[QStringLiteral("servers")].toArray().first());
        obj[IOBOUND_DATA_TYPE::IO_ADDRESS] = *ss.address;
        obj[IOBOUND_DATA_TYPE::IO_PORT] = *ss.port;
        return obj;
    }
    return std::nullopt;
}

bool BuiltinSerializer::SetOutboundInfo(IOConnectionSettings &outbound, const PluginIOBoundData &info) const
{
    if ((QStringList{ "http", "socks", "shadowsocks" }).contains(outbound.protocol))
    {
        QJsonIO::SetValue(outbound.protocolSettings, info[IOBOUND_DATA_TYPE::IO_ADDRESS].toString(), "servers", 0, "address");
        QJsonIO::SetValue(outbound.protocolSettings, info[IOBOUND_DATA_TYPE::IO_PORT].toInt(), "servers", 0, "port");
        return true;
    }

    if ((QStringList{ "vless", "vmess" }).contains(outbound.protocol))
    {
        QJsonIO::SetValue(outbound.protocolSettings, info[IOBOUND_DATA_TYPE::IO_ADDRESS].toString(), "vnext", 0, "address");
        QJsonIO::SetValue(outbound.protocolSettings, info[IOBOUND_DATA_TYPE::IO_PORT].toInt(), "vnext", 0, "port");
        return true;
    }

    return false;
}

std::optional<QString> BuiltinSerializer::Serialize(const QString &name, const OutboundObject &outbound) const
{
    const auto protocol = outbound.outboundSettings.protocol;
    const auto out = outbound.outboundSettings.protocolSettings;
    const auto stream = outbound.outboundSettings.streamSettings;

    if (protocol == QStringLiteral("http") || protocol == QStringLiteral("socks"))
    {
        QUrl url;
        url.setScheme(protocol);
        url.setHost(QJsonIO::GetValue(out, { "servers", 0, "address" }).toString());
        url.setPort(QJsonIO::GetValue(out, { "servers", 0, "port" }).toInt());
        if (QJsonIO::GetValue(out, { "servers", 0 }).toObject().contains(QStringLiteral("users")))
        {
            url.setUserName(QJsonIO::GetValue(out, { "servers", 0, "users", 0, "user" }).toString());
            url.setPassword(QJsonIO::GetValue(out, { "servers", 0, "users", 0, "pass" }).toString());
        }
        return url.toString();
    }
    if (protocol == QStringLiteral("vless"))
    {
        return SerializeVLESS(name, outbound.outboundSettings.protocolSettings, outbound.outboundSettings.streamSettings);
    }
    if (protocol == QStringLiteral("vmess"))
    {
        return SerializeVMess(name, outbound.outboundSettings.protocolSettings, outbound.outboundSettings.streamSettings);
    }
    if (protocol == QStringLiteral("shadowsocks"))
    {
        return SerializeSS(name, outbound.outboundSettings.protocolSettings, outbound.outboundSettings.streamSettings);
    }

    return std::nullopt;
}

std::optional<std::pair<QString, OutboundObject>> BuiltinSerializer::Deserialize(const QString &link) const
{
    if (link.startsWith(QStringLiteral("http://")) || link.startsWith(QStringLiteral("socks://")))
    {
        const QUrl url = link;
        QJsonObject root;
        QJsonIO::SetValue(root, url.host(), "servers", 0, "address");
        QJsonIO::SetValue(root, url.port(), "servers", 0, "port");
        QJsonIO::SetValue(root, url.userName(), "servers", 0, "users", 0, "user");
        QJsonIO::SetValue(root, url.password(), "servers", 0, "users", 0, "pass");
        OutboundObject out;
        out.outboundSettings.protocolSettings = IOProtocolSettings{ root };
        out.outboundSettings.protocol = url.scheme();
        return std::make_pair(url.fragment(), out);
    }

    if (link.startsWith(QStringLiteral("ss://")))
    {
        const auto result = DeserializeSS(link);
        if (!result)
            return std::nullopt;
        const auto &[protocol, stream, name] = *result;
        OutboundObject o;
        o.outboundSettings.protocol = QStringLiteral("shadowsocks");
        o.outboundSettings.protocolSettings = protocol;
        o.outboundSettings.streamSettings = stream;
        o.name = name;
        return std::make_pair(name, o);
    }

    if (link.startsWith(QStringLiteral("vmess://")))
    {
        const auto result = DeserializeVMess(link);
        if (!result)
            return std::nullopt;
        const auto &[protocol, stream, name] = *result;
        OutboundObject o;
        o.outboundSettings.protocol = QStringLiteral("vmess");
        o.outboundSettings.protocolSettings = protocol;
        o.outboundSettings.streamSettings = stream;
        o.name = name;
        return std::make_pair(name, o);
    }

    if (link.startsWith(QStringLiteral("vless://")))
    {
        const auto result = DeserializeVLESS(link);
        if (!result)
            return std::nullopt;
        const auto &[protocol, stream, name] = *result;
        OutboundObject o;
        o.outboundSettings.protocol = QStringLiteral("vless");
        o.outboundSettings.protocolSettings = protocol;
        o.outboundSettings.streamSettings = stream;
        o.name = name;
        return std::make_pair(name, o);
    }

    return std::nullopt;
}

QString SerializeVLESS(const QString &name, const IOProtocolSettings &out, const IOStreamSettings &stream)
{
    QUrl url;
    url.setFragment(QUrl::toPercentEncoding(name));
    url.setScheme(QStringLiteral("vless"));
    url.setHost(QJsonIO::GetValue(out, { "vnext", 0, "address" }).toString());
    url.setPort(QJsonIO::GetValue(out, { "vnext", 0, "port" }).toInt());
    url.setUserName(QJsonIO::GetValue(out, { "vnext", 0, "users", 0, "id" }).toString());

    // -------- COMMON INFORMATION --------
    QUrlQuery query;
    const auto enc = QJsonIO::GetValue(out, { "vnext", 0, "users", 0, "encryption" }).toString(QStringLiteral("none"));
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
        const auto flow = QJsonIO::GetValue(out, QStringLiteral("vnext"), 0, QStringLiteral("users"), 0, QStringLiteral("flow")).toString();
        query.addQueryItem(QStringLiteral("flow"), flow);
    }

    // ======== END OF QUERY ========
    url.setQuery(query);
    return url.toString(QUrl::FullyEncoded);
}

QString SerializeVMess(const QString &alias, const IOProtocolSettings &serverJson, const IOStreamSettings &streamJson)
{
    Qv2ray::Models::VMessClientObject server;
    server.loadJson(serverJson[QStringLiteral("vnext")].toArray().first());

    Qv2ray::Models::StreamSettingsObject stream;
    stream.loadJson(streamJson);

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
    url.setHost(server.address);
    url.setPort(server.port);
    url.setUserName(protocol);
    url.setQuery(query);
    return url.toString();
}

QString SerializeSS(const QString &name, const IOProtocolSettings &out, const IOStreamSettings &)
{
    Qv2ray::Models::ShadowSocksClientObject server;
    server.loadJson(out);
    QUrl url;
    const auto plainUserInfo = server.method + ":" + server.password;
    const auto userinfo = plainUserInfo.toUtf8().toBase64(QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals);
    url.setUserInfo(userinfo);
    url.setScheme(QStringLiteral("ss"));
    url.setHost(server.address);
    url.setPort(server.port);
    url.setFragment(name);
    return url.toString(QUrl::ComponentFormattingOption::FullyEncoded);
}

const static QStringList NetworkType{ "tcp", "http", "ws", "kcp", "quic" };
const static QStringList QuicSecurityTypes{ "none", "aes-128-gcm", "chacha20-poly1305" };
const static QStringList QuicKcpHeaderTypes{ "none", "srtp", "utp", "wechat-video", "dtls", "wireguard" };
const static QStringList FalseTypes{ "false", "False", "No", "Off", "0" };

std::optional<std::tuple<IOProtocolSettings, IOStreamSettings, QString>> DeserializeVLESS(const QString &link)
{
    // must start with vless://
    if (!link.startsWith("vless://"))
    {
        //        *errMessage = QObject::tr("VLESS link should start with vless://");
        return std::nullopt;
    }

    // parse url
    QUrl url(link);
    if (!url.isValid())
    {
        //        *errMessage = QObject::tr("link parse failed: %1").arg(url.errorString());
        return std::nullopt;
    }

    // fetch host
    const auto hostRaw = url.host();
    if (hostRaw.isEmpty())
    {
        //        *errMessage = QObject::tr("empty host");
        return std::nullopt;
    }
    const auto host = (hostRaw.startsWith('[') && hostRaw.endsWith(']')) ? hostRaw.mid(1, hostRaw.length() - 2) : hostRaw;

    // fetch port
    const auto port = url.port();
    if (port == -1)
    {
        //        *errMessage = QObject::tr("missing port");
        return std::nullopt;
    }

    // fetch remarks
    const auto remarks = url.fragment();

    // fetch uuid
    const auto uuid = url.userInfo();
    if (uuid.isEmpty())
    {
        //        *errMessage = QObject::tr("missing uuid");
        return std::nullopt;
    }

    // initialize QJsonObject with basic info
    QJsonObject outbound;
    QJsonObject stream;

    QJsonIO::SetValue(outbound, "vless", "protocol");
    QJsonIO::SetValue(outbound, host, { "settings", "vnext", 0, "address" });
    QJsonIO::SetValue(outbound, port, { "settings", "vnext", 0, "port" });
    QJsonIO::SetValue(outbound, uuid, { "settings", "vnext", 0, "users", 0, "id" });

    // parse query
    QUrlQuery query(url.query());

    // handle type
    const auto hasType = query.hasQueryItem("type");
    const auto type = hasType ? query.queryItemValue("type") : "tcp";
    if (type != "tcp")
        QJsonIO::SetValue(stream, type, "network");

    // handle encryption
    const auto hasEncryption = query.hasQueryItem("encryption");
    const auto encryption = hasEncryption ? query.queryItemValue("encryption") : "none";
    QJsonIO::SetValue(outbound, encryption, { "settings", "vnext", 0, "users", 0, "encryption" });

    // type-wise settings
    if (type == "kcp")
    {
        const auto hasSeed = query.hasQueryItem("seed");
        if (hasSeed)
            QJsonIO::SetValue(stream, query.queryItemValue("seed"), { "kcpSettings", "seed" });

        const auto hasHeaderType = query.hasQueryItem("headerType");
        const auto headerType = hasHeaderType ? query.queryItemValue("headerType") : "none";
        if (headerType != "none")
            QJsonIO::SetValue(stream, headerType, { "kcpSettings", "header", "type" });
    }
    else if (type == "http")
    {
        const auto hasPath = query.hasQueryItem("path");
        const auto path = hasPath ? QUrl::fromPercentEncoding(query.queryItemValue("path").toUtf8()) : "/";
        if (path != "/")
            QJsonIO::SetValue(stream, path, { "httpSettings", "path" });

        const auto hasHost = query.hasQueryItem("host");
        if (hasHost)
        {
            const auto hosts = QJsonArray::fromStringList(query.queryItemValue("host").split(","));
            QJsonIO::SetValue(stream, hosts, { "httpSettings", "host" });
        }
    }
    else if (type == "ws")
    {
        const auto hasPath = query.hasQueryItem("path");
        const auto path = hasPath ? QUrl::fromPercentEncoding(query.queryItemValue("path").toUtf8()) : "/";
        if (path != "/")
            QJsonIO::SetValue(stream, path, { "wsSettings", "path" });

        const auto hasHost = query.hasQueryItem("host");
        if (hasHost)
        {
            QJsonIO::SetValue(stream, query.queryItemValue("host"), { "wsSettings", "headers", "Host" });
        }
    }
    else if (type == "quic")
    {
        const auto hasQuicSecurity = query.hasQueryItem("quicSecurity");
        if (hasQuicSecurity)
        {
            const auto quicSecurity = query.queryItemValue("quicSecurity");
            QJsonIO::SetValue(stream, quicSecurity, { "quicSettings", "security" });

            if (quicSecurity != "none")
            {
                const auto key = query.queryItemValue("key");
                QJsonIO::SetValue(stream, key, { "quicSettings", "key" });
            }

            const auto hasHeaderType = query.hasQueryItem("headerType");
            const auto headerType = hasHeaderType ? query.queryItemValue("headerType") : "none";
            if (headerType != "none")
                QJsonIO::SetValue(stream, headerType, { "quicSettings", "header", "type" });
        }
    }
    else if (type == "grpc")
    {
        const auto hasServiceName = query.hasQueryItem("serviceName");
        if (hasServiceName)
        {
            const auto serviceName = QUrl::fromPercentEncoding(query.queryItemValue("serviceName").toUtf8());
            if (serviceName != "GunService")
            {
                QJsonIO::SetValue(stream, serviceName, { "grpcSettings", "serviceName" });
            }
        }
    }

    // tls-wise settings
    const auto hasSecurity = query.hasQueryItem("security");
    const auto security = hasSecurity ? query.queryItemValue("security") : "none";
    const auto tlsKey = security == "xtls" ? "xtlsSettings" : "tlsSettings";
    if (security != "none")
    {
        QJsonIO::SetValue(stream, security, "security");
    }
    // sni
    const auto hasSNI = query.hasQueryItem("sni");
    if (hasSNI)
    {
        const auto sni = query.queryItemValue("sni");
        QJsonIO::SetValue(stream, sni, { tlsKey, "serverName" });
    }
    // alpn
    const auto hasALPN = query.hasQueryItem("alpn");
    if (hasALPN)
    {
        const auto alpnRaw = QUrl::fromPercentEncoding(query.queryItemValue("alpn").toUtf8());
        const auto alpnArray = QJsonArray::fromStringList(alpnRaw.split(","));
        QJsonIO::SetValue(stream, alpnArray, { tlsKey, "alpn" });
    }
    // xtls-specific
    if (security == "xtls")
    {
        const auto flow = query.queryItemValue("flow");
        QJsonIO::SetValue(outbound, flow, { "settings", "vnext", 0, "users", 0, "flow" });
    }

    return std::make_tuple(IOProtocolSettings{ outbound }, IOStreamSettings{ stream }, remarks);
}

std::optional<std::tuple<IOProtocolSettings, IOStreamSettings, QString>> DeserializeVMess(const QString &link)
{
    QUrl url{ link };
    QUrlQuery query{ url };
    if (!url.isValid())
        return std::nullopt;

    // If previous alias is empty, just the PS is needed, else, append a "_"
    const auto name = url.fragment(QUrl::FullyDecoded).trimmed();

    VMessClientObject server;

    StreamSettingsObject stream;
    QString net;
    bool tls = false;
    // Check streamSettings
    {
        for (const auto &_protocol : url.userName().split("+"))
        {
            if (_protocol == "tls")
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
        stream.security = tls ? "tls" : "";
    }
    // Host Port UUID AlterID
    {
        const auto host = url.host();
        int port = url.port();
        QString uuid;
        int aid;
        {
            const auto pswd = url.password();
            const auto index = pswd.lastIndexOf("-");
            uuid = pswd.mid(0, index);
            aid = pswd.right(pswd.length() - index - 1).toInt();
        }
        server.address = host;
        server.port = port;
        server.id = uuid;
        server.alterId = aid;
        server.security = "auto";
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
        if (net == "tcp")
        {
            stream.tcpSettings->header->type = getQueryValue("type", "none");
        }
        else if (net == "http")
        {
            stream.httpSettings->host->append(getQueryValue("host", ""));
            stream.httpSettings->path = getQueryValue("path", "/");
        }
        else if (net == "ws")
        {
            stream.wsSettings->headers->insert("Host", getQueryValue("host", ""));
            stream.wsSettings->path = getQueryValue("path", "/");
        }
        else if (net == "kcp")
        {
            stream.kcpSettings->seed = getQueryValue("seed", "");
            stream.kcpSettings->header->type = getQueryValue("type", "none");
        }
        else if (net == "quic")
        {
            stream.quicSettings->security = getQueryValue("security", "none");
            stream.quicSettings->key = getQueryValue("key", "");
            stream.quicSettings->header->type = getQueryValue("type", "none");
        }
    }

    if (tls)
    {
        // stream.tlsSettings.allowInsecure = !FalseTypes.contains(getQueryValue("allowInsecure", "false"));
        stream.tlsSettings->serverName = getQueryValue("tlsServerName", "");
    }

    IOProtocolSettings vConf;
    QJsonArray vnextArray;
    vnextArray.append(server.toJson());
    vConf["vnext"] = vnextArray;

    return std::make_tuple(vConf, IOStreamSettings{ stream.toJson() }, name);
}

std::optional<std::tuple<IOProtocolSettings, IOStreamSettings, QString>> DeserializeSS(const QString &link)
{
    ShadowSocksClientObject server;
    QString d_name;

    auto uri = link.mid(5);
    auto hashPos = uri.lastIndexOf("#");
    //    DEBUG("Hash sign position: " + QSTRN(hashPos));

    if (hashPos >= 0)
    {
        // Get the name/remark
        d_name = uri.mid(uri.lastIndexOf("#") + 1);
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

        server.address = decoded.mid(0, colonPos);
        server.port = decoded.mid(colonPos + 1).toInt();
    }
    else
    {
        // SIP002 URI scheme
        auto x = QUrl::fromUserInput(uri);
        server.address = x.host();
        server.port = x.port();
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
    IOProtocolSettings settings{ QJsonObject{ { "servers", QJsonArray{ server.toJson() } } } };
    return std::make_tuple(settings, IOStreamSettings{}, d_name);
}
