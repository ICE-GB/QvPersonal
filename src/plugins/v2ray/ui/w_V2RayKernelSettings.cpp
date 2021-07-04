#include "w_V2RayKernelSettings.hpp"

#include "QvPlugin/PluginInterface.hpp"
#include "common/CommonHelpers.hpp"

#include <QFileDialog>
#include <QStandardPaths>

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

void V2RayKernelSettings::on_checkVCoreSettings_clicked()
{

    if (const auto &[result, msg] = ValidateKernel(settings.CorePath, settings.AssetsPath); !result)
    {
        QvPluginMessageBox(tr("V2Ray Core Settings"), *msg);
    }
    else
    {
        const auto content = tr("V2Ray path configuration check passed.") + //
                             QStringLiteral("\n\n") +                       //
                             tr("Current version of V2Ray is: ") +          //
                             QStringLiteral("\n") +                         //
                             *msg;
        QvPluginMessageBox(tr("V2Ray Core Settings"), content);
    }
}

void V2RayKernelSettings::on_detectCoreBtn_clicked()
{
    QStringList searchPaths;

    // A cursed v2ray core searcher.

    searchPaths << QDir::homePath();
    for (const auto &sp : {
             QStandardPaths::AppDataLocation,      //
             QStandardPaths::AppConfigLocation,    //
             QStandardPaths::AppLocalDataLocation, //
             QStandardPaths::ApplicationsLocation, //
             QStandardPaths::HomeLocation,         //
             QStandardPaths::DesktopLocation,      //
             QStandardPaths::DocumentsLocation,    //
             QStandardPaths::DownloadLocation,     //
         })
        for (const auto &dn : {
                 QStringLiteral("v2ray"),            //
                 QStringLiteral("v2ray-core"),       //
                 QStringLiteral("v2ray-windows-64"), //
             })
            searchPaths << QStandardPaths::locateAll(sp, dn, QStandardPaths::LocateDirectory);

#ifdef Q_OS_WINDOWS
    searchPaths << QDir::homePath() + QStringLiteral("/scoop/apps/v2ray/current/");
    searchPaths << QDir::homePath() + QStringLiteral("/source/repos/");

    for (const auto &dl : QDir::drives())
        for (const auto &dn : {
                 QStringLiteral("v2ray"),            //
                 QStringLiteral("v2ray-core"),       //
                 QStringLiteral("v2ray-windows-64"), //
             })
            searchPaths << dl.absolutePath() + dn;
#elif defined(Q_OS_MACOS)
#elif defined(Q_OS_LINUX)
#endif

    searchPaths.removeDuplicates();

    const auto result = QStandardPaths::findExecutable(QStringLiteral("v2ray"), searchPaths);
    if (result.isEmpty())
    {
        QvPluginMessageBox(QStringLiteral("V2Ray Core Detection"), QStringLiteral("Cannot find v2ray core."));
        return;
    }

    QvPluginMessageBox(QStringLiteral("V2Ray Core Detection"), QStringLiteral("Found v2ray.exe at: ") + result);
    settings.CorePath = result;
    const auto assetsPath = QFileInfo(result).dir();
    const auto dir = assetsPath.entryList();
    if (dir.contains(QStringLiteral("geosite.dat")) && dir.contains(QStringLiteral("geoip.dat")))
    {
        QvPluginMessageBox(QStringLiteral("V2Ray Core Detection"), QStringLiteral("Found assets at: ") + assetsPath.path());
        settings.AssetsPath = assetsPath.path();
    }
}
