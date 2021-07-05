#pragma once

#include "QvGUIPluginInterface.hpp"
#include "common/SettingsModels.hpp"
#include "ui_w_V2RayKernelSettings.h"

class V2RayKernelSettings
    : public Qv2rayPlugin::Gui::PluginSettingsWidget
    , private Ui::V2RayKernelSettings
{
    Q_OBJECT

  public:
    explicit V2RayKernelSettings(QWidget *parent = nullptr);

  protected:
    void changeEvent(QEvent *e) override;

    // PluginSettingsWidget interface
  public:
    virtual void SetSettings(const QJsonObject &) override;
    virtual QJsonObject GetSettings() override;

  private slots:
    void on_selectVCoreBtn_clicked();
    void on_selectVAssetBtn_clicked();
    void on_checkVCoreSettings_clicked();
    void on_detectCoreBtn_clicked();
    void on_resetVCoreBtn_clicked();
    void on_resetVAssetBtn_clicked();

  private:
    V2RayCorePluginSettings settings;
};
