#pragma once
#include "CommonTypes.hpp"
#include "QvPluginInterface.hpp"

class BuiltinSerializer : public Qv2rayPlugin::IOutboundHandler
{
  public:
    explicit BuiltinSerializer() : Qv2rayPlugin::IOutboundHandler(){};

    virtual std::optional<QString> Serialize(const QString &name, const OutboundObject &outbound) const override;
    virtual std::optional<std::pair<QString, OutboundObject>> Deserialize(const QString &link) const override;

    virtual std::optional<PluginIOBoundData> GetOutboundInfo(const IOConnectionSettings &outbound) const override;
    virtual bool SetOutboundInfo(IOConnectionSettings &outbound, const PluginIOBoundData &info) const override;

    QList<QString> SupportedLinkPrefixes() const override
    {
        return { "http", "socks" };
    }

    QList<QString> SupportedProtocols() const override
    {
        return { "http", "socks", "shadowsocks", "vmess", "vless" };
    }
};
