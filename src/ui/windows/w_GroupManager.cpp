#include "w_GroupManager.hpp"

#include "Qv2rayBase/Common/ProfileHelpers.hpp"
#include "Qv2rayBase/Common/Utils.hpp"
#include "Qv2rayBase/Plugin/PluginAPIHost.hpp"
#include "Qv2rayBase/Plugin/PluginManagerCore.hpp"
#include "Qv2rayBase/Profile/ProfileManager.hpp"
#include "ui/widgets/editors/DnsSettingsWidget.hpp"
#include "ui/widgets/editors/RouteSettingsMatrix.hpp"

#include <QFileDialog>

const static auto RemoveInvalidFileName = [](QString fileName) {
    const static QString pattern = R"("/\?%&^*;:|><)";
    std::replace_if(
        fileName.begin(), fileName.end(), [](const QChar &c) { return pattern.contains(c); }, '_');
    return fileName;
};

#define SELECTED_ROWS_INDEX                                                                                                                                              \
    [&]() {                                                                                                                                                              \
        const auto &__selection = connectionsTable->selectedItems();                                                                                                     \
        QSet<int> rows;                                                                                                                                                  \
        for (const auto &selection : __selection)                                                                                                                        \
            rows.insert(connectionsTable->row(selection));                                                                                                               \
        return rows;                                                                                                                                                     \
    }()

#define GET_SELECTED_CONNECTION_IDS(connectionIdList)                                                                                                                    \
    [&]() {                                                                                                                                                              \
        QList<ConnectionId> _list;                                                                                                                                       \
        for (const auto &i : connectionIdList)                                                                                                                           \
            _list.push_back(ConnectionId(connectionsTable->item(i, 0)->data(Qt::UserRole).toString()));                                                                  \
        return _list;                                                                                                                                                    \
    }()

GroupManager::GroupManager(QWidget *parent) : QvDialog("GroupManager", parent)
{
    setupUi(this);
    QvMessageBusConnect();

    for (const auto &[pluginInfo, info] : QvBaselib->PluginAPIHost()->Subscription_GetAllAdapters())
    {
        subscriptionTypeCB->addItem(pluginInfo->metadata().Name + ": " + info.displayName, info.type.toString());
    }

    dnsSettingsWidget = new DnsSettingsWidget(this);
    routeSettingsWidget = new RouteSettingsMatrixWidget(this);

    dnsSettingsGB->setLayout(new QGridLayout(dnsSettingsGB));
    dnsSettingsGB->layout()->addWidget(dnsSettingsWidget);

    routeSettingsGB->setLayout(new QGridLayout(routeSettingsGB));
    routeSettingsGB->layout()->addWidget(routeSettingsWidget);

    connectionListRCMenu->addSection(tr("Connection Management"));
    connectionListRCMenu->addAction(exportConnectionAction);
    connectionListRCMenu->addAction(deleteConnectionAction);
    connectionListRCMenu->addSeparator();
    connectionListRCMenu->addMenu(connectionListRCMenu_CopyToMenu);
    connectionListRCMenu->addMenu(connectionListRCMenu_MoveToMenu);
    connectionListRCMenu->addMenu(connectionListRCMenu_LinkToMenu);

    connect(exportConnectionAction, &QAction::triggered, this, &GroupManager::onRCMExportConnectionTriggered);
    connect(deleteConnectionAction, &QAction::triggered, this, &GroupManager::onRCMDeleteConnectionTriggered);
    connect(QvBaselib->ProfileManager(), &Qv2rayBase::Profile::ProfileManager::OnConnectionLinkedWithGroup, [this] { reloadConnectionsList(currentGroupId); });
    connect(QvBaselib->ProfileManager(), &Qv2rayBase::Profile::ProfileManager::OnGroupCreated, this, &GroupManager::reloadGroupRCMActions);
    connect(QvBaselib->ProfileManager(), &Qv2rayBase::Profile::ProfileManager::OnGroupDeleted, this, &GroupManager::reloadGroupRCMActions);
    connect(QvBaselib->ProfileManager(), &Qv2rayBase::Profile::ProfileManager::OnGroupRenamed, this, &GroupManager::reloadGroupRCMActions);

    GroupManager::updateColorScheme();

    for (const auto &group : QvBaselib->ProfileManager()->GetGroups())
    {
        auto item = new QListWidgetItem(GetDisplayName(group));
        item->setData(Qt::UserRole, group.toString());
        groupList->addItem(item);
    }

    if (groupList->count() > 0)
    {
        groupList->setCurrentItem(groupList->item(0));
    }
    else
    {
        groupInfoGroupBox->setEnabled(false);
        tabWidget->setEnabled(false);
    }

    reloadGroupRCMActions();
}

void GroupManager::onRCMDeleteConnectionTriggered()
{
    const auto list = GET_SELECTED_CONNECTION_IDS(SELECTED_ROWS_INDEX);
    for (const auto &item : list)
        QvBaselib->ProfileManager()->RemoveFromGroup(ConnectionId(item), currentGroupId);
    reloadConnectionsList(currentGroupId);
}

void GroupManager::onRCMExportConnectionTriggered()
{
    const auto &list = GET_SELECTED_CONNECTION_IDS(SELECTED_ROWS_INDEX);
    QFileDialog d;
    switch (list.count())
    {
        case 0: return;
        case 1:
        {
#pragma message("TODO")
            //            const auto id = ConnectionId(list.first());
            //            auto filePath = d.getSaveFileName(this, GetDisplayName(id));
            //            if (filePath.isEmpty())
            //                return;
            //            auto root = RouteManager->GenerateFinalConfig({ id, currentGroupId }, false);
            //            //
            //            // Apply export filter
            //            exportConnectionFilter(root);
            //            //
            //            if (filePath.endsWith(".json"))
            //            {
            //                filePath += ".json";
            //            }
            //            //
            //            WriteFile(JsonToString(root), filePath);
            //            QDesktopServices::openUrl(QUrl::fromLocalFile(QFileInfo(filePath).absoluteDir().absolutePath()));
            //            break;
        }
        default:
        {
            const auto path = d.getExistingDirectory();
            if (path.isEmpty())
                return;
            for (const auto &connId : list)
            {
#pragma message("TODO")
                //                ConnectionId id(connId);
                //                auto root = RouteManager->GenerateFinalConfig({ id, currentGroupId });
                //                //
                //                // Apply export filter
                //                exportConnectionFilter(root);
                //                //

                //                const auto fileName = RemoveInvalidFileName(GetDisplayName(id)) + ".json";
                //                StringToFile(JsonToString(root), path + "/" + fileName);
            }
            QvBaselib->OpenURL(QUrl::fromLocalFile(path));
            break;
        }
    }
}

void GroupManager::reloadGroupRCMActions()
{
    connectionListRCMenu_CopyToMenu->clear();
    connectionListRCMenu_MoveToMenu->clear();
    connectionListRCMenu_LinkToMenu->clear();
    for (const auto &group : QvBaselib->ProfileManager()->GetGroups())
    {
        auto cpAction = new QAction(GetDisplayName(group), connectionListRCMenu_CopyToMenu);
        auto mvAction = new QAction(GetDisplayName(group), connectionListRCMenu_MoveToMenu);
        auto lnAction = new QAction(GetDisplayName(group), connectionListRCMenu_LinkToMenu);
        //
        cpAction->setData(group.toString());
        mvAction->setData(group.toString());
        lnAction->setData(group.toString());
        //
        connectionListRCMenu_CopyToMenu->addAction(cpAction);
        connectionListRCMenu_MoveToMenu->addAction(mvAction);
        connectionListRCMenu_LinkToMenu->addAction(lnAction);
        //
        connect(cpAction, &QAction::triggered, this, &GroupManager::onRCMActionTriggered_Copy);
        connect(mvAction, &QAction::triggered, this, &GroupManager::onRCMActionTriggered_Move);
        connect(lnAction, &QAction::triggered, this, &GroupManager::onRCMActionTriggered_Link);
    }
}

void GroupManager::reloadConnectionsList(const GroupId &group)
{
    if (group.isNull())
        return;
    connectionsTable->clearContents();
    connectionsTable->model()->removeRows(0, connectionsTable->rowCount());
    const auto &connections = QvBaselib->ProfileManager()->GetConnections(group);
    for (auto i = 0; i < connections.count(); i++)
    {
        const auto &conn = connections.at(i);
        connectionsTable->insertRow(i);
        //
        auto displayNameItem = new QTableWidgetItem(GetDisplayName(conn));
        displayNameItem->setData(Qt::UserRole, conn.toString());
        auto typeItem = new QTableWidgetItem(GetConnectionProtocolDescription(conn));
        //
        const auto [type, host, port] = GetOutboundInfo(QvBaselib->ProfileManager()->GetConnection(conn).outbounds.first());
        auto hostPortItem = new QTableWidgetItem(host + ":" + port);
        //
        QStringList groupsNamesString;
        for (const auto &group : QvBaselib->ProfileManager()->GetGroups(conn))
        {
            groupsNamesString.append(GetDisplayName(group));
        }
        auto groupsItem = new QTableWidgetItem(groupsNamesString.join(";"));
        connectionsTable->setItem(i, 0, displayNameItem);
        connectionsTable->setItem(i, 1, typeItem);
        connectionsTable->setItem(i, 2, hostPortItem);
        connectionsTable->setItem(i, 3, groupsItem);
    }
    connectionsTable->resizeColumnsToContents();
}

void GroupManager::onRCMActionTriggered_Copy()
{
    const auto _sender = qobject_cast<QAction *>(sender());
    const GroupId groupId{ _sender->data().toString() };
    //
    const auto list = GET_SELECTED_CONNECTION_IDS(SELECTED_ROWS_INDEX);
    for (const auto &connId : list)
    {
        const auto &connectionId = ConnectionId(connId);
        QvBaselib->ProfileManager()->CreateConnection(QvBaselib->ProfileManager()->GetConnection(connectionId), GetDisplayName(connectionId), groupId);
    }
    reloadConnectionsList(currentGroupId);
}

void GroupManager::onRCMActionTriggered_Link()
{
    const auto _sender = qobject_cast<QAction *>(sender());
    const GroupId groupId{ _sender->data().toString() };
    //
    const auto list = GET_SELECTED_CONNECTION_IDS(SELECTED_ROWS_INDEX);
    for (const auto &connId : list)
    {
        QvBaselib->ProfileManager()->LinkWithGroup(ConnectionId(connId), groupId);
    }
    reloadConnectionsList(currentGroupId);
}

void GroupManager::onRCMActionTriggered_Move()
{
    const auto _sender = qobject_cast<QAction *>(sender());
    const GroupId groupId{ _sender->data().toString() };
    //
    const auto list = GET_SELECTED_CONNECTION_IDS(SELECTED_ROWS_INDEX);
    for (const auto &connId : list)
    {
        QvBaselib->ProfileManager()->MoveToGroup(ConnectionId(connId), currentGroupId, groupId);
    }
    reloadConnectionsList(currentGroupId);
}

void GroupManager::updateColorScheme()
{
    addGroupButton->setIcon(QIcon(STYLE_RESX("add")));
    removeGroupButton->setIcon(QIcon(STYLE_RESX("ashbin")));
}

QvMessageBusSlotImpl(GroupManager)
{
    switch (msg)
    {
        MBShowDefaultImpl;
        MBHideDefaultImpl;
        MBRetranslateDefaultImpl;
        MBUpdateColorSchemeDefaultImpl
    }
}

GroupManager::~GroupManager(){};
void GroupManager::on_addGroupButton_clicked()
{
    auto const key = tr("New Group") + " - " + GenerateRandomString(5);
    auto id = QvBaselib->ProfileManager()->CreateGroup(key);
    //
    auto item = new QListWidgetItem(key);
    item->setData(Qt::UserRole, id.toString());
    groupList->addItem(item);
    groupList->setCurrentRow(groupList->count() - 1);
}

void GroupManager::on_updateButton_clicked()
{
    const auto address = subAddrTxt->text().trimmed();
    if (address.isEmpty())
    {
        QvBaselib->Warn(tr("Update Subscription"), tr("The subscription link is empty."));
        return;
    }
    if (!QUrl(address).isValid())
    {
        QvBaselib->Warn(tr("Update Subscription"), tr("The subscription link is invalid."));
        return;
    }
    if (QvBaselib->Ask(tr("Update Subscription"), tr("Would you like to update the subscription?")) == Qv2rayBase::MessageOpt::Yes)
    {
        this->setEnabled(false);
        qApp->processEvents();
        QvBaselib->ProfileManager()->UpdateSubscription(currentGroupId);
        this->setEnabled(true);
        on_groupList_itemClicked(groupList->currentItem());
    }
}

void GroupManager::on_removeGroupButton_clicked()
{
    if (QvBaselib->Ask(tr("Remove a Group"), tr("All connections will be moved to default group, do you want to continue?")) == Qv2rayBase::MessageOpt::Yes)
    {
        QvBaselib->ProfileManager()->DeleteGroup(currentGroupId, true);
        auto item = groupList->currentItem();
        int index = groupList->row(item);
        groupList->removeItemWidget(item);
        delete item;
        if (groupList->count() > 0)
        {
            index = std::max(index, 0);
            index = std::min(index, groupList->count() - 1);
            groupList->setCurrentItem(groupList->item(index));
            on_groupList_itemClicked(groupList->item(index));
        }
        else
        {
            groupInfoGroupBox->setEnabled(false);
            tabWidget->setEnabled(false);
        }
    }
}

void GroupManager::on_buttonBox_accepted()
{
    if (!currentGroupId.isNull())
    {

        const auto routingId = QvBaselib->ProfileManager()->GetGroupObject(currentGroupId).route_id;
        auto routing = QvBaselib->ProfileManager()->GetRouting(routingId);

        const auto &[dns, fakedns] = dnsSettingsWidget->GetDNSObject();
        routing.overrideDNS = dnsSettingsGB->isChecked();
        routing.dns = dns.toJson();
        routing.fakedns = fakedns.toJson();

        const auto routematrix = routeSettingsWidget->GetRouteConfig();
        routing.extraOptions.insert(RouteMatrixConfig::EXTRA_OPTIONS_ID, routematrix.toJson());

        QvBaselib->ProfileManager()->UpdateRouting(routingId, routing);
    }
    // Nothing?
}

void GroupManager::on_groupList_itemSelectionChanged()
{
    groupInfoGroupBox->setEnabled(groupList->selectedItems().count() > 0);
}

void GroupManager::on_groupList_itemClicked(QListWidgetItem *item)
{
    if (item == nullptr)
    {
        return;
    }
    groupInfoGroupBox->setEnabled(true);
    currentGroupId = GroupId(item->data(Qt::UserRole).toString());
    groupNameTxt->setText(GetDisplayName(currentGroupId));
    //
    const auto _group = QvBaselib->ProfileManager()->GetGroupObject(currentGroupId);
    groupIsSubscriptionGroup->setChecked(_group.subscription_config.isSubscription);
    subAddrTxt->setText(_group.subscription_config.address);
    lastUpdatedLabel->setText(TimeToString(_group.updated));
    createdAtLabel->setText(TimeToString(_group.created));
    updateIntervalSB->setValue(_group.subscription_config.updateInterval);
    {
        const auto type = _group.subscription_config.type;
        const auto index = subscriptionTypeCB->findData(type.toString());
        if (index < 0)
            QvBaselib->Warn(tr("Unknown Subscription Type"), tr("Unknown subscription type \"%1\", a plugin may be missing.").arg(type.toString()));
        else
            subscriptionTypeCB->setCurrentIndex(index);
    }
    //
    // Import filters
    {
        IncludeRelation->setCurrentIndex(_group.subscription_config.includeRelation);
        IncludeKeywords->clear();
        for (const auto &key : _group.subscription_config.includeKeywords)
        {
            auto str = key.trimmed();
            if (!str.isEmpty())
            {
                IncludeKeywords->appendPlainText(str);
            }
        }
        ExcludeRelation->setCurrentIndex(_group.subscription_config.excludeRelation);
        ExcludeKeywords->clear();
        for (const auto &key : _group.subscription_config.excludeKeywords)
        {
            auto str = key.trimmed();
            if (!str.isEmpty())
            {
                ExcludeKeywords->appendPlainText(str);
            }
        }
    }
    //
    // Load DNS / Route config
    const auto routeId = QvBaselib->ProfileManager()->GetGroupRoutingId(currentGroupId);
    {
        const auto routingObject = QvBaselib->ProfileManager()->GetRouting(routeId);
        dnsSettingsWidget->SetDNSObject(V2RayDNSObject::fromJson(routingObject.dns), V2RayFakeDNSObject::fromJson(routingObject.fakedns));
        dnsSettingsGB->setChecked(routingObject.overrideDNS);
        //
        RouteMatrixConfig c;
        c.loadJson(routingObject.extraOptions.value(RouteMatrixConfig::EXTRA_OPTIONS_ID));
        routeSettingsWidget->SetRoute(c);
        routeSettingsGB->setChecked(routingObject.overrideRules);
    }
    reloadConnectionsList(currentGroupId);
}

void GroupManager::on_IncludeRelation_currentTextChanged(const QString &)
{
    auto subscription = QvBaselib->ProfileManager()->GetGroupObject(currentGroupId).subscription_config;
    subscription.includeRelation = (SubscriptionConfigObject::SubscriptionFilterRelation) IncludeRelation->currentIndex();
    QvBaselib->ProfileManager()->SetSubscriptionData(currentGroupId, subscription);
}

void GroupManager::on_ExcludeRelation_currentTextChanged(const QString &)
{
    auto subscription = QvBaselib->ProfileManager()->GetGroupObject(currentGroupId).subscription_config;
    subscription.excludeRelation = (SubscriptionConfigObject::SubscriptionFilterRelation) ExcludeRelation->currentIndex();
    QvBaselib->ProfileManager()->SetSubscriptionData(currentGroupId, subscription);
}

void GroupManager::on_IncludeKeywords_textChanged()
{
    QStringList keywords = IncludeKeywords->toPlainText().replace("\r", "").split("\n");
    auto subscription = QvBaselib->ProfileManager()->GetGroupObject(currentGroupId).subscription_config;
    subscription.includeKeywords = keywords;
    QvBaselib->ProfileManager()->SetSubscriptionData(currentGroupId, subscription);
}

void GroupManager::on_ExcludeKeywords_textChanged()
{
    QStringList keywords = ExcludeKeywords->toPlainText().replace("\r", "").split("\n");
    auto subscription = QvBaselib->ProfileManager()->GetGroupObject(currentGroupId).subscription_config;
    subscription.excludeKeywords = keywords;
    QvBaselib->ProfileManager()->SetSubscriptionData(currentGroupId, subscription);
}

void GroupManager::on_groupList_currentItemChanged(QListWidgetItem *current, QListWidgetItem *priv)
{
    if (priv)
    {
        const auto routingId = QvBaselib->ProfileManager()->GetGroupObject(currentGroupId).route_id;
        auto routing = QvBaselib->ProfileManager()->GetRouting(routingId);

        const auto &[dns, fakedns] = dnsSettingsWidget->GetDNSObject();
        routing.overrideDNS = dnsSettingsGB->isChecked();
        routing.dns = dns.toJson();
        routing.fakedns = fakedns.toJson();

        const auto routematrix = routeSettingsWidget->GetRouteConfig();
        routing.extraOptions.insert(RouteMatrixConfig::EXTRA_OPTIONS_ID, routematrix.toJson());

        QvBaselib->ProfileManager()->UpdateRouting(routingId, routing);
    }
    if (current)
    {
        on_groupList_itemClicked(current);
    }
}

void GroupManager::on_subAddrTxt_textEdited(const QString &arg1)
{
    auto subscription = QvBaselib->ProfileManager()->GetGroupObject(currentGroupId).subscription_config;
    subscription.address = arg1;
    QvBaselib->ProfileManager()->SetSubscriptionData(currentGroupId, subscription);
}

void GroupManager::on_updateIntervalSB_valueChanged(double arg1)
{
    auto subscription = QvBaselib->ProfileManager()->GetGroupObject(currentGroupId).subscription_config;
    subscription.updateInterval = arg1;
    QvBaselib->ProfileManager()->SetSubscriptionData(currentGroupId, subscription);
}

void GroupManager::on_connectionsList_currentItemChanged(QListWidgetItem *current, QListWidgetItem *previous)
{
    Q_UNUSED(previous)
    if (current != nullptr)
    {
        currentConnectionId = ConnectionId(current->data(Qt::UserRole).toString());
    }
}

void GroupManager::on_groupIsSubscriptionGroup_clicked(bool checked)
{
    auto subscription = QvBaselib->ProfileManager()->GetGroupObject(currentGroupId).subscription_config;
    subscription.isSubscription = checked;
    QvBaselib->ProfileManager()->SetSubscriptionData(currentGroupId, subscription);
}

void GroupManager::on_groupNameTxt_textEdited(const QString &arg1)
{
    groupList->selectedItems().first()->setText(arg1);
    QvBaselib->ProfileManager()->RenameGroup(currentGroupId, arg1.trimmed());
}

void GroupManager::on_deleteSelectedConnBtn_clicked()
{
    onRCMDeleteConnectionTriggered();
}

void GroupManager::on_exportSelectedConnBtn_clicked()
{
    if (connectionsTable->selectedItems().isEmpty())
    {
        connectionsTable->selectAll();
    }
    onRCMExportConnectionTriggered();
}

void GroupManager::exportConnectionFilter(ProfileContent &root)
{
#pragma message("TODO")
}

#undef GET_SELECTED_CONNECTION_IDS

void GroupManager::on_connectionsTable_customContextMenuRequested(const QPoint &pos)
{
    Q_UNUSED(pos)
    connectionListRCMenu->popup(QCursor::pos());
}

void GroupManager::on_subscriptionTypeCB_currentIndexChanged(int)
{
    auto subscription = QvBaselib->ProfileManager()->GetGroupObject(currentGroupId).subscription_config;
    subscription.type = SubscriptionDecoderId{ subscriptionTypeCB->currentData().toString() };
    QvBaselib->ProfileManager()->SetSubscriptionData(currentGroupId, subscription);
}
