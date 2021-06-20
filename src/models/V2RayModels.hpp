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
            bool operator==(const Detail &) const = default;
            QJS_FUNC_JSON(F(proxy, block, direct))
        };

        RouteMatrixConfig(const Detail &_domains = {}, const Detail &_ips = {}, const QString &ds = {})
        {
            domainStrategy = ds;
            domains = _domains;
            ips = _ips;
        }
        Bindable<QString> domainStrategy;
        Bindable<QString> domainMatcher{ "mph" };
        Bindable<Detail> domains;
        Bindable<Detail> ips;
        bool operator==(const RouteMatrixConfig &) const = default;
        QJS_FUNC_JSON(F(domainStrategy, domainMatcher, domains, ips))
    };

    struct DNSObject
    {
        struct DNSServerObject
        {
            Bindable<bool> QV2RAY_DNS_IS_COMPLEX_DNS{ false };
            Bindable<bool> SkipFallback{ false };
            Bindable<int> port{ 53 };
            Bindable<QString> address;
            Bindable<QList<QString>> domains;
            Bindable<QList<QString>> expectIPs;
            bool operator==(const DNSServerObject &) const = default;
            QJS_FUNC_JSON(F(QV2RAY_DNS_IS_COMPLEX_DNS, SkipFallback, port, address, domains, expectIPs))
        };

        Bindable<QMap<QString, QString>> hosts;
        Bindable<QList<DNSServerObject>> servers;
        Bindable<QString> clientIp;
        Bindable<QString> tag;
        Bindable<bool> disableCache{ false };
        Bindable<bool> disableFallback{ false };
        Bindable<QString> queryStrategy{ "UseIP" };
        bool operator==(const DNSObject &) const = default;
        QJS_FUNC_JSON(F(hosts, servers, clientIp, tag, disableCache, disableFallback, queryStrategy))
        static auto fromJson(const QJsonObject &o)
        {
            DNSObject dns;
            dns.loadJson(o);
            return dns;
        }
    };

    struct FakeDNSObject
    {
        Bindable<QString> ipPool{ "198.18.0.0/15", true };
        Bindable<int> poolSize{ 65535 };
        bool operator==(const FakeDNSObject &) const = default;
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
            Bindable<QString> version{ "1.1" };
            Bindable<QString> method{ "GET" };
            Bindable<QList<QString>> path{ { "/" } };
            Bindable<QMap<QString, QStringList>> headers{
                { { "Host", { "www.baidu.com", "www.bing.com" } },
                  { "User-Agent",
                    { "Mozilla/5.0 (Windows NT 10.0; WOW64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/53.0.2785.143 Safari/537.36",
                      "Mozilla/5.0 (iPhone; CPU iPhone OS 10_0_2 like Mac OS X) AppleWebKit/601.1 (KHTML, like Gecko) CriOS/53.0.2785.109 Mobile/14A456 Safari/601.1.46" } },
                  { "Accept-Encoding", { "gzip, deflate" } },
                  { "Connection", { "keep-alive" } },
                  { "Pragma", { "no-cache" } } }
            };

            bool operator==(const HTTPRequestObject &) const = default;
            QJS_FUNC_JSON(F(version, method, path, headers))
        };

        struct HTTPResponseObject
        {
            Bindable<QString> version{ "1.1" };
            Bindable<QString> method{ "GET" };
            Bindable<QString> status{ "200" };
            Bindable<QMap<QString, QStringList>> headers{ { { "Content-Type", { "application/octet-stream", "video/mpeg" } },
                                                            { "Transfer-Encoding", { "chunked" } },
                                                            { "Connection", { "keep-alive" } },
                                                            { "Pragma", { "no-cache" } } } };

            bool operator==(const HTTPResponseObject &) const = default;
            QJS_FUNC_JSON(F(version, method, status, status))
        };

        struct TCPHeader_Internal
        {
            Bindable<QString> type{ "none", true };
            Bindable<HTTPRequestObject> request;
            Bindable<HTTPResponseObject> response;
            bool operator==(const TCPHeader_Internal &) const = default;
            QJS_FUNC_JSON(F(type, request, response))
        };

        struct ObfsHeaderObject
        {
            Bindable<QString> type{ "none" };
            bool operator==(const ObfsHeaderObject &) const = default;
        };

        struct TCPObject
        {
            Bindable<TCPHeader_Internal> header;
            bool operator==(const TCPObject &) const = default;
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
            bool operator==(const KCPObject &) const = default;
        };

        struct WebSocketObject
        {
            Bindable<QString> path{ "/" };
            Bindable<QMap<QString, QString>> headers;
            Bindable<int> maxEarlyData{ 0 };
            Bindable<bool> useBrowserForwarding{ false };
            Bindable<QString> earlyDataHeaderName;
            bool operator==(const WebSocketObject &) const = default;
        };

        struct HttpObject
        {
            Bindable<QList<QString>> host;
            Bindable<QString> path{ "/" };
            Bindable<QString> method;
            bool operator==(const HttpObject &) const = default;
        };

        struct DomainSocketObject
        {
            Bindable<QString> path{ "/" };
            bool operator==(const DomainSocketObject &) const = default;
        };

        struct QuicObject
        {
            Bindable<QString> security{ "none" };
            Bindable<QString> key;
            Bindable<ObfsHeaderObject> header;
            bool operator==(const QuicObject &) const = default;
        };

        struct gRPCObject
        {
            Bindable<QString> serviceName{ "GunService" };
            bool operator==(const gRPCObject &) const = default;
        };

        struct SockoptObject
        {
            Bindable<int> mark{ 255 };
            Bindable<int> tcpKeepAliveInterval{ 0 };
            Bindable<bool> tcpFastOpen{ false };
            Bindable<QString> tproxy{ "off" };
            bool operator==(const SockoptObject &) const = default;
        };

        struct CertificateObject
        {
            Bindable<QString> usage{ "encipherment" };
            Bindable<QString> certificateFile;
            Bindable<QString> keyFile;
            Bindable<QList<QString>> certificate;
            Bindable<QList<QString>> key;
            bool operator==(const CertificateObject &) const = default;
        };

        struct TLSObject
        {
            Bindable<QString> serverName;
            Bindable<bool> disableSessionResumption{ true };
            Bindable<bool> disableSystemRoot{ false };
            Bindable<QList<QString>> alpn;
            Bindable<QList<CertificateObject>> certificates;
            bool operator==(const TLSObject &) const = default;
        };

        struct XTLSObject
        {
            Bindable<QString> serverName;
            Bindable<bool> disableSessionResumption{ true };
            Bindable<bool> disableSystemRoot{ false };
            Bindable<QList<QString>> alpn;
            Bindable<QList<CertificateObject>> certificates;
            bool operator==(const XTLSObject &) const = default;
        };
    } // namespace transfer

    struct StreamSettingsObject
    {
        Bindable<QString> network{ "tcp" };
        Bindable<QString> security{ "none" };
        Bindable<transfer::SockoptObject> sockopt;
        Bindable<transfer::TLSObject> tlsSettings;
        Bindable<transfer::XTLSObject> xtlsSettings;
        Bindable<transfer::TCPObject> tcpSettings;
        Bindable<transfer::KCPObject> kcpSettings;
        Bindable<transfer::WebSocketObject> wsSettings;
        Bindable<transfer::HttpObject> HTTPConfig;
        Bindable<transfer::DomainSocketObject> dsSettings;
        Bindable<transfer::QuicObject> quicSettings;
        Bindable<transfer::gRPCObject> grpcSettings;
        bool operator==(const StreamSettingsObject &) const = default;
        QJS_FUNC_JSON(P(network, security, sockopt, tlsSettings, xtlsSettings, tcpSettings, kcpSettings, wsSettings, quicSettings, grpcSettings))
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
} // namespace Qv2ray::models
