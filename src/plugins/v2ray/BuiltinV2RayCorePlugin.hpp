#pragma once

#include "common/SettingsModels.hpp"

#include <QObject>
#include <QtPlugin>
#include <QvPluginInterface.hpp>

using namespace Qv2rayPlugin;

class BuiltinV2RayCorePlugin
    : public QObject
    , Qv2rayInterface
{
    Q_INTERFACES(Qv2rayPlugin::Qv2rayInterface)
    Q_PLUGIN_METADATA(IID Qv2rayInterface_IID)
    Q_OBJECT

  public:
    V2RayCorePluginSettings settings;

    const QvPluginMetadata GetMetadata() const override;
    ~BuiltinV2RayCorePlugin();
    bool InitializePlugin() override;
    void SettingsUpdated() override;

  signals:
    void PluginLog(QString) override;
    void PluginErrorMessageBox(QString, QString) override;
};
