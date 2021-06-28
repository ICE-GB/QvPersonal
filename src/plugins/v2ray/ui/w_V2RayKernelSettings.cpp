#include "w_V2RayKernelSettings.hpp"

#include <QFileDialog>

V2RayKernelSettings::V2RayKernelSettings(QWidget *parent) : Qv2rayPlugin::Gui::PluginSettingsWidget(parent)
{
    setupUi(this);
    settings.APIEnabled.ReadWriteBind(enableAPI, "checked", &QCheckBox::toggled);
    settings.APIPort.ReadWriteBind(statsPortBox, "value", &QSpinBox::valueChanged);
    settings.AssetsPath.ReadWriteBind(vCoreAssetsPathTxt, "text", &QLineEdit::textEdited);
    settings.CorePath.ReadWriteBind(vCorePathTxt, "text", &QLineEdit::textEdited);
    settings.LogLevel.ReadWriteBind(logLevelComboBox, "currentIndex", &QComboBox::currentIndexChanged);
    settings.OutboundMark.ReadWriteBind(somarkSB, "value", &QSpinBox::valueChanged);
}

void V2RayKernelSettings::changeEvent(QEvent *e)
{
    QWidget::changeEvent(e);
    switch (e->type())
    {
        case QEvent::LanguageChange: retranslateUi(this); break;
        default: break;
    }
}

void V2RayKernelSettings::SetSettings(const QJsonObject &json)
{
    settings.loadJson(json);
}

QJsonObject V2RayKernelSettings::GetSettings()
{
    return settings.toJson();
}

void V2RayKernelSettings::on_selectVCoreBtn_clicked()
{
    const auto core = QFileDialog::getOpenFileName(this, tr("Open V2Ray core file"), QDir::currentPath());
    if (!core.isEmpty())
        settings.CorePath = core;
}

void V2RayKernelSettings::on_selectVAssetBtn_clicked()
{
    const auto dir = QFileDialog::getExistingDirectory(this, tr("Open V2Ray assets folder"), QDir::currentPath());
    if (!dir.isEmpty())
        settings.AssetsPath = dir;
}
