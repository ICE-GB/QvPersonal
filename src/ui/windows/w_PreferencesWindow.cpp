#include "w_PreferencesWindow.hpp"

#include "AutoLaunchHelper/AutoLaunchHelper.hpp"
#include "Common/ProfileHelpers.hpp"
#include "Common/Utils.hpp"
#include "Profile/ProfileManager.hpp"
#include "QvPluginInterface.hpp"
#include "StyleManager/StyleManager.hpp"
#include "ui/widgets/editors/DnsSettingsWidget.hpp"
#include "ui/widgets/editors/RouteSettingsMatrix.hpp"

#include <QColorDialog>
#include <QCompleter>
#include <QDesktopServices>
#include <QFileDialog>
#include <QHostInfo>
#include <QInputDialog>
#include <QMessageBox>

#define LOADINGCHECK                                                                                                                                                     \
    if (!finishedLoading)                                                                                                                                                \
        return;

#define NEEDRESTART                                                                                                                                                      \
    LOADINGCHECK                                                                                                                                                         \
    if (finishedLoading)                                                                                                                                                 \
        NeedRestart = true;

#define SET_PROXY_UI_ENABLE(_enabled)                                                                                                                                    \
    qvProxyAddressTxt->setEnabled(_enabled);                                                                                                                             \
    qvProxyPortCB->setEnabled(_enabled);

#define SET_AUTOSTART_UI_ENABLED(_enabled)                                                                                                                               \
    autoStartConnCombo->setEnabled(_enabled);                                                                                                                            \
    autoStartSubsCombo->setEnabled(_enabled);

PreferencesWindow::PreferencesWindow(QWidget *parent) : QvDialog("PreferenceWindow", parent), AppConfig()
{
    setupUi(this);
    //
    QvMessageBusConnect();
    textBrowser->setHtml(ReadFile(":/assets/credit.html"));
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    configdirLabel->setText("ANYWHERE");

    // We add locales
    {
        languageComboBox->setDisabled(true);
        // Since we can't have languages detected. It worths nothing to translate these.
        languageComboBox->setToolTip("Cannot find any language providers.");
    }

    // Set auto start button state
    SetAutoStartButtonsState(GetLaunchAtLoginStatus());
    themeCombo->addItems(StyleManager->AllStyles());
    //
    qvVersion->setText(QV2RAY_VERSION_STRING);
    qvBuildInfo->setText("QV2RAY_BUILD_INFO");
    qvBuildExInfo->setText("QV2RAY_BUILD_EXTRA_INFO");
    qvBuildTime->setText(__DATE__ " " __TIME__);
    qvPluginInterfaceVersionLabel->setText(tr("Version: %1").arg(Qv2rayPlugin::QV2RAY_PLUGIN_INTERFACE_VERSION));
    //
    // Deep copy
    AppConfig.loadJson(GlobalConfig->toJson());
    BaselibConfig.loadJson(QvBaselib->GetConfig()->toJson());
    //
    AppConfig.appearanceConfig->UITheme.ReadWriteBind(themeCombo, "currentText", &QComboBox::currentIndexChanged);
    AppConfig.appearanceConfig->DarkModeUI.ReadWriteBind(darkThemeCB, "checked", &QCheckBox::stateChanged);
    AppConfig.appearanceConfig->DarkModeTrayIcon.ReadWriteBind(darkTrayCB, "checked", &QCheckBox::stateChanged);
    AppConfig.appearanceConfig->Language.ReadWriteBind(languageComboBox, "currentText", &QComboBox::currentIndexChanged);
    AppConfig.behaviorConfig->QuietMode.ReadWriteBind(quietModeCB, "checked", &QCheckBox::stateChanged);

    listenIPTxt->setText(AppConfig.inboundConfig->ListenAddress);

    // BEGIN HTTP CONFIGURATION
    {
        AppConfig.inboundConfig->HasHTTP.ReadWriteBind(httpGroupBox, "checked", &QGroupBox::toggled);
        AppConfig.inboundConfig->HTTPConfig->ListenPort.ReadWriteBind(httpPortLE, "value", &QSpinBox::valueChanged);
        AppConfig.inboundConfig->HTTPConfig->Sniffing.Observe(
            [this](auto newVal)
            {
#pragma message("TODO: Should not update the CB")
                //            httpSniffingCB->setCurrentIndex(newVal);
                const bool hasHttp = AppConfig.inboundConfig->HasHTTP;
                const bool isSniffingOn = newVal != ProtocolInboundBase::SNIFFING_OFF;

                // Disable these fields when HTTP is not enabled & sniffing is turned off.
                httpOverrideHTTPCB->setEnabled(hasHttp && isSniffingOn);
                httpOverrideTLSCB->setEnabled(hasHttp && isSniffingOn);
                httpOverrideFakeDNSCB->setEnabled(hasHttp && isSniffingOn);
            });

        AppConfig.inboundConfig->HTTPConfig->DestinationOverride.Observe(
            [this](auto newVal)
            {
                httpOverrideHTTPCB->setChecked(newVal.contains("http"));
                httpOverrideTLSCB->setChecked(newVal.contains("tls"));
                httpOverrideFakeDNSCB->setChecked(newVal.contains("fakedns"));
            });
    }
    // END OF HTTP CONFIGURATION

    // BEGIN SOCKS CONFIGURATION
    {
        AppConfig.inboundConfig->HasSOCKS.ReadWriteBind(socksGroupBox, "checked", &QGroupBox::toggled);
        AppConfig.inboundConfig->SOCKSConfig->ListenPort.ReadWriteBind(socksPortLE, "value", &QSpinBox::valueChanged);
        // Socks UDP Options
        AppConfig.inboundConfig->SOCKSConfig->EnableUDP.ReadWriteBind(socksUDPCB, "checked", &QCheckBox::stateChanged);
        AppConfig.inboundConfig->SOCKSConfig->UDPLocalAddress.ReadWriteBind(socksUDPIP, "text", &QLineEdit::textEdited);

#pragma message("TODO: ?")
        //    socksUDPIP->setEnabled(has_socks && SOCKSConfig->enableUDP);

        AppConfig.inboundConfig->SOCKSConfig->Sniffing.Observe(
            [this](auto newVal)
            {
#pragma message("TODO: Should not update the CB")
                //            httpSniffingCB->setCurrentIndex(newVal);
                const bool hasSocks = AppConfig.inboundConfig->HasSOCKS;
                const bool isSniffingOn = newVal != ProtocolInboundBase::SNIFFING_OFF;

                // Disable these fields when HTTP is not enabled & sniffing is turned off.
                socksOverrideHTTPCB->setEnabled(hasSocks && isSniffingOn);
                socksOverrideTLSCB->setEnabled(hasSocks && isSniffingOn);
                socksOverrideFakeDNSCB->setEnabled(hasSocks && isSniffingOn);
            });

        AppConfig.inboundConfig->SOCKSConfig->DestinationOverride.Observe(
            [this](auto newVal)
            {
                socksOverrideHTTPCB->setChecked(newVal.contains("http"));
                socksOverrideTLSCB->setChecked(newVal.contains("tls"));
                socksOverrideFakeDNSCB->setChecked(newVal.contains("fakedns"));
            });
    }
    // END OF SOCKS CONFIGURATION

    // BEGIN DOKODEMO_DOOR CONFIGURATIOIN
    {
        const bool has_tproxy = AppConfig.inboundConfig->HasDokodemoDoor;
        tproxyGroupBox->setChecked(has_tproxy);
        tProxyPort->setValue(AppConfig.inboundConfig->DokodemoDoorConfig->ListenPort);

        AppConfig.inboundConfig->DokodemoDoorConfig->Sniffing.Observe(
            [this](auto newVal)
            {
#pragma message("TODO: Should not update the CB")
                //            httpSniffingCB->setCurrentIndex(newVal);
                const bool hasSocks = AppConfig.inboundConfig->HasDokodemoDoor;
                const bool isSniffingOn = newVal != ProtocolInboundBase::SNIFFING_OFF;

                // Disable these fields when HTTP is not enabled & sniffing is turned off.
                tproxyOverrideHTTPCB->setEnabled(hasSocks && isSniffingOn);
                tproxyOverrideTLSCB->setEnabled(hasSocks && isSniffingOn);
                tproxyOverrideFakeDNSCB->setEnabled(hasSocks && isSniffingOn);
            });

        AppConfig.inboundConfig->DokodemoDoorConfig->DestinationOverride.Observe(
            [this](auto newVal)
            {
                tproxyOverrideHTTPCB->setChecked(newVal.contains("http"));
                tproxyOverrideTLSCB->setChecked(newVal.contains("tls"));
                tproxyOverrideFakeDNSCB->setChecked(newVal.contains("fakedns"));
            });

        AppConfig.inboundConfig->DokodemoDoorConfig->WorkingMode.ReadWriteBind(tproxyMode, "currentIndex", &QComboBox::currentIndexChanged);
    }
    // END OF DOKODEMO_DOOR CONFIGURATION

    AppConfig.connectionConfig->OutboundMark.ReadWriteBind(outboundMark, "value", &QSpinBox::valueChanged);
    //
#pragma message("TODO: Move To V2Ray Kernel Plugin")
    //    dnsIntercept->setChecked(AppConfig.connectionConfig->dnsIntercept);
    //    dnsFreedomCb->setChecked(AppConfig.connectionConfig->v2rayFreedomDNS);
    //
    // Kernel Settings
    {
        //        vCorePathTxt->setText(AppConfig.kernelConfig->KernelPath());
        //        vCoreAssetsPathTxt->setText(AppConfig.kernelConfig->AssetsPath());
        //        enableAPI->setChecked(AppConfig.kernelConfig->enableAPI);
        //        statsPortBox->setValue(AppConfig.kernelConfig->statsPort);
        //
        //

        pluginKernelPortAllocateCB->setValue(BaselibConfig.plugin_config.plugin_port_allocation);
        //        pluginKernelPortAllocateCB->setEnabled(AppConfig.pluginConfig->v2rayIntegration);
    }
    // Connection Settings
    {
        AppConfig.connectionConfig->BypassBittorrent.ReadWriteBind(bypassBTCb, "checked", &QCheckBox::stateChanged);
        AppConfig.connectionConfig->ForceDirectConnection.ReadWriteBind(forceDirectConnectionCB, "checked", &QCheckBox::stateChanged);
    }
    //
    {
        qvProxyPortCB->setValue(BaselibConfig.network_config.port);
        qvProxyAddressTxt->setText(BaselibConfig.network_config.address);
        qvNetworkUATxt->setEditText(BaselibConfig.network_config.ua);
        //
        SET_PROXY_UI_ENABLE(BaselibConfig.network_config.type == Qv2rayBase::Models::NetworkProxyConfig::PROXY_HTTP ||
                            BaselibConfig.network_config.type == Qv2rayBase::Models::NetworkProxyConfig::PROXY_SOCKS5)
    }
    //
    //
    {
        dnsSettingsWidget = new DnsSettingsWidget(this);
        dnsSettingsWidget->SetDNSObject(AppConfig.connectionConfig->DNSConfig, AppConfig.connectionConfig->FakeDNSConfig);
        dnsSettingsLayout->addWidget(dnsSettingsWidget);
        //
        routeSettingsWidget = new RouteSettingsMatrixWidget(this);
        routeSettingsWidget->SetRouteConfig(AppConfig.connectionConfig->RouteConfig);
        advRouteSettingsLayout->addWidget(routeSettingsWidget);
    }
    //
#ifdef DISABLE_AUTO_UPDATE
    updateSettingsGroupBox->setEnabled(false);
    updateSettingsGroupBox->setToolTip(tr("Update is disabled by your vendor."));
#endif

#pragma message("TODO")
    //    updateChannelCombo->setCurrentIndex(AppConfig.updateConfig->updateChannel);
    //    cancelIgnoreVersionBtn->setEnabled(!AppConfig.updateConfig->ignoredVersion->isEmpty());
    //    ignoredNextVersion->setText(AppConfig.updateConfig->ignoredVersion);

    {
        noAutoConnectRB->setChecked(AppConfig.behaviorConfig->AutoConnectBehavior == Qv2rayBehaviorConfig::AUTOCONNECT_NONE);
        lastConnectedRB->setChecked(AppConfig.behaviorConfig->AutoConnectBehavior == Qv2rayBehaviorConfig::AUTOCONNECT_LAST_CONNECTED);
        fixedAutoConnectRB->setChecked(AppConfig.behaviorConfig->AutoConnectBehavior == Qv2rayBehaviorConfig::AUTOCONNECT_FIXED);

        SET_AUTOSTART_UI_ENABLED(AppConfig.behaviorConfig->AutoConnectBehavior == Qv2rayBehaviorConfig::AUTOCONNECT_FIXED);

        if (AppConfig.behaviorConfig->AutoConnectProfileId->isNull())
            AppConfig.behaviorConfig->AutoConnectProfileId->groupId = DefaultGroupId;

        const auto &autoStartConnId = AppConfig.behaviorConfig->AutoConnectProfileId->connectionId;
        const auto &autoStartGroupId = AppConfig.behaviorConfig->AutoConnectProfileId->groupId;
        //
        for (const auto &group : QvBaselib->ProfileManager()->GetGroups()) //
            autoStartSubsCombo->addItem(GetDisplayName(group), group.toString());

        autoStartSubsCombo->setCurrentText(GetDisplayName(autoStartGroupId));

        for (const auto &conn : QvBaselib->ProfileManager()->GetConnections(autoStartGroupId))
            autoStartConnCombo->addItem(GetDisplayName(conn), conn.toString());

        autoStartConnCombo->setCurrentText(GetDisplayName(autoStartConnId));
    }
    //
    maxLogLinesSB->setValue(AppConfig.appearanceConfig->MaximizeLogLines);
    jumpListCountSB->setValue(AppConfig.appearanceConfig->RecentJumpListSize);
    //
#pragma message("TODO: To Command Plugin")
    //    setSysProxyCB->setChecked(AppConfig.inboundConfig->systemProxySettings->setSystemProxy);
    //
    finishedLoading = true;
}

QvMessageBusSlotImpl(PreferencesWindow)
{
    switch (msg)
    {
        MBShowDefaultImpl;
        MBHideDefaultImpl;
        MBRetranslateDefaultImpl;
        case UPDATE_COLORSCHEME: break;
    }
}

PreferencesWindow::~PreferencesWindow(){};

void PreferencesWindow::on_buttonBox_accepted()
{
    // Note:
    // A signal-slot connection from buttonbox_accpted to QDialog::accepted()
    // has been removed. To prevent closing this Dialog.
    QSet<int> ports;
    auto size = 0;

    if (AppConfig.inboundConfig->HasHTTP)
    {
        size++;
        ports << AppConfig.inboundConfig->HTTPConfig->ListenPort;
    }

    if (AppConfig.inboundConfig->HasSOCKS)
    {
        size++;
        ports << AppConfig.inboundConfig->SOCKSConfig->ListenPort;
    }

    if (AppConfig.inboundConfig->HasDokodemoDoor)
    {
        size++;
        ports << AppConfig.inboundConfig->DokodemoDoorConfig->ListenPort;
    }

    if (ports.size() != size)
    {
        // Duplicates detected.
        QvBaselib->Warn(tr("Preferences"), tr("Duplicated port numbers detected, please check the port number Settings->"));
        return;
    }

    if (!IsValidIPAddress(AppConfig.inboundConfig->ListenAddress))
    {
        QvBaselib->Warn(tr("Preferences"), tr("Invalid inbound listening address."));
        return;
    }

    if (!dnsSettingsWidget->CheckIsValidDNS())
    {
        QvBaselib->Warn(tr("Preferences"), tr("Invalid DNS Settings."));
        return;
    }

    AppConfig.connectionConfig->RouteConfig = routeSettingsWidget->GetRouteConfig();
    if (!(AppConfig.connectionConfig->RouteConfig == GlobalConfig->connectionConfig->RouteConfig))
    {
        NEEDRESTART
    }

    const auto &[dns, fakedns] = dnsSettingsWidget->GetDNSObject();
    AppConfig.connectionConfig->DNSConfig = dns;
    AppConfig.connectionConfig->FakeDNSConfig = fakedns;

    if (!(AppConfig.connectionConfig->DNSConfig == GlobalConfig->connectionConfig->DNSConfig))
    {
        NEEDRESTART
    }

    if (!(AppConfig.connectionConfig->FakeDNSConfig == GlobalConfig->connectionConfig->FakeDNSConfig))
    {
        NEEDRESTART
    }

    if (AppConfig.appearanceConfig->UITheme != GlobalConfig->appearanceConfig->UITheme)
    {
        StyleManager->ApplyStyle(AppConfig.appearanceConfig->UITheme);
    }

    GlobalConfig->loadJson(AppConfig.toJson());
    QvBaselib->GetConfig()->loadJson(BaselibConfig.toJson());

#pragma message("TODO")
    //    SaveGlobalSettings();

    UIMessageBus.EmitGlobalSignal(QvMBMessage::UPDATE_COLORSCHEME);

#pragma message("TODO: should or not")
    //    if (NeedRestart && !KernelInstance->CurrentConnection().isEmpty())
    //    {
    //        const auto message = tr("You may need to reconnect to apply the settings now.") + NEWLINE +              //
    //                             tr("Otherwise they will be applied next time you connect to a server.") + NEWLINE + //
    //                             NEWLINE +                                                                           //
    //                             tr("Do you want to reconnect now?");
    //        const auto askResult = QvBaselib->Ask(tr("Reconnect Required"), message);
    //        if (askResult == Qv2rayBase::MessageOpt::Yes)
    //        {
    //            QvBaselib->ProfileManager()->RestartConnection();
    //            this->setEnabled(false);
    //        }
    //    }
    accept();
}

void PreferencesWindow::on_logLevelComboBox_currentIndexChanged(int index)
{
    NEEDRESTART
#pragma message("TODO: To Kernel Plugin")
    //    AppConfig.logLevel = index;
}

void PreferencesWindow::on_vCoreAssetsPathTxt_textEdited(const QString &arg1)
{
    NEEDRESTART
#pragma message("TODO: To Kernel Plugin")
    //    AppConfig.kernelConfig->AssetsPath(arg1);
}

void PreferencesWindow::on_listenIPTxt_textEdited(const QString &arg1)
{
    NEEDRESTART
#pragma message("TODO: IPv6")
    AppConfig.inboundConfig->ListenAddress = arg1;

    if (arg1 == "" || IsValidIPAddress(arg1))
    {
        BLACK(listenIPTxt);
    }
    else
    {
        RED(listenIPTxt);
    }

    // pacAccessPathTxt->setText("http://" + arg1 + ":" +
    // QString::number(pacPortSB->value()) + "/pac");
}
void PreferencesWindow::on_proxyDefaultCb_stateChanged(int arg1)
{
    NEEDRESTART
    AppConfig.connectionConfig->ForceDirectConnection = arg1 == Qt::Checked;
}

void PreferencesWindow::on_selectVAssetBtn_clicked()
{
    NEEDRESTART
    QString dir = QFileDialog::getExistingDirectory(this, tr("Open V2Ray assets folder"), QDir::currentPath());

    if (!dir.isEmpty())
    {
        vCoreAssetsPathTxt->setText(dir);
        on_vCoreAssetsPathTxt_textEdited(dir);
    }
}

void PreferencesWindow::on_selectVCoreBtn_clicked()
{
    const auto corePath = QFileDialog::getOpenFileName(this, tr("Open V2Ray core file"), QDir::currentPath());

    if (!corePath.isEmpty())
    {
        vCorePathTxt->setText(corePath);
        on_vCorePathTxt_textEdited(corePath);
    }
}

void PreferencesWindow::on_vCorePathTxt_textEdited(const QString &arg1)
{
    NEEDRESTART
#pragma message("TODO: To Kernel Plugin")
    //    AppConfig.kernelConfig->KernelPath(arg1);
}

void PreferencesWindow::on_aboutQt_clicked()
{
    QApplication::aboutQt();
}

void PreferencesWindow::on_cancelIgnoreVersionBtn_clicked()
{
#pragma message("TODO: ?")
    //    AppConfig.updateConfig->ignoredVersion->clear();
    //    cancelIgnoreVersionBtn->setEnabled(false);
}

void PreferencesWindow::on_bypassBTCb_stateChanged(int arg1)
{
    NEEDRESTART
    if (arg1 == Qt::Checked)
    {
        QvBaselib->Info(tr("Note"), tr("To recognize the protocol of a connection, one must enable sniffing option in inbound proxy.") + NEWLINE +
                                        tr("tproxy inbound's sniffing is enabled by default."));
    }
    AppConfig.connectionConfig->BypassBittorrent = arg1 == Qt::Checked;
}

void PreferencesWindow::on_statsPortBox_valueChanged(int arg1)
{
    NEEDRESTART
#pragma message("TODO: To Kernel Plugin")
    //    AppConfig.kernelConfig->statsPort = arg1;
}

void PreferencesWindow::on_socksPortLE_valueChanged(int arg1)
{
    NEEDRESTART
    AppConfig.inboundConfig->SOCKSConfig->ListenPort = arg1;
}

void PreferencesWindow::on_httpPortLE_valueChanged(int arg1)
{
    NEEDRESTART
    AppConfig.inboundConfig->HTTPConfig->ListenPort = arg1;
}

void PreferencesWindow::on_socksUDPCB_stateChanged(int arg1)
{
    NEEDRESTART
    AppConfig.inboundConfig->SOCKSConfig->EnableUDP = arg1 == Qt::Checked;
    socksUDPIP->setEnabled(arg1 == Qt::Checked);
}

void PreferencesWindow::on_socksUDPIP_textEdited(const QString &arg1)
{
    NEEDRESTART
    AppConfig.inboundConfig->SOCKSConfig->UDPLocalAddress = arg1;

    if (IsValidIPAddress(arg1))
    {
        BLACK(socksUDPIP);
    }
    else
    {
        RED(socksUDPIP);
    }
}

void PreferencesWindow::on_autoStartSubsCombo_currentIndexChanged(int arg1)
{
    LOADINGCHECK
    if (arg1 == -1)
    {
        AppConfig.behaviorConfig->AutoConnectProfileId->clear();
        autoStartConnCombo->clear();
    }
    else
    {
        auto list = QvBaselib->ProfileManager()->GetConnections(GroupId(autoStartSubsCombo->currentData().toString()));
        autoStartConnCombo->clear();

        for (const auto &id : list)
        {
            autoStartConnCombo->addItem(GetDisplayName(id), id.toString());
        }
    }
}

void PreferencesWindow::on_autoStartConnCombo_currentIndexChanged(int arg1)
{
    LOADINGCHECK
    if (arg1 == -1)
    {
        AppConfig.behaviorConfig->AutoConnectProfileId->clear();
    }
    else
    {
        AppConfig.behaviorConfig->AutoConnectProfileId->groupId = GroupId(autoStartSubsCombo->currentData().toString());
        AppConfig.behaviorConfig->AutoConnectProfileId->connectionId = ConnectionId(autoStartConnCombo->currentData().toString());
    }
}

void PreferencesWindow::on_startWithLoginCB_stateChanged(int arg1)
{
    bool isEnabled = arg1 == Qt::Checked;
    SetLaunchAtLoginStatus(isEnabled);

    if (GetLaunchAtLoginStatus() != isEnabled)
    {
        QvBaselib->Warn(tr("Start with boot"), tr("Failed to set auto start option."));
    }

    SetAutoStartButtonsState(GetLaunchAtLoginStatus());
}

void PreferencesWindow::SetAutoStartButtonsState(bool isAutoStart)
{
    startWithLoginCB->setChecked(isAutoStart);
}

void PreferencesWindow::on_checkVCoreSettings_clicked()
{
    auto vcorePath = vCorePathTxt->text();
    auto vAssetsPath = vCoreAssetsPathTxt->text();

#pragma message("TODO, move to plugin")
    //    if (const auto &&[result, msg] = V2RayKernelInstance::ValidateKernel(vcorePath, vAssetsPath); !result)
    //    {
    //        QvBaselib->Warn(tr("V2Ray Core Settings"), *msg);
    //    }
    //    else
    //    {
    //        const auto content = tr("V2Ray path configuration check passed.") + NEWLINE + NEWLINE + tr("Current version of V2Ray is: ") + NEWLINE + *msg;
    //        QvMessageBoxInfo(this, tr("V2Ray Core Settings"), content);
    //    }
}

void PreferencesWindow::on_maxLogLinesSB_valueChanged(int arg1)
{
    LOADINGCHECK
    NEEDRESTART
    AppConfig.appearanceConfig->MaximizeLogLines = arg1;
}

void PreferencesWindow::on_enableAPI_stateChanged(int arg1)
{
    LOADINGCHECK
    NEEDRESTART
#pragma message("TODO: To Kernel Plugin")
    //    AppConfig.kernelConfig->enableAPI = arg1 == Qt::Checked;
}

void PreferencesWindow::on_updateChannelCombo_currentIndexChanged(int index)
{
    LOADINGCHECK
    //    AppConfig.updateConfig->updateChannel = (Qv2rayUpdateChannel) index;
    //    AppConfig.updateConfig->ignoredVersion->clear();
}

void PreferencesWindow::on_pluginKernelPortAllocateCB_valueChanged(int arg1)
{
    LOADINGCHECK
#pragma message("TODO: To Kernel Plugin")
    //    if (KernelInstance->ActivePluginKernelsCount() > 0)
    //        NEEDRESTART;
    //    AppConfig.pluginConfig->portAllocationStart = arg1;
}

void PreferencesWindow::on_qvProxyAddressTxt_textEdited(const QString &arg1)
{
    LOADINGCHECK
    BaselibConfig.network_config.address = arg1;
}

void PreferencesWindow::on_qvProxyTypeCombo_currentTextChanged(const QString &arg1)
{
    LOADINGCHECK
#pragma message("TODO: Change to combobox")
    //    AppConfig.networkConfig->type = arg1.toLower();
}

void PreferencesWindow::on_qvProxyPortCB_valueChanged(int arg1)
{
    LOADINGCHECK
    BaselibConfig.network_config.port = arg1;
}

void PreferencesWindow::on_tProxyPort_valueChanged(int arg1)
{
    LOADINGCHECK
    NEEDRESTART
    AppConfig.inboundConfig->DokodemoDoorConfig->ListenPort = arg1;
}

void PreferencesWindow::on_tproxyOverrideHTTPCB_stateChanged(int arg1)
{
    NEEDRESTART
    if (arg1 != Qt::Checked)
        AppConfig.inboundConfig->DokodemoDoorConfig->DestinationOverride->removeAll("http");
    else if (!AppConfig.inboundConfig->DokodemoDoorConfig->DestinationOverride->contains("http"))
        AppConfig.inboundConfig->DokodemoDoorConfig->DestinationOverride->append("http");
}

void PreferencesWindow::on_tproxyOverrideTLSCB_stateChanged(int arg1)
{
    NEEDRESTART
    if (arg1 != Qt::Checked)
        AppConfig.inboundConfig->DokodemoDoorConfig->DestinationOverride->removeAll("tls");
    else if (!AppConfig.inboundConfig->DokodemoDoorConfig->DestinationOverride->contains("tls"))
        AppConfig.inboundConfig->DokodemoDoorConfig->DestinationOverride->append("tls");
}

void PreferencesWindow::on_tproxyMode_currentTextChanged(const QString &arg1)
{
    NEEDRESTART
#pragma message("TODO: to combobox")
    //    AppConfig.inboundConfig->DokodemoDoorConfig->WorkingMode = arg1;
}

void PreferencesWindow::on_jumpListCountSB_valueChanged(int arg1)
{
    LOADINGCHECK
    AppConfig.appearanceConfig->RecentJumpListSize = arg1;
}

void PreferencesWindow::on_outboundMark_valueChanged(int arg1)
{
    LOADINGCHECK
    NEEDRESTART
    AppConfig.connectionConfig->OutboundMark = arg1;
}

void PreferencesWindow::on_dnsIntercept_toggled(bool checked)
{
    LOADINGCHECK
    NEEDRESTART
#pragma message("TODO: To V2Ray Kernel Plugin")
    //    AppConfig.connectionConfig->dnsIntercept = checked;
}

void PreferencesWindow::on_qvProxyCustomProxy_clicked()
{
#pragma message("TODO: HTTP & SOCKS")
    //    BaselibConfig.network_config.type = Qv2rayBase::Models::NetworkProxyConfig::PROXY_HTTP;
}

void PreferencesWindow::on_qvProxySystemProxy_clicked()
{
    BaselibConfig.network_config.type = Qv2rayBase::Models::NetworkProxyConfig::PROXY_SYSTEM;
}

void PreferencesWindow::on_qvProxyNoProxy_clicked()
{
    BaselibConfig.network_config.type = Qv2rayBase::Models::NetworkProxyConfig::PROXY_NONE;
}

void PreferencesWindow::on_dnsFreedomCb_stateChanged(int arg1)
{
    LOADINGCHECK
    NEEDRESTART
#pragma message("TODO: To V2Ray Kernel Plugin")
    //    AppConfig.connectionConfig->v2rayFreedomDNS = arg1 == Qt::Checked;
}

void PreferencesWindow::on_httpOverrideHTTPCB_stateChanged(int arg1)
{
    LOADINGCHECK
    NEEDRESTART
    if (arg1 != Qt::Checked)
        AppConfig.inboundConfig->HTTPConfig->DestinationOverride->removeAll("http");
    else if (!AppConfig.inboundConfig->HTTPConfig->DestinationOverride->contains("http"))
        AppConfig.inboundConfig->HTTPConfig->DestinationOverride->append("http");
}

void PreferencesWindow::on_httpOverrideTLSCB_stateChanged(int arg1)
{
    LOADINGCHECK
    NEEDRESTART
    if (arg1 != Qt::Checked)
        AppConfig.inboundConfig->HTTPConfig->DestinationOverride->removeAll("tls");
    else if (!AppConfig.inboundConfig->HTTPConfig->DestinationOverride->contains("tls"))
        AppConfig.inboundConfig->HTTPConfig->DestinationOverride->append("tls");
}

void PreferencesWindow::on_socksOverrideHTTPCB_stateChanged(int arg1)
{
    LOADINGCHECK
    NEEDRESTART
    if (arg1 != Qt::Checked)
        AppConfig.inboundConfig->SOCKSConfig->DestinationOverride->removeAll("http");
    else if (!AppConfig.inboundConfig->SOCKSConfig->DestinationOverride->contains("http"))
        AppConfig.inboundConfig->SOCKSConfig->DestinationOverride->append("http");
}

void PreferencesWindow::on_socksOverrideTLSCB_stateChanged(int arg1)
{
    LOADINGCHECK
    NEEDRESTART
    if (arg1 != Qt::Checked)
        AppConfig.inboundConfig->SOCKSConfig->DestinationOverride->removeAll("tls");
    else if (!AppConfig.inboundConfig->SOCKSConfig->DestinationOverride->contains("tls"))
        AppConfig.inboundConfig->SOCKSConfig->DestinationOverride->append("tls");
}

void PreferencesWindow::on_noAutoConnectRB_clicked()
{
    LOADINGCHECK
    AppConfig.behaviorConfig->AutoConnectBehavior = Qv2rayBehaviorConfig::AUTOCONNECT_NONE;
    SET_AUTOSTART_UI_ENABLED(false);
}

void PreferencesWindow::on_lastConnectedRB_clicked()
{
    LOADINGCHECK
    AppConfig.behaviorConfig->AutoConnectBehavior = Qv2rayBehaviorConfig::AUTOCONNECT_LAST_CONNECTED;
    SET_AUTOSTART_UI_ENABLED(false);
}

void PreferencesWindow::on_fixedAutoConnectRB_clicked()
{
    LOADINGCHECK
    AppConfig.behaviorConfig->AutoConnectBehavior = Qv2rayBehaviorConfig::AUTOCONNECT_FIXED;
    SET_AUTOSTART_UI_ENABLED(true);
}

void PreferencesWindow::on_qvNetworkUATxt_editTextChanged(const QString &arg1)
{
    LOADINGCHECK
    BaselibConfig.network_config.ua = arg1;
}

void PreferencesWindow::on_openConfigDirCB_clicked()
{
    QvBaselib->OpenURL({ "QV2RAY_CONFIG_DIR" });
}

void PreferencesWindow::on_socksOverrideFakeDNSCB_stateChanged(int arg1)
{
    NEEDRESTART
    if (arg1 != Qt::Checked)
        AppConfig.inboundConfig->SOCKSConfig->DestinationOverride->removeAll("fakedns");
    else if (!AppConfig.inboundConfig->SOCKSConfig->DestinationOverride->contains("fakedns"))
        AppConfig.inboundConfig->SOCKSConfig->DestinationOverride->append("fakedns");
}

void PreferencesWindow::on_httpOverrideFakeDNSCB_stateChanged(int arg1)
{
    NEEDRESTART
    if (arg1 != Qt::Checked)
        AppConfig.inboundConfig->HTTPConfig->DestinationOverride->removeAll("fakedns");
    else if (!AppConfig.inboundConfig->HTTPConfig->DestinationOverride->contains("fakedns"))
        AppConfig.inboundConfig->HTTPConfig->DestinationOverride->append("fakedns");
}

void PreferencesWindow::on_tproxyOverrideFakeDNSCB_stateChanged(int arg1)
{
    NEEDRESTART
    if (arg1 != Qt::Checked)
        AppConfig.inboundConfig->DokodemoDoorConfig->DestinationOverride->removeAll("fakedns");
    else if (!AppConfig.inboundConfig->DokodemoDoorConfig->DestinationOverride->contains("fakedns"))
        AppConfig.inboundConfig->DokodemoDoorConfig->DestinationOverride->append("fakedns");
}

void PreferencesWindow::on_httpSniffingCB_currentIndexChanged(int index)
{
    AppConfig.inboundConfig->HTTPConfig->Sniffing = (ProtocolInboundBase::SniffingBehavior) index;
}

void PreferencesWindow::on_dokoSniffingCB_currentIndexChanged(int index)
{
    AppConfig.inboundConfig->DokodemoDoorConfig->Sniffing = (ProtocolInboundBase::SniffingBehavior) index;
}

void PreferencesWindow::on_socksSniffingCB_currentIndexChanged(int index)
{
    AppConfig.inboundConfig->SOCKSConfig->Sniffing = (ProtocolInboundBase::SniffingBehavior) index;
}
