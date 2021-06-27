#pragma once

#include "Common/CommonTypes.hpp"
#include "common/SettingsModels.hpp"

#ifdef QV2RAY_V2RAY_PLUGIN_USE_PROTOBUF
#define _FORWARD_DECL_IMPL(cls) class cls;
#define FORWARD_DECLARE_V2RAY_OBJECTS(ns, ...)                                                                                                                           \
    namespace ns                                                                                                                                                         \
    {                                                                                                                                                                    \
        FOR_EACH(_FORWARD_DECL_IMPL, __VA_ARGS__)                                                                                                                        \
    }

FORWARD_DECLARE_V2RAY_OBJECTS(v2ray::core, Config, InboundHandlerConfig, OutboundHandlerConfig)
FORWARD_DECLARE_V2RAY_OBJECTS(google::protobuf, Any)
FORWARD_DECLARE_V2RAY_OBJECTS(v2ray::core::transport::internet, StreamConfig)
#endif

class V2RayProfileGenerator
{
  public:
    static QByteArray GenerateConfiguration(const ProfileContent &);

  private:
    QByteArray Generate();
    explicit V2RayProfileGenerator(const ProfileContent &);

#ifdef QV2RAY_V2RAY_PLUGIN_USE_PROTOBUF
    void GenerateDNSConfig(const QJsonObject &, ::google::protobuf::Any *);
    void GenerateInboundConfig(const InboundObject &, ::v2ray::core::InboundHandlerConfig *);
    void GenerateOutboundConfig(const OutboundObject &, ::v2ray::core::OutboundHandlerConfig *);
    void GenerateStreamSettings(const IOStreamSettings &, ::v2ray::core::transport::internet::StreamConfig *);
#else
    void ProcessRoutingRule(const RuleObject &);
    void ProcessInboundConfig(const InboundObject &);
    void ProcessOutboundConfig(const OutboundObject &);
    void ProcessBalancerConfig(const OutboundObject &);
    QJsonObject GenerateStreamSettings(const IOStreamSettings &);
#endif

  private:
    const OutboundObject findOutbound(const QString &name)
    {
        for (const auto &out : profile.outbounds)
            if (out.name == name)
                return out;
        return {};
    }

    const ProfileContent profile;
    QJsonArray inbounds;
    QJsonArray outbounds;
    QJsonArray rules;
    QJsonArray balancers;
};

#ifdef QV2RAY_V2RAY_PLUGIN_USE_PROTOBUF
#undef FORWARD_DECLARE_V2RAY_OBJECTS
#endif
