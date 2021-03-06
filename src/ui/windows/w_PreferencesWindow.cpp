#include "w_PreferencesWindow.hpp"

#include "AutoLaunchHelper/AutoLaunchHelper.hpp"
#include "Qv2rayBase/Common/ProfileHelpers.hpp"
#include "Qv2rayBase/Common/Utils.hpp"
#include "Qv2rayBase/Interfaces/IStorageProvider.hpp"
#include "Qv2rayBase/Profile/ProfileManager.hpp"
#include "QvPlugin/PluginInterface.hpp"
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

#define NEEDRESTART NeedRestart = true;

#define SET_PROXY_UI_ENABLE(_enabled)                                                                                                                                    \
    qvProxyAddressTxt->setEnabled(_enabled);                                                                                                                             \
    qvProxyPortCB->setEnabled(_enabled);

PreferencesWindow::PreferencesWindow(QWidget *parent) : QvDialog(QStringLiteral("PreferenceWindow"), parent), AppConfig()
{
    setupUi(this);
    QvMessageBusConnect();
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

    // We add locales
    {
        languageComboBox->setDisabled(true);
        // Since we can't have languages detected. It worths nothing to translate these.
        languageComboBox->setToolTip(QStringLiteral("Cannot find any language providers."));
    }

    // Set auto start button state
    SetAutoStartButtonsState(GetLaunchAtLoginStatus());
    themeCombo->addItems(StyleManager->AllStyles());
    //
    // Deep copy
    AppConfig.loadJson(GlobalConfig->toJson());
    BaselibConfig.loadJson(QvBaselib->GetConfig()->toJson());

    AppConfig.appearanceConfig->MaximizeLogLines.ReadWriteBind(maxLogLinesSB, "value", &QSpinBox::valueChanged);
    AppConfig.appearanceConfig->RecentJumpListSize.ReadWriteBind(jumpListCountSB, "value", &QSpinBox::valueChanged);
    AppConfig.appearanceConfig->UITheme.ReadWriteBind(themeCombo, "currentText", &QComboBox::currentIndexChanged);
    AppConfig.appearanceConfig->DarkModeTrayIcon.ReadWriteBind(darkTrayCB, "checked", &QCheckBox::stateChanged);
    AppConfig.appearanceConfig->Language.ReadWriteBind(languageComboBox, "currentText", &QComboBox::currentIndexChanged);
    AppConfig.behaviorConfig->QuietMode.ReadWriteBind(quietModeCB, "checked", &QCheckBox::stateChanged);

    AppConfig.inboundConfig->ListenAddress.ReadWriteBind(listenIPTxt, "text", &QLineEdit::textEdited);
    AppConfig.inboundConfig->ListenAddressV6.ReadWriteBind(listenIPv6Txt, "text", &QLineEdit::textEdited);

    AppConfig.connectionConfig->BypassLAN.ReadWriteBind(bypassLANCB, "checked", &QCheckBox::toggled);
    AppConfig.connectionConfig->BypassCN.ReadWriteBind(bypassCNCB, "checked", &QCheckBox::toggled);
    AppConfig.connectionConfig->DNSInterception.ReadWriteBind(dnsInterceptCB, "checked", &QCheckBox::toggled);

    pluginKernelPortAllocateCB->setValue(BaselibConfig.plugin_config.plugin_port_allocation);

    // BEGIN HTTP CONFIGURATION
    {
        AppConfig.inboundConfig->HasHTTP.ReadWriteBind(httpGroupBox, "checked", &QGroupBox::toggled);
        AppConfig.inboundConfig->HTTPConfig->ListenPort.ReadWriteBind(httpPortLE, "value", &QSpinBox::valueChanged);
        AppConfig.inboundConfig->HTTPConfig->Sniffing.ReadWriteBind(httpSniffingCB, "currentIndex", &QComboBox::currentIndexChanged);
        AppConfig.inboundConfig->HTTPConfig->Sniffing.Observe([this](auto newVal) {
            const bool hasHttp = AppConfig.inboundConfig->HasHTTP;
            httpOverrideHTTPCB->setEnabled(hasHttp && newVal == ProtocolInboundBase::SNIFFING_FULL);
            httpOverrideTLSCB->setEnabled(hasHttp && newVal == ProtocolInboundBase::SNIFFING_FULL);
            httpOverrideFakeDNSCB->setEnabled(hasHttp && newVal != ProtocolInboundBase::SNIFFING_OFF);
        });

        AppConfig.inboundConfig->HTTPConfig->DestinationOverride.Observe([this](auto newVal) {
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
        AppConfig.inboundConfig->SOCKSConfig->EnableUDP.Observe([this](const auto &newval) { socksUDPIP->setEnabled(newval); });
        AppConfig.inboundConfig->SOCKSConfig->EnableUDP.ReadWriteBind(socksUDPCB, "checked", &QCheckBox::stateChanged);
        AppConfig.inboundConfig->SOCKSConfig->UDPLocalAddress.ReadWriteBind(socksUDPIP, "text", &QLineEdit::textEdited);

        AppConfig.inboundConfig->SOCKSConfig->Sniffing.ReadWriteBind(socksSniffingCB, "currentIndex", &QComboBox::currentIndexChanged);
        AppConfig.inboundConfig->SOCKSConfig->Sniffing.Observe([this](auto newVal) {
            const bool hasSocks = AppConfig.inboundConfig->HasSOCKS;
            socksOverrideHTTPCB->setEnabled(hasSocks && newVal == ProtocolInboundBase::SNIFFING_FULL);
            socksOverrideTLSCB->setEnabled(hasSocks && newVal == ProtocolInboundBase::SNIFFING_FULL);
            socksOverrideFakeDNSCB->setEnabled(hasSocks && newVal != ProtocolInboundBase::SNIFFING_OFF);
        });

        AppConfig.inboundConfig->SOCKSConfig->DestinationOverride.Observe([this](auto newVal) {
            socksOverrideHTTPCB->setChecked(newVal.contains("http"));
            socksOverrideTLSCB->setChecked(newVal.contains("tls"));
            socksOverrideFakeDNSCB->setChecked(newVal.contains("fakedns"));
        });
    }
    // END OF SOCKS CONFIGURATION

    // BEGIN DOKODEMO_DOOR CONFIGURATIOIN
    {
        AppConfig.inboundConfig->HasDokodemoDoor.ReadWriteBind(tproxyGroupBox, "checked", &QGroupBox::toggled);
        AppConfig.inboundConfig->DokodemoDoorConfig->ListenPort.ReadWriteBind(tProxyPort, "value", &QSpinBox::valueChanged);

        AppConfig.inboundConfig->DokodemoDoorConfig->Sniffing.ReadWriteBind(dokoSniffingCB, "currentIndex", &QComboBox::currentIndexChanged);
        AppConfig.inboundConfig->DokodemoDoorConfig->Sniffing.Observe([this](auto newVal) {
            const bool hasSocks = AppConfig.inboundConfig->HasDokodemoDoor;
            tproxyOverrideHTTPCB->setEnabled(hasSocks && newVal == ProtocolInboundBase::SNIFFING_FULL);
            tproxyOverrideTLSCB->setEnabled(hasSocks && newVal == ProtocolInboundBase::SNIFFING_FULL);
            tproxyOverrideFakeDNSCB->setEnabled(hasSocks && newVal != ProtocolInboundBase::SNIFFING_OFF);
        });

        AppConfig.inboundConfig->DokodemoDoorConfig->DestinationOverride.Observe([this](auto val) {
            tproxyOverrideHTTPCB->setChecked(val.contains("http"));
            tproxyOverrideTLSCB->setChecked(val.contains("tls"));
            tproxyOverrideFakeDNSCB->setChecked(val.contains("fakedns"));
        });

        AppConfig.inboundConfig->DokodemoDoorConfig->WorkingMode.ReadWriteBind(tproxyModeCB, "currentIndex", &QComboBox::currentIndexChanged);
    }
    // END OF DOKODEMO_DOOR CONFIGURATION

    // Connection Settings
    {
        AppConfig.connectionConfig->BypassBittorrent.Observe([](const auto &newval) {
            if (newval)
                QvBaselib->Info(tr("Note"), tr("To recognize the protocol of a connection, one must enable sniffing option in an inbound proxy."));
        });
        AppConfig.connectionConfig->BypassBittorrent.ReadWriteBind(bypassBTCb, "checked", &QCheckBox::stateChanged);
        AppConfig.connectionConfig->ForceDirectConnection.ReadWriteBind(forceDirectConnectionCB, "checked", &QCheckBox::stateChanged);
    }

    {
        qvProxyPortCB->setValue(BaselibConfig.network_config.port);
        qvProxyAddressTxt->setText(BaselibConfig.network_config.address);
        qvNetworkUATxt->setEditText(BaselibConfig.network_config.ua);
        SET_PROXY_UI_ENABLE(BaselibConfig.network_config.type == Qv2rayBase::Models::NetworkProxyConfig::PROXY_HTTP ||
                            BaselibConfig.network_config.type == Qv2rayBase::Models::NetworkProxyConfig::PROXY_SOCKS5)
    }

    {
        const auto defaultRouteObject = QvBaselib->ProfileManager()->GetRouting();
        dnsSettingsWidget = new DnsSettingsWidget(this);
        dnsSettingsWidget->SetDNSObject(V2RayDNSObject::fromJson(defaultRouteObject.dns), V2RayFakeDNSObject::fromJson(defaultRouteObject.fakedns));
        dnsSettingsLayout->addWidget(dnsSettingsWidget);

        routeSettingsWidget = new RouteSettingsMatrixWidget(this);
        routeSettingsWidget->SetRoute(RouteMatrixConfig::fromJson(defaultRouteObject.extraOptions[RouteMatrixConfig::EXTRA_OPTIONS_ID].toObject()));
        advRouteSettingsLayout->addWidget(routeSettingsWidget);
    }

    {
        AppConfig.behaviorConfig->AutoConnectBehavior.Observe([this](const auto &newval) {
            noAutoConnectRB->setChecked(newval == Qv2rayBehaviorConfig::AUTOCONNECT_NONE);
            lastConnectedRB->setChecked(newval == Qv2rayBehaviorConfig::AUTOCONNECT_LAST_CONNECTED);
            fixedAutoConnectRB->setChecked(newval == Qv2rayBehaviorConfig::AUTOCONNECT_FIXED);
            autoStartConnCombo->setEnabled(newval == Qv2rayBehaviorConfig::AUTOCONNECT_FIXED);
            autoStartSubsCombo->setEnabled(newval == Qv2rayBehaviorConfig::AUTOCONNECT_FIXED);
        });

        if (AppConfig.behaviorConfig->AutoConnectProfileId->isNull())
            AppConfig.behaviorConfig->AutoConnectProfileId->groupId = DefaultGroupId;

        const auto &autoStartConnId = AppConfig.behaviorConfig->AutoConnectProfileId->connectionId;
        const auto &autoStartGroupId = AppConfig.behaviorConfig->AutoConnectProfileId->groupId;

        for (const auto &group : QvBaselib->ProfileManager()->GetGroups())
            autoStartSubsCombo->addItem(GetDisplayName(group), group.toString());

        autoStartSubsCombo->setCurrentText(GetDisplayName(autoStartGroupId));

        for (const auto &conn : QvBaselib->ProfileManager()->GetConnections(autoStartGroupId))
            autoStartConnCombo->addItem(GetDisplayName(conn), conn.toString());

        autoStartConnCombo->setCurrentText(GetDisplayName(autoStartConnId));
    }
}

QvMessageBusSlotImpl(PreferencesWindow)
{
    switch (msg)
    {
        MBShowDefaultImpl;
        MBHideDefaultImpl;
        MBRetranslateDefaultImpl;
        case MessageBus::UPDATE_COLORSCHEME: break;
    }
}

PreferencesWindow::~PreferencesWindow(){};

void PreferencesWindow::on_buttonBox_accepted()
{
    // Note:
    // A signal-slot connection from buttonbox_accepted to QDialog::accepted()
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

    auto defaultRouteObject = QvBaselib->ProfileManager()->GetRouting();
    if (const auto newval = routeSettingsWidget->GetRouteConfig().toJson(); newval != defaultRouteObject.extraOptions[RouteMatrixConfig::EXTRA_OPTIONS_ID].toObject())
    {
        defaultRouteObject.extraOptions.insert(RouteMatrixConfig::EXTRA_OPTIONS_ID, newval);
        NEEDRESTART
    }

    const auto &[dns, fakedns] = dnsSettingsWidget->GetDNSObject();

    if (const auto newval = dns.toJson(); defaultRouteObject.dns != newval)
    {
        defaultRouteObject.dns = newval;
        NEEDRESTART
    }

    if (const auto newval = fakedns.toJson(); defaultRouteObject.fakedns != newval)
    {
        defaultRouteObject.fakedns = newval;
        NEEDRESTART
    }

    if (AppConfig.appearanceConfig->UITheme != GlobalConfig->appearanceConfig->UITheme)
    {
        StyleManager->ApplyStyle(AppConfig.appearanceConfig->UITheme);
    }

    const auto newnew = AppConfig.toJson();
    GlobalConfig->loadJson(newnew);
    QvBaselib->GetConfig()->loadJson(BaselibConfig.toJson());
    accept();
}

void PreferencesWindow::on_aboutQt_clicked()
{
    QApplication::aboutQt();
}

void PreferencesWindow::on_autoStartSubsCombo_currentIndexChanged(int arg1)
{
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

void PreferencesWindow::on_pluginKernelPortAllocateCB_valueChanged(int arg1)
{
    BaselibConfig.plugin_config.plugin_port_allocation = arg1;
}

void PreferencesWindow::on_qvProxyAddressTxt_textEdited(const QString &arg1)
{
    BaselibConfig.network_config.address = arg1;
}

void PreferencesWindow::on_qvProxyPortCB_valueChanged(int arg1)
{
    BaselibConfig.network_config.port = arg1;
}

void PreferencesWindow::on_tproxyOverrideHTTPCB_stateChanged(int arg1)
{
    NEEDRESTART
    if (arg1 != Qt::Checked)
        AppConfig.inboundConfig->DokodemoDoorConfig->DestinationOverride->removeAll("http");
    else if (!AppConfig.inboundConfig->DokodemoDoorConfig->DestinationOverride->contains("http"))
        AppConfig.inboundConfig->DokodemoDoorConfig->DestinationOverride->append(QStringLiteral("http"));
    AppConfig.inboundConfig->DokodemoDoorConfig->DestinationOverride.EmitNotify();
}

void PreferencesWindow::on_tproxyOverrideTLSCB_stateChanged(int arg1)
{
    NEEDRESTART
    if (arg1 != Qt::Checked)
        AppConfig.inboundConfig->DokodemoDoorConfig->DestinationOverride->removeAll("tls");
    else if (!AppConfig.inboundConfig->DokodemoDoorConfig->DestinationOverride->contains("tls"))
        AppConfig.inboundConfig->DokodemoDoorConfig->DestinationOverride->append(QStringLiteral("tls"));
    AppConfig.inboundConfig->DokodemoDoorConfig->DestinationOverride.EmitNotify();
}

void PreferencesWindow::on_tproxyOverrideFakeDNSCB_stateChanged(int arg1)
{
    NEEDRESTART
    if (arg1 != Qt::Checked)
        AppConfig.inboundConfig->DokodemoDoorConfig->DestinationOverride->removeAll("fakedns");
    else if (!AppConfig.inboundConfig->DokodemoDoorConfig->DestinationOverride->contains("fakedns"))
        AppConfig.inboundConfig->DokodemoDoorConfig->DestinationOverride->append(QStringLiteral("fakedns"));
    AppConfig.inboundConfig->DokodemoDoorConfig->DestinationOverride.EmitNotify();
}

void PreferencesWindow::on_httpOverrideHTTPCB_stateChanged(int arg1)
{
    NEEDRESTART
    if (arg1 != Qt::Checked)
        AppConfig.inboundConfig->HTTPConfig->DestinationOverride->removeAll("http");
    else if (!AppConfig.inboundConfig->HTTPConfig->DestinationOverride->contains("http"))
        AppConfig.inboundConfig->HTTPConfig->DestinationOverride->append(QStringLiteral("http"));
    AppConfig.inboundConfig->HTTPConfig->DestinationOverride.EmitNotify();
}

void PreferencesWindow::on_httpOverrideTLSCB_stateChanged(int arg1)
{
    NEEDRESTART
    if (arg1 != Qt::Checked)
        AppConfig.inboundConfig->HTTPConfig->DestinationOverride->removeAll("tls");
    else if (!AppConfig.inboundConfig->HTTPConfig->DestinationOverride->contains("tls"))
        AppConfig.inboundConfig->HTTPConfig->DestinationOverride->append(QStringLiteral("tls"));
    AppConfig.inboundConfig->HTTPConfig->DestinationOverride.EmitNotify();
}

void PreferencesWindow::on_httpOverrideFakeDNSCB_stateChanged(int arg1)
{
    NEEDRESTART
    if (arg1 != Qt::Checked)
        AppConfig.inboundConfig->HTTPConfig->DestinationOverride->removeAll("fakedns");
    else if (!AppConfig.inboundConfig->HTTPConfig->DestinationOverride->contains("fakedns"))
        AppConfig.inboundConfig->HTTPConfig->DestinationOverride->append(QStringLiteral("fakedns"));
    AppConfig.inboundConfig->HTTPConfig->DestinationOverride.EmitNotify();
}

void PreferencesWindow::on_socksOverrideHTTPCB_stateChanged(int arg1)
{
    NEEDRESTART
    if (arg1 != Qt::Checked)
        AppConfig.inboundConfig->SOCKSConfig->DestinationOverride->removeAll("http");
    else if (!AppConfig.inboundConfig->SOCKSConfig->DestinationOverride->contains("http"))
        AppConfig.inboundConfig->SOCKSConfig->DestinationOverride->append(QStringLiteral("http"));
    AppConfig.inboundConfig->SOCKSConfig->DestinationOverride.EmitNotify();
}

void PreferencesWindow::on_socksOverrideTLSCB_stateChanged(int arg1)
{
    NEEDRESTART
    if (arg1 != Qt::Checked)
        AppConfig.inboundConfig->SOCKSConfig->DestinationOverride->removeAll("tls");
    else if (!AppConfig.inboundConfig->SOCKSConfig->DestinationOverride->contains("tls"))
        AppConfig.inboundConfig->SOCKSConfig->DestinationOverride->append(QStringLiteral("tls"));
    AppConfig.inboundConfig->SOCKSConfig->DestinationOverride.EmitNotify();
}

void PreferencesWindow::on_socksOverrideFakeDNSCB_stateChanged(int arg1)
{
    NEEDRESTART
    if (arg1 != Qt::Checked)
        AppConfig.inboundConfig->SOCKSConfig->DestinationOverride->removeAll("fakedns");
    else if (!AppConfig.inboundConfig->SOCKSConfig->DestinationOverride->contains("fakedns"))
        AppConfig.inboundConfig->SOCKSConfig->DestinationOverride->append(QStringLiteral("fakedns"));
    AppConfig.inboundConfig->SOCKSConfig->DestinationOverride.EmitNotify();
}

void PreferencesWindow::on_noAutoConnectRB_clicked()
{
    AppConfig.behaviorConfig->AutoConnectBehavior = Qv2rayBehaviorConfig::AUTOCONNECT_NONE;
}

void PreferencesWindow::on_lastConnectedRB_clicked()
{
    AppConfig.behaviorConfig->AutoConnectBehavior = Qv2rayBehaviorConfig::AUTOCONNECT_LAST_CONNECTED;
}

void PreferencesWindow::on_fixedAutoConnectRB_clicked()
{
    AppConfig.behaviorConfig->AutoConnectBehavior = Qv2rayBehaviorConfig::AUTOCONNECT_FIXED;
}

void PreferencesWindow::on_qvNetworkUATxt_editTextChanged(const QString &arg1)
{
    BaselibConfig.network_config.ua = arg1;
}

void PreferencesWindow::on_qvProxyTypeCombo_currentIndexChanged(int index)
{
    BaselibConfig.network_config.type = (Qv2rayBase::Models::NetworkProxyConfig::ProxyType) index;
    SET_PROXY_UI_ENABLE(BaselibConfig.network_config.type == Qv2rayBase::Models::NetworkProxyConfig::PROXY_HTTP ||
                        BaselibConfig.network_config.type == Qv2rayBase::Models::NetworkProxyConfig::PROXY_SOCKS5)
}
