#include "ConnectionInfoWidget.hpp"

#include "Common/ProfileHelpers.hpp"
#include "Common/Utils.hpp"
#include "Profile/KernelManager.hpp"
#include "Profile/ProfileManager.hpp"
#include "QRCodeHelper/QRCodeHelper.hpp"
#include "ui/WidgetUIBase.hpp"

constexpr auto INDEX_CONNECTION = 0;
constexpr auto INDEX_GROUP = 1;

QvMessageBusSlotImpl(ConnectionInfoWidget)
{
    switch (msg)
    {
        MBRetranslateDefaultImpl;
        MBUpdateColorSchemeDefaultImpl;
        case MessageBus::HIDE_WINDOWS:
        case MessageBus::SHOW_WINDOWS: break;
    }
}

void ConnectionInfoWidget::updateColorScheme()
{
    latencyBtn->setIcon(QIcon(STYLE_RESX("ping_gauge")));
    deleteBtn->setIcon(QIcon(STYLE_RESX("ashbin")));
    editBtn->setIcon(QIcon(STYLE_RESX("edit")));
    editJsonBtn->setIcon(QIcon(STYLE_RESX("code")));
    shareLinkTxt->setStyleSheet("border-bottom: 1px solid gray; border-radius: 0px; padding: 2px; background-color: " +
                                this->palette().color(this->backgroundRole()).name(QColor::HexRgb));
    groupSubsLinkTxt->setStyleSheet("border-bottom: 1px solid gray; border-radius: 0px; padding: 2px; background-color: " +
                                    this->palette().color(this->backgroundRole()).name(QColor::HexRgb));

    qrLabel->setPixmap(IsComplexConfig(connectionId) ? QvApp->Qv2rayLogo : (isRealPixmapShown ? qrPixmap : qrPixmapBlured));

    const auto isCurrentItem = QvBaselib->KernelManager()->CurrentConnection().connectionId == connectionId;
    connectBtn->setIcon(QIcon(isCurrentItem ? STYLE_RESX("stop") : STYLE_RESX("start")));
}

ConnectionInfoWidget::ConnectionInfoWidget(QWidget *parent) : QWidget(parent)
{
    setupUi(this);

    QvMessageBusConnect();
    updateColorScheme();

    shareLinkTxt->setAutoFillBackground(true);
    shareLinkTxt->setCursor(QCursor(Qt::CursorShape::IBeamCursor));
    shareLinkTxt->installEventFilter(this);
    groupSubsLinkTxt->installEventFilter(this);
    qrLabel->installEventFilter(this);
    qrLabel->setScaledContents(true);

    connect(QvBaselib->ProfileManager(), &Qv2rayBase::Profile::ProfileManager::OnConnected, this, &ConnectionInfoWidget::OnConnected);
    connect(QvBaselib->ProfileManager(), &Qv2rayBase::Profile::ProfileManager::OnDisconnected, this, &ConnectionInfoWidget::OnDisConnected);
    connect(QvBaselib->ProfileManager(), &Qv2rayBase::Profile::ProfileManager::OnGroupRenamed, this, &ConnectionInfoWidget::OnGroupRenamed);
    connect(QvBaselib->ProfileManager(), &Qv2rayBase::Profile::ProfileManager::OnConnectionModified, this, &ConnectionInfoWidget::OnConnectionModified);
    connect(QvBaselib->ProfileManager(), &Qv2rayBase::Profile::ProfileManager::OnConnectionLinkedWithGroup, this, &ConnectionInfoWidget::OnConnectionModified_Pair);
    connect(QvBaselib->ProfileManager(), &Qv2rayBase::Profile::ProfileManager::OnConnectionRemovedFromGroup, this, &ConnectionInfoWidget::OnConnectionModified_Pair);
}

void ConnectionInfoWidget::ShowDetails(const ProfileId &idpair)
{
    this->groupId = idpair.groupId;
    this->connectionId = idpair.connectionId;
    bool isConnection = !connectionId.isNull();
    //
    editBtn->setEnabled(isConnection);
    editJsonBtn->setEnabled(isConnection);
    connectBtn->setEnabled(isConnection);
    stackedWidget->setCurrentIndex(isConnection ? INDEX_CONNECTION : INDEX_GROUP);
    if (isConnection)
    {
        auto shareLink = ConvertConfigToString(idpair.connectionId);
        if (shareLink)
        {
            qrPixmap = QPixmap::fromImage(EncodeQRCode(*shareLink, qrLabel->width() * devicePixelRatio()));
            shareLinkTxt->setText(*shareLink);
        }
        shareLinkTxt->setCursorPosition(0);
        protocolLabel->setText(GetConnectionProtocolDescription(connectionId));
        groupLabel->setText(GetDisplayName(groupId));

        if (IsComplexConfig(connectionId))
        {
            qrLabel->setPixmap(QvApp->Qv2rayLogo);
        }
        else
        {
            const auto root = QvBaselib->ProfileManager()->GetConnection(connectionId);
            if (!root.outbounds.isEmpty())
            {
                const auto &[protocol, host, port] = GetOutboundInfo(root.outbounds.first());
                Q_UNUSED(protocol)
                addressLabel->setText(host);
                portLabel->setNum(port.from);
            }
            qrPixmapBlured = BlurImage(ColorizeImage(qrPixmap, StyleManager->isDarkMode() ? QColor(Qt::black) : QColor(Qt::white), 0.7), 35);
            qrLabel->setPixmap(qrPixmapBlured);
        }

        const auto isCurrentItem = QvBaselib->KernelManager()->CurrentConnection().connectionId == connectionId;
        connectBtn->setIcon(QIcon(isCurrentItem ? STYLE_RESX("stop") : STYLE_RESX("start")));

        isRealPixmapShown = false;
    }
    else
    {
        connectBtn->setIcon(QIcon(STYLE_RESX("start")));
        groupNameLabel->setText(GetDisplayName(groupId));

        QStringList shareLinks;
        for (const auto &connection : QvBaselib->ProfileManager()->GetConnections(groupId))
        {
            const auto link = ConvertConfigToString(connection);
            if (link)
                shareLinks.append(*link);
        }

        groupShareTxt->setPlainText(shareLinks.join(NEWLINE));
        const auto &groupMetaData = QvBaselib->ProfileManager()->GetGroupObject(groupId);
        groupSubsLinkTxt->setText(groupMetaData.subscription_config.isSubscription ? groupMetaData.subscription_config.address : tr("Not a subscription"));
    }
}

ConnectionInfoWidget::~ConnectionInfoWidget()
{
}

void ConnectionInfoWidget::OnConnectionModified(const ConnectionId &id)
{
    if (id == this->connectionId)
        ShowDetails({ id, groupId });
}

void ConnectionInfoWidget::OnConnectionModified_Pair(const ProfileId &id)
{
    if (id.connectionId == this->connectionId && id.groupId == this->groupId)
        ShowDetails(id);
}
void ConnectionInfoWidget::OnGroupRenamed(const GroupId &id, const QString &oldName, const QString &newName)
{
    Q_UNUSED(oldName)
    if (this->groupId == id)
    {
        groupNameLabel->setText(newName);
        groupLabel->setText(newName);
    }
}

void ConnectionInfoWidget::on_connectBtn_clicked()
{
    if (QvBaselib->ProfileManager()->IsConnected({ connectionId, groupId }))
    {
        QvBaselib->ProfileManager()->StopConnection();
    }
    else
    {
        QvBaselib->ProfileManager()->StartConnection({ connectionId, groupId });
    }
}

void ConnectionInfoWidget::on_editBtn_clicked()
{
    emit OnEditRequested(connectionId);
}

void ConnectionInfoWidget::on_editJsonBtn_clicked()
{
    emit OnJsonEditRequested(connectionId);
}

void ConnectionInfoWidget::on_deleteBtn_clicked()
{
    if (QvBaselib->Ask(tr("Delete an item"), tr("Are you sure to delete the current item?")) == Qv2rayBase::MessageOpt::Yes)
    {
        if (!connectionId.isNull())
            QvBaselib->ProfileManager()->RemoveFromGroup(connectionId, groupId);
        else
            QvBaselib->ProfileManager()->DeleteGroup(groupId, false);
    }
}

bool ConnectionInfoWidget::eventFilter(QObject *object, QEvent *event)
{
    if (shareLinkTxt->underMouse() && event->type() == QEvent::MouseButtonRelease)
    {
        if (!shareLinkTxt->hasSelectedText())
            shareLinkTxt->selectAll();
    }
    else if (groupSubsLinkTxt->underMouse() && event->type() == QEvent::MouseButtonRelease)
    {
        if (!groupSubsLinkTxt->hasSelectedText())
            groupSubsLinkTxt->selectAll();
    }
    else if (qrLabel->underMouse() && event->type() == QEvent::MouseButtonRelease)
    {
        qrLabel->setPixmap(IsComplexConfig(connectionId) ? QvApp->Qv2rayLogo : (isRealPixmapShown ? qrPixmapBlured : qrPixmap));
        isRealPixmapShown = !isRealPixmapShown;
    }

    return QWidget::eventFilter(object, event);
}

void ConnectionInfoWidget::OnConnected(const ProfileId &id)
{
    if (id == ProfileId{ connectionId, groupId })
    {
        connectBtn->setIcon(QIcon(STYLE_RESX("stop")));
    }
}

void ConnectionInfoWidget::OnDisConnected(const ProfileId &id)
{
    if (id == ProfileId{ connectionId, groupId })
    {
        connectBtn->setIcon(QIcon(STYLE_RESX("start")));
    }
}

void ConnectionInfoWidget::on_latencyBtn_clicked()
{
    if (!connectionId.isNull())
    {
        QvBaselib->ProfileManager()->StartLatencyTest(connectionId, GlobalConfig->behaviorConfig->DefaultLatencyTestEngine);
    }
    else
    {
        QvBaselib->ProfileManager()->StartLatencyTest(groupId, GlobalConfig->behaviorConfig->DefaultLatencyTestEngine);
    }
}
