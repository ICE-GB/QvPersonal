#pragma once

#include "Common/CommonTypes.hpp"
#include "Common/Settings.hpp"
#include "src/plugins/PluginsCommon/V2RayModels.hpp"

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

        Bindable<UIStyleType> DarkModeTrayIcon;
        Bindable<QString> UITheme;
        Bindable<QString> Language;
        Bindable<qsizetype> RecentJumpListSize;
        Bindable<QList<ProfileId>> RecentConnections;
        Bindable<int> MaximizeLogLines;
        QJS_FUNC_JSON(F(DarkModeTrayIcon, UITheme, Language, RecentJumpListSize, RecentConnections, MaximizeLogLines))
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
        Bindable<ProfileId> AutoConnectProfileId;
        Bindable<ProfileId> LastConnectedId;
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

        virtual void Propagate(InboundObject &in) const
        {
            in.inboundSettings.port = ListenPort;
            in.options[QStringLiteral("sniffing")] = QJsonObject{
                { QStringLiteral("enabled"), Sniffing != SNIFFING_OFF },                             //
                { QStringLiteral("metadataOnly"), Sniffing == SNIFFING_METADATA_ONLY },              //
                { QStringLiteral("destOverride"), QJsonArray::fromStringList(DestinationOverride) }, //
            };
        }
    };

    struct SocksInboundConfig : public ProtocolInboundBase
    {
        Bindable<bool> EnableUDP;
        Bindable<QString> UDPLocalAddress;
        SocksInboundConfig() : ProtocolInboundBase(1080){};
        QJS_FUNC_JSON(F(EnableUDP, UDPLocalAddress), B(ProtocolInboundBase))

        virtual void Propagate(InboundObject &in) const
        {
            ProtocolInboundBase::Propagate(in);
            in.inboundSettings.protocolSettings[QStringLiteral("udp")] = *EnableUDP;
            in.inboundSettings.protocolSettings[QStringLiteral("ip")] = *UDPLocalAddress;
        }
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
            REDIRECT,
            TPROXY,
        };
        Bindable<DokoWorkingMode> WorkingMode{ TPROXY };
        DokodemoDoorInboundConfig() : ProtocolInboundBase(12345){};
        QJS_FUNC_JSON(F(WorkingMode), B(ProtocolInboundBase))

        virtual void Propagate(InboundObject &in) const
        {
            ProtocolInboundBase::Propagate(in);
            in.inboundSettings.protocolSettings[QStringLiteral("network")] = QStringLiteral("tcp,udp");
            in.inboundSettings.protocolSettings[QStringLiteral("followRedirect")] = true;
        }
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
        Bindable<bool> BypassLAN{ true };
        Bindable<bool> BypassCN{ true };
        Bindable<bool> BypassBittorrent{ false };
        Bindable<bool> ForceDirectConnection{ false };
        Bindable<bool> DNSInterception{ false };
        Bindable<V2RayDNSObject> DNSConfig;
        Bindable<FakeDNSObject> FakeDNSConfig;
        Bindable<RouteMatrixConfig> RouteConfig;
        QJS_FUNC_JSON(F(BypassLAN, BypassCN, BypassBittorrent, ForceDirectConnection, DNSInterception, DNSConfig, FakeDNSConfig, RouteConfig))
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
