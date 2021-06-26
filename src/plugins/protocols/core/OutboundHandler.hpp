#pragma once
#include "QvPluginInterface.hpp"

class BuiltinSerializer : public Qv2rayPlugin::IOutboundHandler
{
  public:
    explicit BuiltinSerializer() : Qv2rayPlugin::IOutboundHandler(){};

    virtual std::optional<QString> Serialize(const QString &name, const IOConnectionSettings &outbound) const override;
    virtual std::optional<std::pair<QString, IOConnectionSettings>> Deserialize(const QString &link) const override;

    virtual std::optional<PluginIOBoundData> GetOutboundInfo(const IOConnectionSettings &) const override
    {
        return std::nullopt;
    }
    virtual bool SetOutboundInfo(IOConnectionSettings &, const PluginIOBoundData &) const override
    {
        return true;
    }

    QList<QString> SupportedLinkPrefixes() const override
    {
        return { "http", "socks", "vmess", "vless", "ss" };
    }

    QList<QString> SupportedProtocols() const override
    {
        return { "http", "socks", "shadowsocks", "vmess", "vless" };
    }
};
