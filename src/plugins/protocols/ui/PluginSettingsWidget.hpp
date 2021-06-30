#pragma once

#include "QvPlugin/Gui/QvGUIPluginInterface.hpp"
#include "ui_PluginSettingsWidget.h"
class SimplePluginSettingsWidget
    : public Qv2rayPlugin::Gui::PluginSettingsWidget
    , private Ui::PluginSettingsWidget
{
    Q_OBJECT

  public:
    explicit SimplePluginSettingsWidget(QWidget *parent = nullptr);
    void SetSettings(const QJsonObject &) override{};
    QJsonObject GetSettings() override
    {
        return {};
    };

  protected:
    void changeEvent(QEvent *e) override;
};
