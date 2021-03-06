#include "ProxyConfigurator/ProxyConfigurator.hpp"
#include "Qv2rayApplication.hpp"
#include "Qv2rayBase/Common/Utils.hpp"
#include "Qv2rayBase/Profile/KernelManager.hpp"
#include "Qv2rayBase/Profile/ProfileManager.hpp"
#include "ui/WidgetUIBase.hpp"
#include "w_MainWindow.hpp"

#ifdef Q_OS_MAC
#include <ApplicationServices/ApplicationServices.h>
#endif

#define QV_MODULE_NAME "MainWindowExtra"

void MainWindow::MWToggleVisibility()
{
    if (isHidden() || isMinimized())
        MWShowWindow();
    else
        MWHideWindow();
}

void MainWindow::MWShowWindow()
{
    this->show();
#ifdef Q_OS_WIN
    setWindowState(Qt::WindowNoState);
    this->activateWindow();
#elif defined(Q_OS_MAC)
    ProcessSerialNumber psn = { 0, kCurrentProcess };
    TransformProcessType(&psn, kProcessTransformToForegroundApplication);
#endif
    tray_action_ToggleVisibility->setText(tr("Hide"));
}

void MainWindow::MWHideWindow()
{
#ifdef Q_OS_MAC
    ProcessSerialNumber psn = { 0, kCurrentProcess };
    TransformProcessType(&psn, kProcessTransformToUIElementApplication);
#endif
    this->hide();
    tray_action_ToggleVisibility->setText(tr("Show"));
}

void MainWindow::MWSetSystemProxy()
{
    const auto inboundInfo = QvBaselib->KernelManager()->GetCurrentConnectionInboundInfo();
    bool httpEnabled = false;
    bool socksEnabled = false;
    int httpPort = 0;
    int socksPort = 0;
    QString httpAddress;
    QString socksAddress;

    for (const auto &[protocol, listenAddr, listenPort] : inboundInfo)
    {
        if (protocol == "http")
        {
            httpEnabled = true;
            httpAddress = listenAddr;
            httpPort = listenPort.from;
        }
        else if (protocol == "socks")
        {
            socksEnabled = true;
            socksAddress = listenAddr;
            socksPort = listenPort.from;
        }
    }

    QString proxyAddress;
    if (httpEnabled)
        proxyAddress = httpAddress;
    else if (socksEnabled)
        proxyAddress = socksAddress;

    const QHostAddress ha(proxyAddress);
    if (ha.isEqual(QHostAddress::AnyIPv4)) // "0.0.0.0"
        proxyAddress = "127.0.0.1";
    else if (ha.isEqual(QHostAddress::AnyIPv6)) // "::"
        proxyAddress = "::1";

    if (!proxyAddress.isEmpty())
    {
        QvLog() << "ProxyAddress:" << proxyAddress;
        QvLog() << "HTTP Port:" << httpPort;
        QvLog() << "SOCKS Port:" << socksPort;
        SetSystemProxy(proxyAddress, httpPort, socksPort);
        qvAppTrayIcon->setIcon(Q_TRAYICON("tray-systemproxy"));
        if (!GlobalConfig->behaviorConfig->QuietMode)
        {
            QvApp->ShowTrayMessage(tr("System proxy configured."));
        }
    }
    else
    {
        QvLog() << "Neither of HTTP nor SOCKS is enabled, cannot set system proxy.";
        QvBaselib->Warn(tr("Cannot set system proxy"), tr("Both HTTP and SOCKS inbounds are not enabled"));
    }
}

void MainWindow::MWClearSystemProxy()
{
    ClearSystemProxy();
    qvAppTrayIcon->setIcon(QvBaselib->KernelManager()->CurrentConnection().isNull() ? Q_TRAYICON("tray") : Q_TRAYICON("tray-connected"));
    if (!GlobalConfig->behaviorConfig->QuietMode)
    {
        QvApp->ShowTrayMessage(tr("System proxy removed."));
    }
}

bool MainWindow::StartAutoConnectionEntry()
{
    if (QvApp->StartupArguments.noAutoConnection)
        return false;
    switch (GlobalConfig->behaviorConfig->AutoConnectBehavior)
    {
        case Qv2rayBehaviorConfig::AUTOCONNECT_NONE: return false;
        case Qv2rayBehaviorConfig::AUTOCONNECT_FIXED: return QvBaselib->ProfileManager()->StartConnection(GlobalConfig->behaviorConfig->AutoConnectProfileId);
        case Qv2rayBehaviorConfig::AUTOCONNECT_LAST_CONNECTED: return QvBaselib->ProfileManager()->StartConnection(GlobalConfig->behaviorConfig->LastConnectedId);
    }
    Q_UNREACHABLE();
}

void MainWindow::CheckSubscriptionsUpdate()
{
    QList<std::pair<QString, GroupId>> updateList;
    QStringList updateNamesList;

    for (const auto &entry : QvBaselib->ProfileManager()->GetGroups())
    {
        const auto info = QvBaselib->ProfileManager()->GetGroupObject(entry);
        if (!info.subscription_config.isSubscription)
            continue;
        //
        // The update is ignored.
        if (info.subscription_config.updateInterval == 0.0f)
            continue;

        if (info.updated + 24h <= system_clock::now())
        {
            updateList << std::pair{ info.name, entry };
            updateNamesList << info.name;
            QvLog() << QString("Subscription update \"%1\": L=%2 R=%3").arg(info.name).arg(TimeToString(info.updated)).arg(info.subscription_config.updateInterval);
        }
    }

    if (!updateList.isEmpty())
    {
        const auto result = GlobalConfig->behaviorConfig->QuietMode ?
                                Qv2rayBase::MessageOpt::Yes :
                                QvBaselib->Ask(tr("Update Subscriptions"),                                  //
                                               tr("Do you want to update these subscriptions?") + NEWLINE + //
                                                   updateNamesList.join(NEWLINE),                           //
                                               { Qv2rayBase::MessageOpt::Yes, Qv2rayBase::MessageOpt::No, Qv2rayBase::MessageOpt::Ignore });

        for (const auto &[name, id] : updateList)
        {
            if (result == Qv2rayBase::MessageOpt::Yes)
            {
                QvLog() << "Updating subscription:" << name;
                QvBaselib->ProfileManager()->UpdateSubscriptionAsync(id);
            }
            else if (result == Qv2rayBase::MessageOpt::Ignore)
            {
                QvLog() << "Ignored subscription update:" << name;
                QvBaselib->ProfileManager()->IgnoreSubscriptionUpdate(id);
            }
        }
    }
}

void MainWindow::updateColorScheme()
{
    vCoreLogHighlighter->loadRules(StyleManager->isDarkMode());
    qvAppTrayIcon->setIcon(QvBaselib->KernelManager()->CurrentConnection().isNull() ? Q_TRAYICON("tray") : Q_TRAYICON("tray-connected"));
    //
    importConfigButton->setIcon(QIcon(STYLE_RESX("add")));
    updownImageBox->setStyleSheet("image: url(" + STYLE_RESX("netspeed_arrow") + ")");
    updownImageBox_2->setStyleSheet("image: url(" + STYLE_RESX("netspeed_arrow") + ")");
    //
    tray_action_ToggleVisibility->setIcon(this->windowIcon());

    action_RCM_Start->setIcon(QIcon(STYLE_RESX("start")));
    action_RCM_Edit->setIcon(QIcon(STYLE_RESX("edit")));
    action_RCM_EditJson->setIcon(QIcon(STYLE_RESX("code")));
    action_RCM_EditComplex->setIcon(QIcon(STYLE_RESX("edit")));
    action_RCM_DuplicateConnection->setIcon(QIcon(STYLE_RESX("copy")));
    action_RCM_DeleteConnection->setIcon(QIcon(STYLE_RESX("ashbin")));
    action_RCM_ResetStats->setIcon(QIcon(STYLE_RESX("ashbin")));
    action_RCM_TestLatency->setIcon(QIcon(STYLE_RESX("ping_gauge")));
    //
    clearChartBtn->setIcon(QIcon(STYLE_RESX("ashbin")));
    clearlogButton->setIcon(QIcon(STYLE_RESX("ashbin")));
    //
    locateBtn->setIcon(QIcon(STYLE_RESX("map")));
    sortBtn->setIcon(QIcon(STYLE_RESX("arrow-down-filling")));
    collapseGroupsBtn->setIcon(QIcon(STYLE_RESX("arrow-up")));
}

void MainWindow::UpdateActionTranslations()
{
    tray_SystemProxyMenu->setTitle(tr("System Proxy"));
    tray_RecentConnectionsMenu->setTitle(tr("Recent Connections"));
    tray_ClearRecentConnectionsAction->setText(tr("Clear Recent Connections"));
    //
    tray_action_ToggleVisibility->setText(tr("Hide"));
    tray_action_Preferences->setText(tr("Preferences"));
    tray_action_Quit->setText(tr("Quit"));
    tray_action_Start->setText(tr("Connect"));
    tray_action_Restart->setText(tr("Reconnect"));
    tray_action_Stop->setText(tr("Disconnect"));
    tray_action_SetSystemProxy->setText(tr("Enable System Proxy"));
    tray_action_ClearSystemProxy->setText(tr("Disable System Proxy"));
    //
    action_RCM_Start->setText(tr("Connect to this"));
    action_RCM_SetAutoConnection->setText(tr("Set as automatically connected"));
    action_RCM_EditJson->setText(tr("Edit as JSON"));
    action_RCM_UpdateSubscription->setText(tr("Update Subscription"));
    action_RCM_EditComplex->setText(tr("Edit as Complex Config"));
    action_RCM_RenameConnection->setText(tr("Rename"));
    action_RCM_Edit->setText(tr("Edit"));
    action_RCM_DuplicateConnection->setText(tr("Duplicate to the Same Group"));
    action_RCM_TestLatency->setText(tr("Test Latency"));
    action_RCM_ResetStats->setText(tr("Clear Usage Data"));
    action_RCM_DeleteConnection->setText(tr("Delete Connection"));
    //
    sortMenu->setTitle(tr("Sort connection list."));
    sortAction_SortByName_Asc->setText(tr("By connection name, A-Z"));
    sortAction_SortByName_Dsc->setText(tr("By connection name, Z-A"));
    sortAction_SortByPing_Asc->setText(tr("By latency, Ascending"));
    sortAction_SortByPing_Dsc->setText(tr("By latency, Descending"));
    sortAction_SortByData_Asc->setText(tr("By data, Ascending"));
    sortAction_SortByData_Dsc->setText(tr("By data, Descending"));
    //
    action_RCM_SwitchCoreLog->setText(tr("Switch to Core log"));
    action_RCM_SwitchQv2rayLog->setText(tr("Switch to Qv2ray log"));
    //
    action_RCM_CopyGraph->setText(tr("Copy graph as image."));
    action_RCM_CopyRecentLogs->setText(tr("Copy latest logs."));
    action_RCM_CopySelected->setText(tr("Copy selected."));
}
