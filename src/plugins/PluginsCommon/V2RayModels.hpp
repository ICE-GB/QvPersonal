#pragma once

#include "Common/CommonTypes.hpp"
#include "Utils/BindableProps.hpp"

namespace Qv2ray::Models
{
    struct RouteMatrixConfig
    {
        struct Detail
        {
            Detail(const QList<QString> &_direct = {}, const QList<QString> &_block = {}, const QList<QString> &_proxy = {})
            {
                direct = _direct;
                block = _block;
                proxy = _proxy;
            }
            Bindable<QList<QString>> direct;
            Bindable<QList<QString>> block;
            Bindable<QList<QString>> proxy;
            QJS_FUNC_COMPARE(Detail, direct, block, proxy);
            QJS_FUNC_JSON(F(proxy, block, direct))
        };

        RouteMatrixConfig(const Detail &_domains = {}, const Detail &_ips = {}, const QString &ds = {})
        {
            domainStrategy = ds;
            domains = _domains;
            ips = _ips;
        }
        Bindable<QString> domainStrategy;
        Bindable<QString> domainMatcher{ QStringLiteral("mph") };
        Bindable<Detail> domains;
        Bindable<Detail> ips;
        QJS_FUNC_COMPARE(RouteMatrixConfig, domainStrategy, domainMatcher, domains, ips);
        QJS_FUNC_JSON(F(domainStrategy, domainMatcher, domains, ips))
    };

    struct V2RayDNSObject : BasicDNSObject
    {
        enum QueryStrategy
        {
            UseIP,
            UseIPv4,
            UseIPv6,
        };

        struct V2RayDNSServerObject : BasicDNSServerObject
        {
            Bindable<bool> QV2RAY_DNS_IS_COMPLEX_DNS{ false };
            Bindable<bool> SkipFallback{ false };
            Bindable<QList<QString>> domains;
            Bindable<QList<QString>> expectIPs;
            QJS_FUNC_COMPARE(V2RayDNSServerObject, QV2RAY_DNS_IS_COMPLEX_DNS, SkipFallback, port, address, domains, expectIPs);
            QJS_FUNC_JSON(F(QV2RAY_DNS_IS_COMPLEX_DNS, SkipFallback, port, address, domains, expectIPs), B(BasicDNSServerObject))
        };

        Bindable<QList<V2RayDNSServerObject>> servers;
        Bindable<QString> clientIp;
        Bindable<QString> tag;
        Bindable<bool> disableCache{ false };
        Bindable<bool> disableFallback{ false };
        Bindable<QString> queryStrategy{ QStringLiteral("UseIP") };
        QJS_FUNC_COMPARE(V2RayDNSObject, servers, clientIp, tag, disableCache, disableFallback, queryStrategy);
        QJS_FUNC_JSON(F(servers, clientIp, tag, disableCache, disableFallback, queryStrategy), B(BasicDNSObject));
        static auto fromJson(const QJsonObject &o)
        {
            V2RayDNSObject dns;
            dns.loadJson(o);
            return dns;
        }
    };

    struct FakeDNSObject
    {
        Bindable<QString> ipPool{ QStringLiteral("198.18.0.0/15") };
        Bindable<int> poolSize{ 65535 };
        QJS_FUNC_COMPARE(FakeDNSObject, ipPool, poolSize);
        QJS_FUNC_JSON(F(ipPool, poolSize))
        static auto fromJson(const QJsonObject &o)
        {
            FakeDNSObject dns;
            dns.loadJson(o);
            return dns;
        }
    };

    namespace transfer
    {
        struct HTTPRequestObject
        {
            Bindable<QString> version{ QStringLiteral("1.1") };
            Bindable<QString> method{ QStringLiteral("GET") };
            Bindable<QList<QString>> path{ { QStringLiteral("/") } };
            Bindable<QMap<QString, QStringList>> headers{
                { { QStringLiteral("Host"), { QStringLiteral("www.baidu.com"), QStringLiteral("www.bing.com") } },
                  { QStringLiteral("User-Agent"),
                    { QStringLiteral("Mozilla/5.0 (Windows NT 10.0; WOW64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/53.0.2785.143 Safari/537.36"),
                      QStringLiteral(
                          "Mozilla/5.0 (iPhone; CPU iPhone OS 10_0_2 like Mac OS X) AppleWebKit/601.1 (KHTML, like Gecko) CriOS/53.0.2785.109 Mobile/14A456 Safari/601.1.46") } },
                  { QStringLiteral("Accept-Encoding"), { QStringLiteral("gzip, deflate") } },
                  { QStringLiteral("Connection"), { QStringLiteral("keep-alive") } },
                  { QStringLiteral("Pragma"), { QStringLiteral("no-cache") } } }
            };
            QJS_FUNC_COMPARE(HTTPRequestObject, version, method, path, headers);
            QJS_FUNC_JSON(F(version, method, path, headers))
        };

        struct HTTPResponseObject
        {
            Bindable<QString> version{ QStringLiteral("1.1") };
            Bindable<QString> status{ QStringLiteral("200") };
            Bindable<QString> reason{ QStringLiteral("OK") };
            Bindable<QMap<QString, QStringList>> headers{ { { QStringLiteral("Content-Type"),
                                                              { QStringLiteral("application/octet-stream"), QStringLiteral("video/mpeg") } },
                                                            { QStringLiteral("Transfer-Encoding"), { QStringLiteral("chunked") } },
                                                            { QStringLiteral("Connection"), { QStringLiteral("keep-alive") } },
                                                            { QStringLiteral("Pragma"), { QStringLiteral("no-cache") } } } };
            QJS_FUNC_COMPARE(HTTPResponseObject, version, reason, status, headers);
            QJS_FUNC_JSON(F(version, reason, status, status))
        };

        struct TCPHeader_Internal
        {
            Bindable<QString> type{ QStringLiteral("none") };
            Bindable<HTTPRequestObject> request;
            Bindable<HTTPResponseObject> response;
            QJS_FUNC_COMPARE(TCPHeader_Internal, type, request, response);
            QJS_FUNC_JSON(F(type, request, response))
        };

        struct ObfsHeaderObject
        {
            Bindable<QString> type{ QStringLiteral("none") };
            QJS_FUNC_COMPARE(ObfsHeaderObject, type);
            QJS_FUNC_JSON(F(type))
        };

        struct TCPObject
        {
            Bindable<TCPHeader_Internal> header;
            QJS_FUNC_COMPARE(TCPObject, header);
            QJS_FUNC_JSON(F(header))
        };

        struct KCPObject
        {
            Bindable<int> mtu{ 1350 };
            Bindable<int> tti{ 50 };
            Bindable<int> uplinkCapacity{ 5 };
            Bindable<int> downlinkCapacity{ 20 };
            Bindable<bool> congestion{ false };
            Bindable<int> readBufferSize{ 2 };
            Bindable<int> writeBufferSize{ 2 };
            Bindable<QString> seed;
            Bindable<ObfsHeaderObject> header;
            QJS_FUNC_COMPARE(KCPObject, mtu, tti, uplinkCapacity, congestion, readBufferSize, writeBufferSize, seed, header);
            QJS_FUNC_JSON(F(mtu, tti, uplinkCapacity, congestion, readBufferSize, writeBufferSize, seed, header))
        };

        struct WebSocketObject
        {
            Bindable<QString> path{ QStringLiteral("/") };
            Bindable<QMap<QString, QString>> headers;
            Bindable<int> maxEarlyData{ 0 };
            Bindable<bool> useBrowserForwarding{ false };
            Bindable<QString> earlyDataHeaderName;
            QJS_FUNC_COMPARE(WebSocketObject, path, headers, maxEarlyData, useBrowserForwarding, useBrowserForwarding);
            QJS_FUNC_JSON(F(path, headers, maxEarlyData, useBrowserForwarding, useBrowserForwarding))
        };

        struct HttpObject
        {
            Bindable<QList<QString>> host;
            Bindable<QString> path{ QStringLiteral("/") };
            Bindable<QString> method;
            QJS_FUNC_COMPARE(HttpObject, host, path, method);
            QJS_FUNC_JSON(F(host, path, method))
        };

        struct DomainSocketObject
        {
            Bindable<QString> path{ QStringLiteral("/") };
            Bindable<bool> abstract{ false };
            Bindable<bool> padding{ false };
            QJS_FUNC_COMPARE(DomainSocketObject, path, abstract, padding);
            QJS_FUNC_JSON(F(path, abstract, padding))
        };

        struct QuicObject
        {
            Bindable<QString> security{ QStringLiteral("none") };
            Bindable<QString> key;
            Bindable<ObfsHeaderObject> header;
            QJS_FUNC_COMPARE(QuicObject, security, key, header);
            QJS_FUNC_JSON(F(security, key, header))
        };

        struct gRPCObject
        {
            Bindable<QString> serviceName{ QStringLiteral("GunService") };
            QJS_FUNC_COMPARE(gRPCObject, serviceName);
            QJS_FUNC_JSON(F(serviceName))
        };

        struct SockoptObject
        {
            Bindable<int> mark{ 255 };
            Bindable<int> tcpKeepAliveInterval{ 0 };
            Bindable<bool> tcpFastOpen{ false };
            Bindable<QString> tproxy{ QStringLiteral("off") };
            QJS_FUNC_COMPARE(SockoptObject, mark, tcpKeepAliveInterval, tcpFastOpen, tproxy);
            QJS_FUNC_JSON(F(mark, tcpKeepAliveInterval, tcpFastOpen, tproxy))
        };

        struct CertificateObject
        {
            Bindable<QString> usage{ QStringLiteral("encipherment") };
            Bindable<QString> certificateFile;
            Bindable<QString> keyFile;
            Bindable<QList<QString>> certificate;
            Bindable<QList<QString>> key;
            QJS_FUNC_COMPARE(CertificateObject, usage, certificateFile, keyFile, certificate, key);
            QJS_FUNC_JSON(F(usage, certificateFile, keyFile, certificate, key))
        };

        struct TLSObject
        {
            Bindable<QString> serverName;
            Bindable<bool> disableSessionResumption{ true };
            Bindable<bool> disableSystemRoot{ false };
            Bindable<QList<QString>> alpn;
            Bindable<QList<CertificateObject>> certificates;
            QJS_FUNC_COMPARE(TLSObject, serverName, disableSessionResumption, disableSystemRoot, alpn, certificates);
            QJS_FUNC_JSON(F(serverName, disableSessionResumption, disableSystemRoot, alpn, certificates))
        };

        struct XTLSObject
        {
            Bindable<QString> serverName;
            Bindable<bool> disableSessionResumption{ true };
            Bindable<bool> disableSystemRoot{ false };
            Bindable<QList<QString>> alpn;
            Bindable<QList<CertificateObject>> certificates;
            QJS_FUNC_COMPARE(XTLSObject, serverName, disableSessionResumption, disableSystemRoot, alpn, certificates);
            QJS_FUNC_JSON(F(serverName, disableSessionResumption, disableSystemRoot, alpn, certificates))
        };
    } // namespace transfer

    struct StreamSettingsObject
    {
        Bindable<QString> network{ QStringLiteral("tcp") };
        Bindable<QString> security{ QStringLiteral("none") };
        Bindable<transfer::SockoptObject> sockopt;
        Bindable<transfer::TLSObject> tlsSettings;
        Bindable<transfer::XTLSObject> xtlsSettings;
        Bindable<transfer::TCPObject> tcpSettings;
        Bindable<transfer::KCPObject> kcpSettings;
        Bindable<transfer::WebSocketObject> wsSettings;
        Bindable<transfer::HttpObject> httpSettings;
        Bindable<transfer::DomainSocketObject> dsSettings;
        Bindable<transfer::QuicObject> quicSettings;
        Bindable<transfer::gRPCObject> grpcSettings;
        QJS_FUNC_COMPARE(StreamSettingsObject, network, security, sockopt, tlsSettings, xtlsSettings, tcpSettings, kcpSettings, wsSettings, httpSettings, dsSettings,
                         quicSettings, grpcSettings);
        QJS_FUNC_JSON(P(network, security, sockopt, tlsSettings, xtlsSettings, tcpSettings, kcpSettings, wsSettings, httpSettings, dsSettings, quicSettings,
                        grpcSettings))
        static auto fromJson(const QJsonObject &o)
        {
            StreamSettingsObject stream;
            stream.loadJson(o);
            return stream;
        };

        operator IOStreamSettings()
        {
            return IOStreamSettings{ this->toJson() };
        }
    };

    struct HTTPSOCKSObject
    {
        Bindable<QString> user;
        Bindable<QString> pass;
        Bindable<int> level{ 0 };
        QJS_FUNC_JSON(P(user, pass, level))
    };

    //
    // ShadowSocks Server
    struct ShadowSocksClientObject
    {
        Bindable<QString> method{ QStringLiteral("aes-256-gcm") };
        Bindable<QString> password;
        QJS_FUNC_JSON(P(method, password))
    };

    //
    // VLESS Server
    struct VLESSClientObject
    {
        Bindable<QString> id;
        Bindable<QString> encryption{ QStringLiteral("none") };
        Bindable<QString> flow;
        QJS_FUNC_JSON(P(id, encryption, flow))
    };

    //
    // VMess Server
    constexpr auto VMESS_USER_ALTERID_DEFAULT = 0;
    struct VMessClientObject
    {
        Bindable<QString> id;
        Bindable<int> alterId{ VMESS_USER_ALTERID_DEFAULT };
        Bindable<QString> security{ QStringLiteral("auto") };
        QJS_FUNC_JSON(F(id, alterId, security))
    };

} // namespace Qv2ray::models
