#include "ConnectionItemWidget.hpp"

#include "Qv2rayBase/Common/ProfileHelpers.hpp"
#include "Qv2rayBase/Common/Utils.hpp"
#include "Qv2rayBase/Profile/KernelManager.hpp"
#include "Qv2rayBase/Profile/ProfileManager.hpp"
#include "ui/WidgetUIBase.hpp"

#include <QStyleFactory>

#define QV_MODULE_NAME "ConnectionItemWidget"

ConnectionItemWidget::ConnectionItemWidget(QWidget *parent) : QWidget(parent)
{
    setupUi(this);
    connect(QvBaselib->KernelManager(), &Qv2rayBase::Profile::KernelManager::OnConnected, this, &ConnectionItemWidget::OnConnected);
    connect(QvBaselib->KernelManager(), &Qv2rayBase::Profile::KernelManager::OnDisconnected, this, &ConnectionItemWidget::OnDisConnected);
    connect(QvBaselib->KernelManager(), &Qv2rayBase::Profile::KernelManager::OnStatsDataAvailable, this, &ConnectionItemWidget::OnConnectionStatsArrived);
    connect(QvBaselib->ProfileManager(), &Qv2rayBase::Profile::ProfileManager::OnLatencyTestStarted, this, &ConnectionItemWidget::OnLatencyTestStart);
    connect(QvBaselib->ProfileManager(), &Qv2rayBase::Profile::ProfileManager::OnLatencyTestFinished, this, &ConnectionItemWidget::OnLatencyTestFinished);
}

ConnectionItemWidget::ConnectionItemWidget(const ProfileId &id, QWidget *parent) : ConnectionItemWidget(parent)
{
    connectionId = id.connectionId;
    groupId = id.groupId;
    originalItemName = GetDisplayName(id.connectionId);
    //
    indentSpacer->changeSize(10, indentSpacer->sizeHint().height());
    //
    auto latency = GetConnectionLatency(id.connectionId);
    latencyLabel->setText(latency == LATENCY_TEST_VALUE_NODATA ?         //
                              tr("Not Tested") :                         //
                              (latency == LATENCY_TEST_VALUE_ERROR ?     //
                                   tr("Error") :                         //
                                   (QString::number(latency) + " ms"))); //

    connTypeLabel->setText(GetConnectionProtocolDescription(id.connectionId).toUpper());
    const auto [uplink, downlink] = GetConnectionUsageAmount(connectionId, StatisticsObject::PROXY);
    dataLabel->setText(FormatBytes(uplink) + " / " + FormatBytes(downlink));
    //
    if (QvBaselib->ProfileManager()->IsConnected(id))
    {
        emit RequestWidgetFocus(this);
    }
    // Fake trigger
    OnConnectionItemRenamed(id.connectionId, "", originalItemName);
    connect(QvBaselib->ProfileManager(), &Qv2rayBase::Profile::ProfileManager::OnConnectionRenamed, this, &ConnectionItemWidget::OnConnectionItemRenamed);
    //
    // Rename events
    connect(renameTxt, &QLineEdit::returnPressed, this, &ConnectionItemWidget::on_doRenameBtn_clicked);
    connect(QvBaselib->ProfileManager(), &Qv2rayBase::Profile::ProfileManager::OnConnectionModified, this, &ConnectionItemWidget::OnConnectionModified);
}

// ======================================= Initialisation for root nodes.
ConnectionItemWidget::ConnectionItemWidget(const GroupId &id, QWidget *parent) : ConnectionItemWidget(parent)
{
    layout()->removeWidget(connTypeLabel);
    layout()->removeWidget(dataLabel);
    delete connTypeLabel;
    delete dataLabel;
    //
    delete doRenameBtn;
    delete renameTxt;
    //
    groupId = id;
    originalItemName = GetDisplayName(id);
    RecalculateConnections();
    //
    auto font = connNameLabel->font();
    font.setBold(true);
    connNameLabel->setFont(font);
    //
    OnGroupItemRenamed(id, QLatin1String(""), originalItemName);
    connect(QvBaselib->ProfileManager(), &Qv2rayBase::Profile::ProfileManager::OnConnectionCreated, this, &ConnectionItemWidget::RecalculateConnections);
    connect(QvBaselib->ProfileManager(), &Qv2rayBase::Profile::ProfileManager::OnConnectionModified, this, &ConnectionItemWidget::RecalculateConnections);
    connect(QvBaselib->ProfileManager(), &Qv2rayBase::Profile::ProfileManager::OnConnectionLinkedWithGroup, this, &ConnectionItemWidget::RecalculateConnections);
    connect(QvBaselib->ProfileManager(), &Qv2rayBase::Profile::ProfileManager::OnSubscriptionAsyncUpdateFinished, this, &ConnectionItemWidget::RecalculateConnections);
    connect(QvBaselib->ProfileManager(), &Qv2rayBase::Profile::ProfileManager::OnConnectionRemovedFromGroup, this, &ConnectionItemWidget::RecalculateConnections);
    connect(QvBaselib->ProfileManager(), &Qv2rayBase::Profile::ProfileManager::OnGroupRenamed, this, &ConnectionItemWidget::OnGroupItemRenamed);
}

void ConnectionItemWidget::BeginConnection()
{
    if (IsConnection())
    {
        QvBaselib->ProfileManager()->StartConnection({ connectionId, groupId });
    }
    else
    {

        QvLog() << "Trying to start a non-connection entry, this call is illegal.";
    }
}

bool ConnectionItemWidget::NameMatched(const QString &arg) const
{
    auto searchString = arg.toLower();
    auto isGroupNameMatched = GetDisplayName(groupId).toLower().contains(arg);

    if (IsConnection())
    {
        return isGroupNameMatched || GetDisplayName(connectionId).toLower().contains(searchString);
    }
    else
    {
        return isGroupNameMatched;
    }
}

void ConnectionItemWidget::RecalculateConnections()
{
    auto connectionCount = QvBaselib->ProfileManager()->GetConnections(groupId).count();
    latencyLabel->setText(QString::number(connectionCount) + " " + (connectionCount < 2 ? tr("connection") : tr("connections")));
    OnGroupItemRenamed(groupId, "", originalItemName);
}

void ConnectionItemWidget::OnConnected(const ProfileId &id)
{
    if (id == ProfileId{ connectionId, groupId })
    {
        connNameLabel->setText("● " + originalItemName);
        QvDebug() << "ConnectionItemWidgetOnConnected signal received for:" << id.connectionId;
        emit RequestWidgetFocus(this);
    }
}

void ConnectionItemWidget::OnDisConnected(const ProfileId &id)
{
    if (id == ProfileId{ connectionId, groupId })
    {
        connNameLabel->setText(originalItemName);
    }
}

void ConnectionItemWidget::OnConnectionStatsArrived(const ProfileId &id, const StatisticsObject &data)
{
    if (id.connectionId == connectionId)
        dataLabel->setText(FormatBytes(data.proxyUp) + " / " + FormatBytes(data.proxyDown));
}

void ConnectionItemWidget::OnConnectionModified(const ConnectionId &id)
{
    if (connectionId == id)
        connTypeLabel->setText(GetConnectionProtocolDescription(id).toUpper());
}

void ConnectionItemWidget::OnLatencyTestStart(const ConnectionId &id)
{
    if (id == connectionId)
    {
        latencyLabel->setText(tr("Testing..."));
    }
}
void ConnectionItemWidget::OnLatencyTestFinished(const ConnectionId &id, const int average)
{
    if (id == connectionId)
    {
        latencyLabel->setText(average == LATENCY_TEST_VALUE_ERROR ? tr("Error") : QString::number(average) + tr("ms"));
    }
}

void ConnectionItemWidget::CancelRename()
{
    stackedWidget->setCurrentIndex(0);
}

void ConnectionItemWidget::BeginRename()
{
    if (IsConnection())
    {
        stackedWidget->setCurrentIndex(1);
        renameTxt->setStyle(QStyleFactory::create("Fusion"));
        renameTxt->setStyleSheet("background-color: " + this->palette().color(this->backgroundRole()).name(QColor::HexRgb));
        renameTxt->setText(originalItemName);
        renameTxt->setFocus();
    }
}

ConnectionItemWidget::~ConnectionItemWidget()
{
}

void ConnectionItemWidget::on_doRenameBtn_clicked()
{
    if (renameTxt->text().isEmpty())
        return;
    if (connectionId.isNull())
        QvBaselib->ProfileManager()->RenameGroup(groupId, renameTxt->text());
    else
        QvBaselib->ProfileManager()->RenameConnection(connectionId, renameTxt->text());
    stackedWidget->setCurrentIndex(0);
}

void ConnectionItemWidget::OnConnectionItemRenamed(const ConnectionId &id, const QString &, const QString &newName)
{
    if (id == connectionId)
    {
        connNameLabel->setText((QvBaselib->ProfileManager()->IsConnected({ connectionId, groupId }) ? "● " : "") + newName);
        originalItemName = newName;
        const auto conn = QvBaselib->ProfileManager()->GetConnectionObject(connectionId);
        this->setToolTip(newName + NEWLINE + tr("Last Connected: ") + TimeToString(conn.last_connected) + NEWLINE + tr("Last Updated: ") + TimeToString(conn.updated));
    }
}

void ConnectionItemWidget::OnGroupItemRenamed(const GroupId &id, const QString &, const QString &newName)
{
    if (id == groupId)
    {
        originalItemName = newName;
        connNameLabel->setText(newName);
        const auto grp = QvBaselib->ProfileManager()->GetGroupObject(id);
        this->setToolTip(newName + NEWLINE +                                                              //
                         (grp.subscription_config.isSubscription ? (tr("Subscription") + NEWLINE) : "") + //
                         tr("Last Updated: ") + TimeToString(grp.updated));
    }
}
