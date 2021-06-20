#include "ConnectionModelHelper.hpp"

#include "Common/ProfileHelpers.hpp"
#include "Common/Utils.hpp"
#include "Profile/ProfileManager.hpp"
#include "Qv2rayBaseLibrary.hpp"
#include "ui/widgets/ConnectionItemWidget.hpp"

const auto NumericString = [](auto i) { return QString("%1").arg(i, 30, 10, QLatin1Char('0')); };

ConnectionListHelper::ConnectionListHelper(QTreeView *view, QObject *parent) : QObject(parent)
{
    parentView = view;
    model = new QStandardItemModel();
    view->setModel(model);
    for (const auto &group : QvBaselib->ProfileManager()->GetGroups())
    {
        addGroupItem(group);
        for (const auto &connection : QvBaselib->ProfileManager()->GetConnections(group))
        {
            addConnectionItem({ connection, group });
        }
    }

    const auto renamedLambda = [&](const ConnectionId &id, const QString &, const QString &newName) {
        for (const auto &gid : QvBaselib->ProfileManager()->GetGroups(id))
        {
            ConnectionGroupPair pair{ id, gid };
            if (pairs.contains(pair))
                pairs[pair]->setData(newName, ROLE_DISPLAYNAME);
        }
    };

    const auto latencyLambda = [&](const ConnectionId &id, const int avg) {
        for (const auto &gid : QvBaselib->ProfileManager()->GetGroups(id))
        {
            ConnectionGroupPair pair{ id, gid };
            if (pairs.contains(pair))
                pairs[pair]->setData(NumericString(avg), ROLE_LATENCY);
        }
    };

    const auto statsLambda = [&](const ConnectionGroupPair &id, const QMap<StatisticsObject::StatisticsType, StatisticsObject::StatsEntry> &) {
        if (connections.contains(id.connectionId))
        {
            for (const auto &index : connections[id.connectionId])
                index->setData(NumericString(GetConnectionTotalUsage(id.connectionId, StatisticsObject::API_OUTBOUND_PROXY)), ROLE_DATA_USAGE);
        }
    };

    connect(QvBaselib->ProfileManager(), &Qv2rayBase::Profile::ProfileManager::OnConnectionRemovedFromGroup, this, &ConnectionListHelper::OnConnectionDeleted);
    connect(QvBaselib->ProfileManager(), &Qv2rayBase::Profile::ProfileManager::OnConnectionCreated, this, &ConnectionListHelper::OnConnectionCreated);
    connect(QvBaselib->ProfileManager(), &Qv2rayBase::Profile::ProfileManager::OnConnectionLinkedWithGroup, this, &ConnectionListHelper::OnConnectionLinkedWithGroup);
    connect(QvBaselib->ProfileManager(), &Qv2rayBase::Profile::ProfileManager::OnGroupCreated, this, &ConnectionListHelper::OnGroupCreated);
    connect(QvBaselib->ProfileManager(), &Qv2rayBase::Profile::ProfileManager::OnGroupDeleted, this, &ConnectionListHelper::OnGroupDeleted);
    connect(QvBaselib->ProfileManager(), &Qv2rayBase::Profile::ProfileManager::OnConnectionRenamed, renamedLambda);
    connect(QvBaselib->ProfileManager(), &Qv2rayBase::Profile::ProfileManager::OnLatencyTestFinished, latencyLambda);
    connect(QvBaselib->ProfileManager(), &Qv2rayBase::Profile::ProfileManager::OnStatsAvailable, statsLambda);
}

ConnectionListHelper::~ConnectionListHelper()
{
    delete model;
}

void ConnectionListHelper::Sort(ConnectionInfoRole role, Qt::SortOrder order)
{
    model->setSortRole(role);
    model->sort(0, order);
}

void ConnectionListHelper::Filter(const QString &key)
{
    for (const auto &groupId : QvBaselib->ProfileManager()->GetGroups())
    {
        const auto groupItem = model->indexFromItem(groups[groupId]);
        bool isTotallyHide = true;
        for (const auto &connectionId : QvBaselib->ProfileManager()->GetConnections(groupId))
        {
            const auto connectionItem = model->indexFromItem(pairs[{ connectionId, groupId }]);
            const auto willTotallyHide = static_cast<ConnectionItemWidget *>(parentView->indexWidget(connectionItem))->NameMatched(key);
            parentView->setRowHidden(connectionItem.row(), connectionItem.parent(), !willTotallyHide);
            isTotallyHide &= willTotallyHide;
        }
        parentView->indexWidget(groupItem)->setHidden(isTotallyHide);
        if (!isTotallyHide)
            parentView->expand(groupItem);
    }
}

QStandardItem *ConnectionListHelper::addConnectionItem(const ConnectionGroupPair &id)
{
    // Create Standard Item
    auto connectionItem = new QStandardItem();
    connectionItem->setData(GetDisplayName(id.connectionId), ConnectionInfoRole::ROLE_DISPLAYNAME);
    connectionItem->setData(NumericString(GetConnectionLatency(id.connectionId)), ConnectionInfoRole::ROLE_LATENCY);
    connectionItem->setData(NumericString(GetConnectionTotalUsage(id.connectionId, StatisticsObject::API_OUTBOUND_PROXY)), ConnectionInfoRole::ROLE_DATA_USAGE);
    //
    // Find groups
    const auto groupIndex = groups.contains(id.groupId) ? groups[id.groupId] : addGroupItem(id.groupId);
    // Append into model
    groupIndex->appendRow(connectionItem);
    const auto connectionIndex = connectionItem->index();
    //
    auto widget = new ConnectionItemWidget(id, parentView);
    connect(widget, &ConnectionItemWidget::RequestWidgetFocus, [connectionIndex, this]() {
        parentView->setCurrentIndex(connectionIndex);
        parentView->scrollTo(connectionIndex);
        parentView->clicked(connectionIndex);
    });
    //
    parentView->setIndexWidget(connectionIndex, widget);
    pairs[id] = connectionItem;
    connections[id.connectionId].append(connectionItem);
    return connectionItem;
}

QStandardItem *ConnectionListHelper::addGroupItem(const GroupId &groupId)
{
    // Create Item
    const auto item = new QStandardItem();
    // Set item into model
    model->appendRow(item);
    // Get item index
    const auto index = item->index();
    parentView->setIndexWidget(index, new ConnectionItemWidget(groupId, parentView));
    groups[groupId] = item;
    return item;
}

void ConnectionListHelper::OnConnectionCreated(const ConnectionGroupPair &id, const QString &)
{
    addConnectionItem(id);
}

void ConnectionListHelper::OnConnectionDeleted(const ConnectionGroupPair &id)
{
    auto item = pairs.take(id);
    const auto index = model->indexFromItem(item);
    if (!index.isValid())
        return;
    model->removeRow(index.row(), index.parent());
    connections[id.connectionId].removeAll(item);
}

void ConnectionListHelper::OnConnectionLinkedWithGroup(const ConnectionGroupPair &pairId)
{
    addConnectionItem(pairId);
}

void ConnectionListHelper::OnGroupCreated(const GroupId &id, const QString &)
{
    addGroupItem(id);
}

void ConnectionListHelper::OnGroupDeleted(const GroupId &id, const QList<ConnectionId> &connections)
{
    for (const auto &conn : connections)
    {
        const ConnectionGroupPair pair{ conn, id };
        OnConnectionDeleted(pair);
    }
    const auto item = groups.take(id);
    const auto index = model->indexFromItem(item);
    if (!index.isValid())
        return;
    model->removeRow(index.row(), index.parent());
}
