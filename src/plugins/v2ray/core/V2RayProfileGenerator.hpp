#pragma once

#include "Common/CommonTypes.hpp"

#define _FORWARD_DECL_IMPL(cls) class cls;
#define FORWARD_DECLARE_V2RAY_OBJECTS(ns, ...)                                                                                                                           \
    namespace ns                                                                                                                                                         \
    {                                                                                                                                                                    \
        FOR_EACH(_FORWARD_DECL_IMPL, __VA_ARGS__)                                                                                                                        \
    }

FORWARD_DECLARE_V2RAY_OBJECTS(v2ray::core, Config, InboundHandlerConfig, OutboundHandlerConfig)
FORWARD_DECLARE_V2RAY_OBJECTS(v2ray::core::transport::internet, StreamConfig)

class V2RayProfileGenerator
{
  public:
    QByteArray GenerateConfiguration(const ProfileContent &);

    void GenerateInboundConfig(const InboundObject &, ::v2ray::core::InboundHandlerConfig *);
    void GenerateOutboundConfig(const OutboundObject &, ::v2ray::core::OutboundHandlerConfig *);
    void GenerateStreamSettings(const IOStreamSettings &, ::v2ray::core::transport::internet::StreamConfig *);
};

#undef FORWARD_DECLARE_V2RAY_OBJECTS
