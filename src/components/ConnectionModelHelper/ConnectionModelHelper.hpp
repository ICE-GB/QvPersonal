#pragma once

#include "QvPluginInterface.hpp"

#include <QStandardItem>
#include <QStandardItemModel>
#include <QTreeView>

namespace Qv2ray::ui::widgets::models
{
    enum ConnectionInfoRole
    {
        // 10 -> Magic value.
        ROLE_DISPLAYNAME = Qt::UserRole + 10,
        ROLE_LATENCY,
        ROLE_IMPORTTIME,
        ROLE_LAST_CONNECTED_TIME,
        ROLE_DATA_USAGE
    };

    class ConnectionListHelper : public QObject
    {
        Q_OBJECT
      public:
        ConnectionListHelper(QTreeView *parentView, QObject *parent = nullptr);
        ~ConnectionListHelper();
        void Sort(ConnectionInfoRole, Qt::SortOrder);
        void Filter(const QString &);

        inline QModelIndex GetConnectionPairIndex(const ProfileId &id) const
        {
            return model->indexFromItem(pairs.value(id));
        }

        inline QModelIndex GetGroupIndex(const GroupId &id) const
        {
            return model->indexFromItem(groups[id]);
        }

      private:
        QStandardItem *addConnectionItem(const ProfileId &id);
        QStandardItem *addGroupItem(const GroupId &groupId);
        void OnGroupCreated(const GroupId &id, const QString &displayName);
        void OnGroupDeleted(const GroupId &id, const QList<ConnectionId> &connections);
        void OnConnectionCreated(const ProfileId &Id, const QString &displayName);
        void OnConnectionDeleted(const ProfileId &Id);
        void OnConnectionLinkedWithGroup(const ProfileId &id);

      private:
        QTreeView *parentView;
        QStandardItemModel *model;

        QHash<GroupId, QStandardItem *> groups;
        QHash<ProfileId, QStandardItem *> pairs;
        QHash<ConnectionId, QList<QStandardItem *>> connections;
    };

} // namespace Qv2ray::ui::widgets::models

using namespace Qv2ray::ui::widgets::models;
