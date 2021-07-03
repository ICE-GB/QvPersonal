#pragma once

#include "QvPlugin/PluginInterface.hpp"

using namespace Qv2rayPlugin;

class SimpleBase64Decoder : public SubscriptionDecoder
{
  public:
    explicit SimpleBase64Decoder() : SubscriptionDecoder(){};
    SubscriptionDecodeResult DecodeData(const QByteArray &data) const override;
};

class OOCv1Decoder : public SubscriptionDecoder
{
  public:
    explicit OOCv1Decoder() : SubscriptionDecoder(){};
    SubscriptionDecodeResult DecodeData(const QByteArray &data) const override;
};

class BuiltinSubscriptionAdapterInterface : public ISubscriptionHandler
{
  public:
    explicit BuiltinSubscriptionAdapterInterface() = default;

    QList<SubscriptionInfoObject> GetInfo() const override
    {
        return {
            { SubscriptionDecoderId{ "ooc-v1" }, "Open Online Config v1", []() { return std::make_unique<OOCv1Decoder>(); } },
            { SubscriptionDecoderId{ "simple_base64" }, "Base64", []() { return std::make_unique<SimpleBase64Decoder>(); } },
        };
    }
};
