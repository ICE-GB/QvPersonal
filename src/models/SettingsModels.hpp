#pragma once

#include "Common/CommonTypes.hpp"
#include "Common/Settings.hpp"
#include "V2RayModels.hpp"

#include <QJsonObject>
#include <QString>

namespace Qv2ray::Models
{
    struct Qv2rayAppearanceConfig
    {
        enum UIStyleType
        {
            AUTO,
            ALWAYS_ON,
            ALWAYS_OFF
        };

        Bindable<UIStyleType> DarkModeUI;
        Bindable<UIStyleType> DarkModeTrayIcon;
        Bindable<QString> UITheme;
        Bindable<QString> Language;
        Bindable<qsizetype> RecentJumpListSize;
        Bindable<QList<ConnectionGroupPair>> RecentConnections;
        Bindable<int> MaximizeLogLines;
        QJS_FUNC_JSON(F(DarkModeUI, DarkModeTrayIcon, UITheme, Language, RecentJumpListSize, RecentConnections, MaximizeLogLines))
    };

    struct Qv2rayBehaviorConfig
    {
        enum AutoConnectBehavior
        {
            AUTOCONNECT_NONE = 0,
            AUTOCONNECT_FIXED = 1,
            AUTOCONNECT_LAST_CONNECTED = 2
        };
        Bindable<LatencyTestEngineId> DefaultLatencyTestEngine;
        Bindable<AutoConnectBehavior> AutoConnectBehavior;
        Bindable<bool> QuietMode;
        Bindable<ConnectionGroupPair> AutoConnectProfileId;
        Bindable<ConnectionGroupPair> LastConnectedId;
        Bindable<QString> GeoIPPath;
        Bindable<QString> GeoSitePath;
        QJS_FUNC_JSON(F(DefaultLatencyTestEngine, AutoConnectBehavior, QuietMode, AutoConnectProfileId, LastConnectedId, GeoIPPath, GeoSitePath))
    };

    struct ProtocolInboundBase
    {
        enum SniffingBehavior
        {
            SNIFFING_OFF = 0,
            SNIFFING_METADATA_ONLY = 1,
            SNIFFING_FULL = 2
        };

        Bindable<int> ListenPort;
        Bindable<SniffingBehavior> Sniffing{ SNIFFING_OFF };
        Bindable<QList<QString>> DestinationOverride{ { "http", "tls" } };
        ProtocolInboundBase(int port = 0) : ListenPort(port){};
        QJS_FUNC_JSON(F(ListenPort, Sniffing, DestinationOverride))
    };

    struct SocksInboundConfig : public ProtocolInboundBase
    {
        Bindable<bool> EnableUDP;
        Bindable<QString> UDPLocalAddress;
        SocksInboundConfig() : ProtocolInboundBase(1080){};
        QJS_FUNC_JSON(F(EnableUDP, UDPLocalAddress), B(ProtocolInboundBase))
    };

    struct HTTPInboundConfig : public ProtocolInboundBase
    {
        HTTPInboundConfig() : ProtocolInboundBase(8080){};
        QJS_FUNC_JSON(B(ProtocolInboundBase))
    };

    struct DokodemoDoorInboundConfig : public ProtocolInboundBase
    {
        enum DokoWorkingMode
        {
            TPROXY,
            REDIRECT
        };
        Bindable<DokoWorkingMode> WorkingMode{ TPROXY };
        Bindable<int> OutboundMark{ 255 };
        DokodemoDoorInboundConfig() : ProtocolInboundBase(12345){};
        QJS_FUNC_JSON(F(WorkingMode, OutboundMark), B(ProtocolInboundBase))
    };

    struct Qv2rayInboundConfig
    {
        Bindable<QString> ListenAddress{ "127.0.0.1" };
        Bindable<QString> ListenAddressV6{ "::1" };

        Bindable<bool> HasSOCKS{ true };
        Bindable<SocksInboundConfig> SOCKSConfig;

        Bindable<bool> HasHTTP{ true };
        Bindable<HTTPInboundConfig> HTTPConfig;

        Bindable<bool> HasDokodemoDoor{ false };
        Bindable<DokodemoDoorInboundConfig> DokodemoDoorConfig;

        QJS_FUNC_JSON(F(HasSOCKS, SOCKSConfig), F(HasHTTP, HTTPConfig), F(HasDokodemoDoor, DokodemoDoorConfig))
    };

    struct Qv2rayConnectionConfig
    {
        Bindable<bool> BypassBittorrent = false;
        Bindable<bool> ForceDirectConnection = false;
        Bindable<DNSObject> DNSConfig;
        Bindable<FakeDNSObject> FakeDNSConfig;
        Bindable<RouteMatrixConfig> RouteConfig;
        Bindable<int> OutboundMark;
        QJS_FUNC_JSON(F(BypassBittorrent, ForceDirectConnection, DNSConfig, FakeDNSConfig, RouteConfig))
    };

    struct Qv2rayApplicationConfigObject
    {
        Bindable<Qv2rayAppearanceConfig> appearanceConfig;
        Bindable<Qv2rayBehaviorConfig> behaviorConfig;
        Bindable<Qv2rayConnectionConfig> connectionConfig;
        Bindable<Qv2rayInboundConfig> inboundConfig;
        QJS_FUNC_JSON(F(appearanceConfig, behaviorConfig, connectionConfig, inboundConfig))
    };

} // namespace Qv2ray::models

using namespace Qv2ray::Models;
