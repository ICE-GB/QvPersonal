#include "w_MainWindow.hpp"

#include "Common/ProfileHelpers.hpp"
#include "GuiPluginHost/GuiPluginHost.hpp"
#include "Profile/KernelManager.hpp"
#include "Profile/ProfileManager.hpp"
#include "Qv2rayApplication.hpp"
#include "ui/WidgetUIBase.hpp"
#include "ui/widgets/ConnectionInfoWidget.hpp"
#include "ui/windows/editors/w_JsonEditor.hpp"
#include "ui/windows/editors/w_OutboundEditor.hpp"
#include "ui/windows/editors/w_RoutesEditor.hpp"
#include "ui/windows/w_GroupManager.hpp"
#include "ui/windows/w_ImportConfig.hpp"
#include "ui/windows/w_PluginManager.hpp"
#include "ui/windows/w_PreferencesWindow.hpp"

#include <QClipboard>
#include <QInputDialog>
#include <QScrollBar>

#define QV_MODULE_NAME "MainWindow"
#define TRAY_TOOLTIP_PREFIX "Qv2ray " QV2RAY_VERSION_STRING

#define CheckCurrentWidget                                                                                                                                               \
    auto widget = GetIndexWidget(connectionTreeView->currentIndex());                                                                                                    \
    if (widget == nullptr)                                                                                                                                               \
        return;

#define GetIndexWidget(item) (qobject_cast<ConnectionItemWidget *>(connectionTreeView->indexWidget(item)))
#define NumericString(i) (QString("%1").arg(i, 30, 10, QLatin1Char('0')))

#define PLUGIN_BUTTON_PROPERTY_KEY "plugin_list_index"

QvMessageBusSlotImpl(MainWindow)
{
    switch (msg)
    {
        MBShowDefaultImpl;
        MBHideDefaultImpl;
        MBUpdateColorSchemeDefaultImpl;
        case RETRANSLATE:
        {
            retranslateUi(this);
            UpdateActionTranslations();
            break;
        }
    }
}

void MainWindow::SortConnectionList(ConnectionInfoRole byCol, bool asending)
{
    modelHelper->Sort(byCol, asending ? Qt::AscendingOrder : Qt::DescendingOrder);
    on_locateBtn_clicked();
}

void MainWindow::ReloadRecentConnectionList()
{
    QList<ConnectionGroupPair> newRecentConnections;
    const auto iterateRange = std::min(*GlobalConfig->appearanceConfig->RecentJumpListSize, GlobalConfig->appearanceConfig->RecentConnections->count());
    for (auto i = 0; i < iterateRange; i++)
    {
        const auto &item = GlobalConfig->appearanceConfig->RecentConnections->at(i);
        if (newRecentConnections.contains(item) || item.isNull())
            continue;
        newRecentConnections << item;
    }
    GlobalConfig->appearanceConfig->RecentConnections = newRecentConnections;
}

void MainWindow::OnRecentConnectionsMenuReadyToShow()
{
    tray_RecentConnectionsMenu->clear();
    tray_RecentConnectionsMenu->addAction(tray_ClearRecentConnectionsAction);
    tray_RecentConnectionsMenu->addSeparator();
    for (const auto &conn : *GlobalConfig->appearanceConfig->RecentConnections)
    {
        if (QvBaselib->ProfileManager()->IsValidId(conn))
        {
            const auto name = GetDisplayName(conn.connectionId) + " (" + GetDisplayName(conn.groupId) + ")";
            tray_RecentConnectionsMenu->addAction(name, [=]() { emit QvBaselib->ProfileManager()->StartConnection(conn); });
        }
    }
}

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent)
{
    setupUi(this);
    constexpr auto sizeRatioA = 0.382;
    constexpr auto sizeRatioB = 1 - sizeRatioA;
    splitter->setSizes({ (int) (width() * sizeRatioA), (int) (width() * sizeRatioB) });

    QvMessageBusConnect();
    //
    infoWidget = new ConnectionInfoWidget(this);
    connectionInfoLayout->addWidget(infoWidget);
    //
    masterLogBrowser->setDocument(vCoreLogDocument);
    vCoreLogHighlighter = new SyntaxHighlighter(GlobalConfig->appearanceConfig->DarkModeUI, masterLogBrowser->document());
    // For charts
    speedChartWidget = new SpeedWidget(this);
    speedChart->addWidget(speedChartWidget);
    //
    modelHelper = new ConnectionListHelper(connectionTreeView);
    //
    this->setWindowIcon(QIcon(":/assets/icons/qv2ray.png"));
    updateColorScheme();
    UpdateActionTranslations();
    //
    //
    connect(QvBaselib->ProfileManager(), &Qv2rayBase::Profile::ProfileManager::OnKernelCrashed,
            [this](const ConnectionGroupPair &, const QString &reason)
            {
                MWShowWindow();
                qApp->processEvents();
                QvBaselib->Warn(tr("Kernel terminated."),
                                 tr("The kernel terminated unexpectedly:") + NEWLINE + reason + NEWLINE + NEWLINE +
                                     tr("To solve the problem, read the kernel log in the log text browser."));
            });
    //
    connect(QvBaselib->ProfileManager(), &Qv2rayBase::Profile::ProfileManager::OnConnected, this, &MainWindow::OnConnected);
    connect(QvBaselib->ProfileManager(), &Qv2rayBase::Profile::ProfileManager::OnDisconnected, this, &MainWindow::OnDisconnected);
    connect(QvBaselib->ProfileManager(), &Qv2rayBase::Profile::ProfileManager::OnStatsAvailable, this, &MainWindow::OnStatsAvailable);
    connect(QvBaselib->KernelManager(), &Qv2rayBase::Profile::KernelManager::OnKernelLogAvailable, this, &MainWindow::OnVCoreLogAvailable);
    connect(QvBaselib->ProfileManager(), &Qv2rayBase::Profile::ProfileManager::OnSubscriptionAsyncUpdateFinished,
            [](const GroupId &gid)
            {
                QvApp->ShowTrayMessage(tr("Subscription \"%1\" has been updated").arg(GetDisplayName(gid))); //
            });

    connect(infoWidget, &ConnectionInfoWidget::OnEditRequested, this, &MainWindow::OnEditRequested);
    connect(infoWidget, &ConnectionInfoWidget::OnJsonEditRequested, this, &MainWindow::OnEditJsonRequested);

    connect(masterLogBrowser->verticalScrollBar(), &QSlider::valueChanged, this, &MainWindow::OnLogScrollbarValueChanged);
    //
    // Setup System tray icons and menus
    qvAppTrayIcon->setToolTip(TRAY_TOOLTIP_PREFIX);
    qvAppTrayIcon->show();
    //
    // Basic tray actions
    tray_action_Start->setEnabled(true);
    tray_action_Stop->setEnabled(false);
    tray_action_Restart->setEnabled(false);
    //
    tray_SystemProxyMenu->setEnabled(false);
    tray_SystemProxyMenu->addAction(tray_action_SetSystemProxy);
    tray_SystemProxyMenu->addAction(tray_action_ClearSystemProxy);
    //
    tray_RootMenu->addAction(tray_action_ToggleVisibility);
    tray_RootMenu->addSeparator();
    tray_RootMenu->addAction(tray_action_Preferences);
    tray_RootMenu->addMenu(tray_SystemProxyMenu);
    //
    tray_RootMenu->addSeparator();
    tray_RootMenu->addMenu(tray_RecentConnectionsMenu);
    connect(tray_RecentConnectionsMenu, &QMenu::aboutToShow, this, &MainWindow::OnRecentConnectionsMenuReadyToShow);
    //
    tray_RootMenu->addSeparator();
    tray_RootMenu->addAction(tray_action_Start);
    tray_RootMenu->addAction(tray_action_Stop);
    tray_RootMenu->addAction(tray_action_Restart);
    tray_RootMenu->addSeparator();
    tray_RootMenu->addAction(tray_action_Quit);
    qvAppTrayIcon->setContextMenu(tray_RootMenu);
    //
    connect(tray_action_ToggleVisibility, &QAction::triggered, this, &MainWindow::MWToggleVisibility);
    connect(tray_action_Preferences, &QAction::triggered, this, &MainWindow::on_preferencesBtn_clicked);
    connect(tray_action_Start, &QAction::triggered, [this] { QvBaselib->ProfileManager()->StartConnection(lastConnected); });
    connect(tray_action_Stop, &QAction::triggered, QvBaselib->ProfileManager(), &Qv2rayBase::Profile::ProfileManager::StopConnection);
    connect(tray_action_Restart, &QAction::triggered, QvBaselib->ProfileManager(), &Qv2rayBase::Profile::ProfileManager::RestartConnection);
    connect(tray_action_Quit, &QAction::triggered, this, &MainWindow::Action_Exit);
    connect(tray_action_SetSystemProxy, &QAction::triggered, this, &MainWindow::MWSetSystemProxy);
    connect(tray_action_ClearSystemProxy, &QAction::triggered, this, &MainWindow::MWClearSystemProxy);
    connect(tray_ClearRecentConnectionsAction, &QAction::triggered,
            [this]()
            {
                GlobalConfig->appearanceConfig->RecentConnections->clear();
                ReloadRecentConnectionList();
                if (!GlobalConfig->behaviorConfig->QuietMode)
                    QvApp->ShowTrayMessage(tr("Recent Connection list cleared."));
            });
    connect(qvAppTrayIcon, &QSystemTrayIcon::activated, this, &MainWindow::on_activatedTray);
    //
    // Actions for right click the log text browser
    //
    logRCM_Menu->addAction(action_RCM_CopySelected);
    logRCM_Menu->addAction(action_RCM_CopyRecentLogs);
    logRCM_Menu->addSeparator();
    logRCM_Menu->addAction(action_RCM_SwitchCoreLog);
    logRCM_Menu->addAction(action_RCM_SwitchQv2rayLog);
    connect(masterLogBrowser, &QTextBrowser::customContextMenuRequested, [this](const QPoint &) { logRCM_Menu->popup(QCursor::pos()); });
    connect(action_RCM_SwitchCoreLog, &QAction::triggered, [this] { masterLogBrowser->setDocument(vCoreLogDocument); });
    connect(action_RCM_SwitchQv2rayLog, &QAction::triggered, [this] { masterLogBrowser->setDocument(qvLogDocument); });
    connect(action_RCM_CopyRecentLogs, &QAction::triggered, this, &MainWindow::Action_CopyRecentLogs);
    connect(action_RCM_CopySelected, &QAction::triggered, masterLogBrowser, &QTextBrowser::copy);
    //
    speedChartWidget->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(speedChartWidget, &QWidget::customContextMenuRequested, [this](const QPoint &) { graphWidgetMenu->popup(QCursor::pos()); });
    //
    masterLogBrowser->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
    {
        auto font = masterLogBrowser->font();
        font.setPointSize(9);
        masterLogBrowser->setFont(font);
        qvLogDocument->setDefaultFont(font);
        vCoreLogDocument->setDefaultFont(font);
    }
    //
    // Globally invokable signals.
    //
    connect(this, &MainWindow::StartConnection, QvBaselib->ProfileManager(), &Qv2rayBase::Profile::ProfileManager::RestartConnection);
    connect(this, &MainWindow::StopConnection, QvBaselib->ProfileManager(), &Qv2rayBase::Profile::ProfileManager::StopConnection);
    connect(this, &MainWindow::RestartConnection, QvBaselib->ProfileManager(), &Qv2rayBase::Profile::ProfileManager::RestartConnection);
    //
    // Actions for right click the connection list
    //
    connectionListRCM_Menu->addAction(action_RCM_Start);
    connectionListRCM_Menu->addSeparator();
    connectionListRCM_Menu->addAction(action_RCM_Edit);
    connectionListRCM_Menu->addAction(action_RCM_EditJson);
    connectionListRCM_Menu->addAction(action_RCM_EditComplex);
    connectionListRCM_Menu->addSeparator();

    connectionListRCM_Menu->addAction(action_RCM_TestLatency);

    connectionListRCM_Menu->addSeparator();
    connectionListRCM_Menu->addAction(action_RCM_SetAutoConnection);
    connectionListRCM_Menu->addSeparator();
    connectionListRCM_Menu->addAction(action_RCM_RenameConnection);
    connectionListRCM_Menu->addAction(action_RCM_DuplicateConnection);
    connectionListRCM_Menu->addAction(action_RCM_ResetStats);
    connectionListRCM_Menu->addAction(action_RCM_UpdateSubscription);
    connectionListRCM_Menu->addSeparator();

    connectionListRCM_Menu->addAction(action_RCM_DeleteConnection);

    //
    connect(action_RCM_Start, &QAction::triggered, this, &MainWindow::Action_Start);
    connect(action_RCM_SetAutoConnection, &QAction::triggered, this, &MainWindow::Action_SetAutoConnection);
    connect(action_RCM_Edit, &QAction::triggered, this, &MainWindow::Action_Edit);
    connect(action_RCM_EditJson, &QAction::triggered, this, &MainWindow::Action_EditJson);
    connect(action_RCM_EditComplex, &QAction::triggered, this, &MainWindow::Action_EditComplex);
    connect(action_RCM_TestLatency, &QAction::triggered, this, &MainWindow::Action_TestLatency);
    connect(action_RCM_RenameConnection, &QAction::triggered, this, &MainWindow::Action_RenameConnection);
    connect(action_RCM_DuplicateConnection, &QAction::triggered, this, &MainWindow::Action_DuplicateConnection);
    connect(action_RCM_ResetStats, &QAction::triggered, this, &MainWindow::Action_ResetStats);
    connect(action_RCM_UpdateSubscription, &QAction::triggered, this, &MainWindow::Action_UpdateSubscription);
    connect(action_RCM_DeleteConnection, &QAction::triggered, this, &MainWindow::Action_DeleteConnections);
    //
    // Sort Menu
    //
    sortMenu->addAction(sortAction_SortByName_Asc);
    sortMenu->addAction(sortAction_SortByName_Dsc);
    sortMenu->addSeparator();
    sortMenu->addAction(sortAction_SortByData_Asc);
    sortMenu->addAction(sortAction_SortByData_Dsc);
    sortMenu->addSeparator();
    sortMenu->addAction(sortAction_SortByPing_Asc);
    sortMenu->addAction(sortAction_SortByPing_Dsc);
    //
    connect(sortAction_SortByName_Asc, &QAction::triggered, [this] { SortConnectionList(ROLE_DISPLAYNAME, true); });
    connect(sortAction_SortByName_Dsc, &QAction::triggered, [this] { SortConnectionList(ROLE_DISPLAYNAME, false); });
    connect(sortAction_SortByData_Asc, &QAction::triggered, [this] { SortConnectionList(ROLE_DATA_USAGE, true); });
    connect(sortAction_SortByData_Dsc, &QAction::triggered, [this] { SortConnectionList(ROLE_DATA_USAGE, false); });
    connect(sortAction_SortByPing_Asc, &QAction::triggered, [this] { SortConnectionList(ROLE_LATENCY, true); });
    connect(sortAction_SortByPing_Dsc, &QAction::triggered, [this] { SortConnectionList(ROLE_LATENCY, false); });
    //
    sortBtn->setMenu(sortMenu);
    //
    graphWidgetMenu->addAction(action_RCM_CopyGraph);
    connect(action_RCM_CopyGraph, &QAction::triggered, this, &MainWindow::Action_CopyGraphAsImage);
    //
    // Find and start if there is an auto-connection
    const auto connectionStarted = StartAutoConnectionEntry();

    if (!connectionStarted && !QvBaselib->ProfileManager()->GetConnections().isEmpty())
    {
        // Select the first connection.
        const auto groups = QvBaselib->ProfileManager()->GetGroups();
        if (!groups.isEmpty())
        {
            const auto connections = QvBaselib->ProfileManager()->GetConnections(groups.first());
            if (!connections.empty())
            {
                const auto index = modelHelper->GetConnectionPairIndex({ connections.first(), groups.first() });
                connectionTreeView->setCurrentIndex(index);
                on_connectionTreeView_clicked(index);
            }
        }
    }
    ReloadRecentConnectionList();
    //
    //
    if (!connectionStarted || !GlobalConfig->behaviorConfig->QuietMode)
        MWShowWindow();
    if (GlobalConfig->behaviorConfig->QuietMode)
        MWToggleVisibilitySetText();
    //
    CheckSubscriptionsUpdate();
    qvLogTimerId = startTimer(1000);
    //

    for (const auto &[pluginInterface, guiInterface] : GUIPluginHost->GUI_QueryByComponent(Qv2rayPlugin::GUI_COMPONENT_MAIN_WINDOW_ACTIONS))
    {
        auto mainWindowWidgetPtr = guiInterface->GetMainWindowWidget();
        if (!mainWindowWidgetPtr)
            continue;
        const auto index = pluginWidgets.count();
        {
            // Let Qt manage the ownership.
            auto widget = mainWindowWidgetPtr.release();
            pluginWidgets.append(widget);
        }
        auto btn = new QPushButton(pluginInterface->GetMetadata().Name, this);
        connect(btn, &QPushButton::clicked, this, &MainWindow::OnPluginButtonClicked);
        btn->setProperty(PLUGIN_BUTTON_PROPERTY_KEY, index);
        topButtonsLayout->addWidget(btn);
    }
}

void MainWindow::OnPluginButtonClicked()
{
    const auto senderWidget = qobject_cast<QPushButton *>(sender());
    if (!senderWidget)
        return;
    bool ok = false;
    const auto index = senderWidget->property(PLUGIN_BUTTON_PROPERTY_KEY).toInt(&ok);
    if (!ok)
        return;
    const auto widget = pluginWidgets.at(index);
    if (!widget)
        return;
    widget->setVisible(!widget->isVisible());
}

void MainWindow::ProcessCommand(QString command, QStringList commands, QMap<QString, QString> args)
{
    if (commands.isEmpty())
        return;
    if (command == "open")
    {
        const auto subcommand = commands.takeFirst();
        QvDialog *w;
        if (subcommand == "preference")
            w = new PreferencesWindow();
        else if (subcommand == "plugin")
            w = new PluginManageWindow();
        else if (subcommand == "group")
            w = new GroupManager();
        else if (subcommand == "import")
            w = new ImportConfigWindow();
        else
            return;
        w->processCommands(command, commands, args);
        w->exec();
        delete w;
    }
}

void MainWindow::timerEvent(QTimerEvent *event)
{
    if (event->timerId() == qvLogTimerId)
    {
#pragma message("TODO Use Qt Message Logger")
        //        auto log = ReadLog().trimmed();
        //        if (!log.isEmpty())
        //        {
        //            FastAppendTextDocument(NEWLINE + log, qvLogDocument);
        //        }
    }
}

void MainWindow::keyPressEvent(QKeyEvent *e)
{
    if (focusWidget() == connectionTreeView)
    {
        CheckCurrentWidget;
        if (e->key() == Qt::Key_Enter || e->key() == Qt::Key_Return)
        {
            // If pressed enter or return on connectionListWidget. Try to connect to the selected connection.
            if (widget->IsConnection())
            {
                widget->BeginConnection();
            }
            else
            {
                connectionTreeView->expand(connectionTreeView->currentIndex());
            }
        }
        else if (e->key() == Qt::Key_F2)
        {
            widget->BeginRename();
        }
        else if (e->key() == Qt::Key_Delete)
        {
            Action_DeleteConnections();
        }
    }

    if (e->key() == Qt::Key_Escape)
    {
        auto widget = GetIndexWidget(connectionTreeView->currentIndex());
        // Check if this key was accpted by the ConnectionItemWidget
        if (widget && widget->IsRenaming())
        {
            widget->CancelRename();
            return;
        }
        else if (this->isActiveWindow())
        {
            this->close();
        }
    }
    // Ctrl + Q = Exit
    else if (e->modifiers() & Qt::ControlModifier && e->key() == Qt::Key_Q)
    {
        if (QvBaselib->Ask(tr("Quit Qv2ray"), tr("Are you sure to exit Qv2ray?")) == Qv2rayBase::MessageOpt::Yes)
            Action_Exit();
    }
    // Control + W = Close Window
    else if (e->modifiers() & Qt::ControlModifier && e->key() == Qt::Key_W)
    {
        if (this->isActiveWindow())
            this->close();
    }
}

void MainWindow::keyReleaseEvent(QKeyEvent *e)
{
    // Workaround of QtWidget not grabbing KeyDown and KeyUp in keyPressEvent
    if (e->key() == Qt::Key_Up || e->key() == Qt::Key_Down)
    {
        if (focusWidget() == connectionTreeView)
        {
            CheckCurrentWidget;
            on_connectionTreeView_clicked(connectionTreeView->currentIndex());
        }
    }
}

void MainWindow::changeEvent(QEvent *e)
{

    if (e->type() == QEvent::WindowStateChange)
    {
        MWToggleVisibilitySetText();
    }
}

void MainWindow::Action_Start()
{
    CheckCurrentWidget;
    if (widget->IsConnection())
    {
        widget->BeginConnection();
    }
}

MainWindow::~MainWindow()
{
    delete modelHelper;
    for (auto &widget : pluginWidgets)
        widget->accept();
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    MWHideWindow();
    event->ignore();
}

void MainWindow::on_activatedTray(QSystemTrayIcon::ActivationReason reason)
{
#ifndef __APPLE__
    const auto toggleTriggerEvent = QSystemTrayIcon::Trigger;
#else
    const auto toggleTriggerEvent = QSystemTrayIcon::DoubleClick;
#endif
    if (reason == toggleTriggerEvent)
        MWToggleVisibility();
}

void MainWindow::Action_Exit()
{
    QvBaselib->ProfileManager()->StopConnection();
    QvApp->quit();
}

void MainWindow::on_preferencesBtn_clicked()
{
    PreferencesWindow{ this }.exec();
}

void MainWindow::on_clearlogButton_clicked()
{
    masterLogBrowser->document()->clear();
}
void MainWindow::on_connectionTreeView_customContextMenuRequested(const QPoint &pos)
{
    Q_UNUSED(pos)
    auto _pos = QCursor::pos();
    auto item = connectionTreeView->indexAt(connectionTreeView->mapFromGlobal(_pos));
    if (item.isValid())
    {
        bool isConnection = GetIndexWidget(item)->IsConnection();
        // Disable connection-specific settings.
        action_RCM_Start->setEnabled(isConnection);
        action_RCM_SetAutoConnection->setEnabled(isConnection);
        action_RCM_Edit->setEnabled(isConnection);
        action_RCM_EditJson->setEnabled(isConnection);
        action_RCM_EditComplex->setEnabled(isConnection);
        action_RCM_RenameConnection->setEnabled(isConnection);
        action_RCM_DuplicateConnection->setEnabled(isConnection);
        action_RCM_UpdateSubscription->setEnabled(!isConnection);
        //        action_RCM_RealLatencyTest->setEnabled(isConnection && QvBaselib->ProfileManager()->IsConnected(GetIndexWidget(item)->Identifier()));
        connectionListRCM_Menu->popup(_pos);
    }
}

void MainWindow::Action_DeleteConnections()
{
    QList<ConnectionGroupPair> connlist;
    QList<GroupId> groupsList;

    for (const auto &item : connectionTreeView->selectionModel()->selectedIndexes())
    {
        const auto widget = GetIndexWidget(item);
        if (!widget)
            continue;

        const auto identifier = widget->Identifier();
        if (widget->IsConnection())
        {
            // Simply add the connection id
            connlist.append(identifier);
            continue;
        }

        for (const auto &conns : QvBaselib->ProfileManager()->GetConnections(identifier.groupId))
        {
            connlist.append(ConnectionGroupPair{ conns, identifier.groupId });
        }

        const auto message = tr("Do you want to remove this group as well?") + NEWLINE + tr("Group: ") + GetDisplayName(identifier.groupId);
        if (QvBaselib->Ask(tr("Removing Connection"), message) == Qv2rayBase::MessageOpt::Yes)
        {
            groupsList << identifier.groupId;
        }
    }

    const auto strRemoveConnTitle = tr("Removing Connection(s)", "", connlist.count());
    const auto strRemoveConnContent = tr("Are you sure to remove selected connection(s)?", "", connlist.count());
    if (QvBaselib->Ask(strRemoveConnTitle, strRemoveConnContent) != Qv2rayBase::MessageOpt::Yes)
    {
        return;
    }

    for (const auto &conn : connlist)
    {
        QvBaselib->ProfileManager()->RemoveFromGroup(conn.connectionId, conn.groupId);
    }

    for (const auto &group : groupsList)
    {
        QvBaselib->ProfileManager()->DeleteGroup(group);
    }
}

void MainWindow::on_importConfigButton_clicked()
{
#pragma message("TODO: Properly handle this")
    ImportConfigWindow w(this);
    //    w.DoImport();
}

void MainWindow::Action_EditComplex()
{
    CheckCurrentWidget;
    if (widget->IsConnection())
    {
        auto id = widget->Identifier();
        ProfileContent root = QvBaselib->ProfileManager()->GetConnection(id.connectionId);
        bool isChanged = false;
        //
        QvLog() << "INFO: Opening route editor.";
        RouteEditor routeWindow(root, this);
        root = routeWindow.OpenEditor();
        isChanged = routeWindow.result() == QDialog::Accepted;
        if (isChanged)
        {
            QvBaselib->ProfileManager()->UpdateConnection(id.connectionId, root);
        }
    }
}

void MainWindow::on_subsButton_clicked()
{
    GroupManager().exec();
}

void MainWindow::OnDisconnected(const ConnectionGroupPair &id)
{
    Q_UNUSED(id)
    qvAppTrayIcon->setIcon(Q_TRAYICON("tray"));
    tray_action_Start->setEnabled(true);
    tray_action_Stop->setEnabled(false);
    tray_action_Restart->setEnabled(false);
    tray_SystemProxyMenu->setEnabled(false);
    lastConnected = id;
    locateBtn->setEnabled(false);
    if (!GlobalConfig->behaviorConfig->QuietMode)
    {
        QvApp->ShowTrayMessage(tr("Disconnected from: ") + GetDisplayName(id.connectionId));
    }
    qvAppTrayIcon->setToolTip(TRAY_TOOLTIP_PREFIX);
    netspeedLabel->setText("0.00 B/s" NEWLINE "0.00 B/s");
    dataamountLabel->setText("0.00 B" NEWLINE "0.00 B");
    connetionStatusLabel->setText(tr("Not Connected"));
#pragma message("TODO Move To Command Plugin")
    //    if (GlobalConfig.inboundConfig->systemProxySettings->setSystemProxy)
    //    {
    //        MWClearSystemProxy();
    //    }
}

void MainWindow::OnConnected(const ConnectionGroupPair &id)
{
    Q_UNUSED(id)
    qvAppTrayIcon->setIcon(Q_TRAYICON("tray-connected"));
    tray_action_Start->setEnabled(false);
    tray_action_Stop->setEnabled(true);
    tray_action_Restart->setEnabled(true);
    tray_SystemProxyMenu->setEnabled(true);
    lastConnected = id;
    locateBtn->setEnabled(true);
    on_clearlogButton_clicked();
    auto name = GetDisplayName(id.connectionId);
    if (!GlobalConfig->behaviorConfig->QuietMode)
    {
        QvApp->ShowTrayMessage(tr("Connected: ") + name);
    }
    qvAppTrayIcon->setToolTip(TRAY_TOOLTIP_PREFIX NEWLINE + tr("Connected: ") + name);
    connetionStatusLabel->setText(tr("Connected: ") + name);
    //
    GlobalConfig->appearanceConfig->RecentConnections->removeAll(id);
    GlobalConfig->appearanceConfig->RecentConnections->push_front(id);
    ReloadRecentConnectionList();
#pragma message("TODO Move To Command Plugin")
    //    if (GlobalConfig.inboundConfig->systemProxySettings->setSystemProxy)
    //    {
    //        MWSetSystemProxy();
    //    }
}

void MainWindow::on_connectionFilterTxt_textEdited(const QString &arg1)
{
    modelHelper->Filter(arg1);
}

void MainWindow::OnStatsAvailable(const ConnectionGroupPair &id, const QMap<StatisticsObject::StatisticsType, StatisticsObject::StatsEntry> &data)
{
    if (!QvBaselib->ProfileManager()->IsConnected(id))
        return;

    // This may not be, or may not precisely be, speed per second if the backend
    // has "any" latency. (Hope not...)
    QMap<SpeedWidget::GraphType, long> pointData;
    for (const auto &[type, data] : data.toStdMap())
    {
        const auto upSpeed = data.up;
        const auto downSpeed = data.down;
        switch (type)
        {
            case StatisticsObject::API_ALL_INBOUND: break;
            case StatisticsObject::API_OUTBOUND_PROXY:
                pointData[SpeedWidget::OUTBOUND_PROXY_UP] = upSpeed;
                pointData[SpeedWidget::OUTBOUND_PROXY_DOWN] = downSpeed;
                break;
            case StatisticsObject::API_OUTBOUND_DIRECT:
                pointData[SpeedWidget::OUTBOUND_DIRECT_UP] = upSpeed;
                pointData[SpeedWidget::OUTBOUND_DIRECT_DOWN] = downSpeed;

                break;
        }
    }

    speedChartWidget->AddPointData(pointData);
    auto totalSpeedUp = FormatBytes(data[StatisticsObject::API_OUTBOUND_PROXY].up) + "/s";
    auto totalSpeedDown = FormatBytes(data[StatisticsObject::API_OUTBOUND_PROXY].down) + "/s";

    const auto &[totalUp, totalDown] = GetConnectionUsageAmount(id.connectionId, StatisticsObject::API_OUTBOUND_PROXY);
    auto totalDataUp = FormatBytes(totalUp);
    auto totalDataDown = FormatBytes(totalDown);
    //
    netspeedLabel->setText(totalSpeedUp + NEWLINE + totalSpeedDown);
    dataamountLabel->setText(totalDataUp + NEWLINE + totalDataDown);
    //
    qvAppTrayIcon->setToolTip(TRAY_TOOLTIP_PREFIX NEWLINE + tr("Connected: ") + GetDisplayName(id.connectionId) + //
                              NEWLINE "Up: " + totalSpeedUp + " Down: " + totalSpeedDown);
}

void MainWindow::OnVCoreLogAvailable(const ConnectionGroupPair &id, const QString &log)
{
    Q_UNUSED(id);
    FastAppendTextDocument(log.trimmed(), vCoreLogDocument);
    // vCoreLogDocument->setPlainText(vCoreLogDocument->toPlainText() + log);
    // From https://gist.github.com/jemyzhang/7130092
    auto maxLines = GlobalConfig->appearanceConfig->MaximizeLogLines;
    auto block = vCoreLogDocument->begin();

    while (block.isValid())
    {
        if (vCoreLogDocument->blockCount() > maxLines)
        {
            QTextCursor cursor(block);
            block = block.next();
            cursor.select(QTextCursor::BlockUnderCursor);
            cursor.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor);
            cursor.removeSelectedText();
            continue;
        }

        break;
    }
}

void MainWindow::OnEditRequested(const ConnectionId &id)
{
    auto outBoundRoot = QvBaselib->ProfileManager()->GetConnection(id);
    ProfileContent root;
    bool isChanged;

    if (IsComplexConfig(id))
    {
        QvLog() << "INFO: Opening route editor.";
        RouteEditor routeWindow(outBoundRoot, this);
        root = routeWindow.OpenEditor();
        isChanged = routeWindow.result() == QDialog::Accepted;
    }
    else
    {
        QvLog() << "INFO: Opening single connection edit window.";
        auto out = outBoundRoot.outbounds.first();
        OutboundEditor w(out, this);
        auto outbound = w.OpenEditor();
        isChanged = w.result() == QDialog::Accepted;
        root.outbounds[0] = outbound;
    }

    if (isChanged)
    {
        QvBaselib->ProfileManager()->UpdateConnection(id, root);
    }
}
void MainWindow::OnEditJsonRequested(const ConnectionId &id)
{
    JsonEditor w(QvBaselib->ProfileManager()->GetConnection(id).toJson(), this);
    auto root = w.OpenEditor();

    ProfileContent newRoot;
    newRoot.loadJson(root);

    if (w.result() == QDialog::Accepted)
    {
        QvBaselib->ProfileManager()->UpdateConnection(id, newRoot);
    }
}

void MainWindow::OnLogScrollbarValueChanged(int value)
{
    if (masterLogBrowser->verticalScrollBar()->maximum() == value)
        qvLogAutoScoll = true;
    else
        qvLogAutoScoll = false;
}

void MainWindow::on_locateBtn_clicked()
{
    auto id = QvBaselib->KernelManager()->CurrentConnection();
    if (!id.isNull())
    {
        const auto index = modelHelper->GetConnectionPairIndex(id);
        connectionTreeView->setCurrentIndex(index);
        connectionTreeView->scrollTo(index);
        on_connectionTreeView_clicked(index);
    }
}

void MainWindow::Action_RenameConnection()
{
    CheckCurrentWidget;
    widget->BeginRename();
}

void MainWindow::Action_DuplicateConnection()
{
    QList<ConnectionGroupPair> connlist;

    for (const auto &item : connectionTreeView->selectionModel()->selectedIndexes())
    {
        auto widget = GetIndexWidget(item);
        if (widget->IsConnection())
        {
            connlist.append(widget->Identifier());
        }
    }

    QvLog() << "Selected" << connlist.count() << "items.";

    const auto strDupConnTitle = tr("Duplicating Connection(s)", "", connlist.count());
    const auto strDupConnContent = tr("Are you sure to duplicate these connection(s)?", "", connlist.count());
    if (connlist.count() > 1 && QvBaselib->Ask(strDupConnTitle, strDupConnContent) != Qv2rayBase::MessageOpt::Yes)
    {
        return;
    }

    for (const auto &conn : connlist)
    {
        QvBaselib->ProfileManager()->CreateConnection(QvBaselib->ProfileManager()->GetConnection(conn.connectionId), GetDisplayName(conn.connectionId) + tr(" (Copy)"),
                                                      conn.groupId);
    }
}

void MainWindow::Action_Edit()
{
    CheckCurrentWidget;
    OnEditRequested(widget->Identifier().connectionId);
}

void MainWindow::Action_EditJson()
{
    CheckCurrentWidget;
    OnEditJsonRequested(widget->Identifier().connectionId);
}

void MainWindow::on_chartVisibilityBtn_clicked()
{
    speedChartHolderWidget->setVisible(!speedChartWidget->isVisible());
}

void MainWindow::on_logVisibilityBtn_clicked()
{
    masterLogBrowser->setVisible(!masterLogBrowser->isVisible());
}

void MainWindow::on_clearChartBtn_clicked()
{
    speedChartWidget->Clear();
}

void MainWindow::on_masterLogBrowser_textChanged()
{
    if (!qvLogAutoScoll)
        return;
    auto bar = masterLogBrowser->verticalScrollBar();
    bar->setValue(bar->maximum());
}

void MainWindow::Action_SetAutoConnection()
{
    auto current = connectionTreeView->currentIndex();
    if (current.isValid())
    {
        auto widget = GetIndexWidget(current);
        const auto identifier = widget->Identifier();
        GlobalConfig->behaviorConfig->AutoConnectProfileId = identifier;
        GlobalConfig->behaviorConfig->AutoConnectBehavior = Qv2rayBehaviorConfig::AUTOCONNECT_FIXED;
        if (!GlobalConfig->behaviorConfig->QuietMode)
        {
            QvApp->ShowTrayMessage(tr("%1 has been set as auto connect.").arg(GetDisplayName(identifier.connectionId)));
        }
#pragma message("TODO")
        //        SaveGlobalSettings();
    }
}

void MainWindow::Action_ResetStats()
{
    auto current = connectionTreeView->currentIndex();
    if (current.isValid())
    {
        auto widget = GetIndexWidget(current);
        if (widget)
        {
            if (widget->IsConnection())
                QvBaselib->ProfileManager()->ClearConnectionUsage(widget->Identifier());
            else
                QvBaselib->ProfileManager()->ClearGroupUsage(widget->Identifier().groupId);
        }
    }
}

void MainWindow::Action_UpdateSubscription()
{
    auto current = connectionTreeView->currentIndex();
    if (current.isValid())
    {
        auto widget = GetIndexWidget(current);
        if (widget)
        {
            if (widget->IsConnection())
                return;
            const auto gid = widget->Identifier().groupId;
            if (QvBaselib->ProfileManager()->GetGroupObject(gid).subscription_config.isSubscription)
                QvBaselib->ProfileManager()->UpdateSubscriptionAsync(gid);
            else
                QvBaselib->Info(tr("Update Subscription"), tr("Selected group is not a subscription"));
        }
    }
}

void MainWindow::Action_TestLatency()
{
    for (const auto &current : connectionTreeView->selectionModel()->selectedIndexes())
    {
        if (!current.isValid())
            continue;
        const auto widget = GetIndexWidget(current);
        if (!widget)
            continue;

#pragma message("TODO Add Default LatencyTestEngineId")
        //        if (widget->IsConnection())
        //            QvBaselib->ProfileManager()->StartLatencyTest(widget->Identifier().connectionId);
        //        else
        //            QvBaselib->ProfileManager()->StartLatencyTest(widget->Identifier().groupId);
    }
}

// void MainWindow::Action_TestRealLatency()
//{
//    for (const auto &current : connectionTreeView->selectionModel()->selectedIndexes())
//    {
//        if (!current.isValid())
//            continue;
//        const auto widget = GetIndexWidget(current);
//        if (!widget)
//            continue;
//        if (widget->IsConnection() && QvBaselib->ProfileManager()->IsConnected(widget->Identifier()))
//            QvBaselib->ProfileManager()->StartLatencyTest(widget->Identifier().connectionId, REALPING);
//    }
//}

void MainWindow::Action_CopyGraphAsImage()
{
    const auto image = speedChartWidget->grab();
    qApp->clipboard()->setImage(image.toImage());
}

void MainWindow::on_pluginsBtn_clicked()
{
    PluginManageWindow(this).exec();
}

void MainWindow::on_newConnectionBtn_clicked()
{
    OutboundEditor w(this);
    auto outboundEntry = w.OpenEditor();
    bool isChanged = w.result() == QDialog::Accepted;
    if (isChanged)
    {
        const auto alias = w.GetFriendlyName();
        ProfileContent root{ outboundEntry };
        const auto item = connectionTreeView->currentIndex();
        const auto id = item.isValid() ? GetIndexWidget(item)->Identifier().groupId : DefaultGroupId;
        QvBaselib->ProfileManager()->CreateConnection(root, alias, id);
    }
}

void MainWindow::on_newComplexConnectionBtn_clicked()
{
    RouteEditor w({}, this);
    auto root = w.OpenEditor();
    bool isChanged = w.result() == QDialog::Accepted;
    if (isChanged)
    {
        const auto item = connectionTreeView->currentIndex();
        const auto id = item.isValid() ? GetIndexWidget(item)->Identifier().groupId : DefaultGroupId;
        QvBaselib->ProfileManager()->CreateConnection(root, root.outbounds.first().name, id);
    }
}

void MainWindow::on_collapseGroupsBtn_clicked()
{
    connectionTreeView->collapseAll();
}

void MainWindow::Action_CopyRecentLogs()
{
    const auto lines = SplitLines(masterLogBrowser->document()->toPlainText());
    bool accepted = false;
    const auto line = QInputDialog::getInt(this, tr("Copy latest logs"), tr("Number of lines of logs to copy"), 20, 0, 2500, 1, &accepted);
    if (!accepted)
        return;
    const auto totalLinesCount = lines.count();
    const auto linesToCopy = std::min((int) totalLinesCount, line);
    QStringList result;
    for (auto i = totalLinesCount - linesToCopy; i < totalLinesCount; i++)
    {
        result.append(lines[i]);
    }
    qApp->clipboard()->setText(result.join(NEWLINE));
}

void MainWindow::on_connectionTreeView_doubleClicked(const QModelIndex &index)
{
    auto widget = GetIndexWidget(index);
    if (widget == nullptr)
        return;
    if (widget->IsConnection())
        widget->BeginConnection();
}

void MainWindow::on_connectionTreeView_clicked(const QModelIndex &index)
{
    auto widget = GetIndexWidget(index);
    if (widget == nullptr)
        return;
    infoWidget->ShowDetails(widget->Identifier());
}
