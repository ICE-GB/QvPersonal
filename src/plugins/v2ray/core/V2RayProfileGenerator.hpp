#pragma once

#include "Common/CommonTypes.hpp"

#define _FORWARD_DECL_IMPL(cls) class cls;
#define FORWARD_DECLARE_V2RAY_OBJECTS(ns, ...)                                                                                                                           \
    namespace ns                                                                                                                                                         \
    {                                                                                                                                                                    \
        FOR_EACH(_FORWARD_DECL_IMPL, __VA_ARGS__)                                                                                                                        \
    }

FORWARD_DECLARE_V2RAY_OBJECTS(v2ray::core::app::proxyman, InboundConfig, OutboundConfig)
FORWARD_DECLARE_V2RAY_OBJECTS(v2ray::core::app::browserforwarder, Config)
FORWARD_DECLARE_V2RAY_OBJECTS(v2ray::core::app::observatory, Config)
FORWARD_DECLARE_V2RAY_OBJECTS(v2ray::core::app::router, Config, RoutingRule)
FORWARD_DECLARE_V2RAY_OBJECTS(v2ray::core, Config, InboundHandlerConfig, OutboundHandlerConfig)
FORWARD_DECLARE_V2RAY_OBJECTS(v2ray::core::transport::internet, ProxyConfig, StreamConfig)

class V2RayProfileGenerator
{
  public:
    QByteArray GenerateConfiguration(const ProfileContent &);

    v2ray::core::app::router::RoutingRule GenerateRoutingRule(const RuleObject &);
    v2ray::core::app::router::Config GenetateRouting(const RoutingObject &);

    v2ray::core::transport::internet::ProxyConfig GenerateOutboundProxyConfig(const IOProtocolSettings &);
    v2ray::core::transport::internet::ProxyConfig GenerateInboundProxyConfig(const IOProtocolSettings &);

    v2ray::core::transport::internet::StreamConfig GenerateStreamConfig(const IOStreamSettings &);

    v2ray::core::app::browserforwarder::Config GenerateBrowserForwarderConfig();
    v2ray::core::app::observatory::Config GenerateObservatoryConfig();

    void GenerateStreamSettings(const IOStreamSettings &, ::v2ray::core::transport::internet::StreamConfig *);

    void GenerateInboundConfig(const InboundObject &, ::v2ray::core::InboundHandlerConfig *);
    void GenerateOutboundConfig(const OutboundObject &, v2ray::core::OutboundHandlerConfig *);
};

#undef FORWARD_DECLARE_V2RAY_OBJECTS
