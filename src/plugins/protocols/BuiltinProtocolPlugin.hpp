#pragma once

#include <QObject>
#include <QtPlugin>
#include <QvPluginInterface.hpp>

using namespace Qv2rayPlugin;

class InternalProtocolSupportPlugin
    : public QObject
    , public Qv2rayInterface
{
    Q_INTERFACES(Qv2rayPlugin::Qv2rayInterface)
    Q_PLUGIN_METADATA(IID Qv2rayInterface_IID)
    Q_OBJECT
  public:
    //
    // Basic metainfo of this plugin
    const QvPluginMetadata GetMetadata() const override
    {
        return { "Builtin Protocol Support",                                                  //
                 "Qv2ray Core Workgroup",                                                     //
                 PluginId("qv2ray_builtin_protocol"),                                         //
                 "VMess, VLESS, SOCKS, HTTP, Shadowsocks, DNS, Dokodemo-door editor support", //
                 "Qv2ray Repository",                                                         //
                 {
                     COMPONENT_OUTBOUND_HANDLER, //
                     COMPONENT_GUI               //
                 } };
    }

    bool InitializePlugin() override;
    void SettingsUpdated() override{};

  signals:
    void PluginLog(QString) override;
    void PluginErrorMessageBox(QString, QString) override;
};
