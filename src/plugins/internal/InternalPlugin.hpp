#pragma once
#include "QvPlugin/PluginInterface.hpp"

#ifndef QT_STATICPLUGIN
#define QT_STATICPLUGIN
#endif

#include <QtPlugin>

class Qv2rayInternalPlugin
    : public QObject
    , public Qv2rayPlugin::Qv2rayInterface
{
    Q_OBJECT
    Q_INTERFACES(Qv2rayPlugin::Qv2rayInterface)
    Q_PLUGIN_METADATA(IID Qv2rayInterface_IID)

  public:
    virtual const Qv2rayPlugin::QvPluginMetadata GetMetadata() const override;
    virtual bool InitializePlugin() override;
    virtual void SettingsUpdated() override;

  signals:
    void PluginLog(QString) override;
    void PluginErrorMessageBox(QString title, QString message) override;
};
