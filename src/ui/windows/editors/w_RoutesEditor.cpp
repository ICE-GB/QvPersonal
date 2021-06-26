#include "w_RoutesEditor.hpp"

#include "Common/ProfileHelpers.hpp"
#include "Plugin/PluginAPIHost.hpp"
#include "Profile/ProfileManager.hpp"
#include "ui/node/NodeBase.hpp"
#include "ui/node/models/InboundNodeModel.hpp"
#include "ui/node/models/OutboundNodeModel.hpp"
#include "ui/node/models/RuleNodeModel.hpp"
#include "ui/widgets/complex/ChainEditorWidget.hpp"
#include "ui/widgets/complex/RoutingEditorWidget.hpp"
#include "ui/widgets/editors/DnsSettingsWidget.hpp"
#include "ui/windows/w_ImportConfig.hpp"
#include "w_InboundEditor.hpp"
#include "w_JsonEditor.hpp"
#include "w_OutboundEditor.hpp"

#include <nodes/ConnectionStyle>
#include <nodes/FlowScene>
#include <nodes/FlowView>
#include <nodes/FlowViewStyle>
#include <nodes/Node>

#ifdef QT_DEBUG
#include "nodes/../../src/ConnectionPainter.hpp"
#endif

#define QV_MODULE_NAME "RouteEditor"

using namespace QtNodes;
using namespace Qv2ray::ui::nodemodels;

namespace
{
    constexpr auto NODE_TAB_ROUTE_EDITOR = 0;
    constexpr auto NODE_TAB_CHAIN_EDITOR = 1;
    constexpr auto DarkConnectionStyle = R"({"ConnectionStyle": {"ConstructionColor": "gray","NormalColor": "black","SelectedColor": "gray",
                                         "SelectedHaloColor": "deepskyblue","HoveredColor": "deepskyblue","LineWidth": 3.0,
                                         "ConstructionLineWidth": 2.0,"PointDiameter": 10.0,"UseDataDefinedColors": true}})";
    constexpr auto LightNodeStyle = R"({"NodeStyle": {"NormalBoundaryColor": "darkgray","SelectedBoundaryColor": "deepskyblue",
                                    "GradientColor0": "mintcream","GradientColor1": "mintcream","GradientColor2": "mintcream",
                                    "GradientColor3": "mintcream","ShadowColor": [200, 200, 200],"FontColor": [10, 10, 10],
                                    "FontColorFaded": [100, 100, 100],"ConnectionPointColor": "white","PenWidth": 2.0,"HoveredPenWidth": 2.5,
                                    "ConnectionPointDiameter": 10.0,"Opacity": 1.0}})";
    constexpr auto LightViewStyle = R"({"FlowViewStyle": {"BackgroundColor": [255, 255, 240],"FineGridColor": [245, 245, 230],"CoarseGridColor": [235, 235, 220]}})";
    constexpr auto LightConnectionStyle = R"({"ConnectionStyle": {"ConstructionColor": "gray","NormalColor": "black","SelectedColor": "gray",
                                          "SelectedHaloColor": "deepskyblue","HoveredColor": "deepskyblue","LineWidth": 3.0,"ConstructionLineWidth": 2.0,
                                          "PointDiameter": 10.0,"UseDataDefinedColors": false}})";
    constexpr auto IMPORT_ALL_CONNECTIONS_FAKE_ID = "__ALL_CONNECTIONS__";
    constexpr auto IMPORT_ALL_CONNECTIONS_SEPARATOR = "_";

} // namespace

#define LOADINGCHECK                                                                                                                                                     \
    if (isLoading)                                                                                                                                                       \
        return;
#define LOAD_FLAG_BEGIN isLoading = true;
#define LOAD_FLAG_END isLoading = false;

void RouteEditor::updateColorScheme()
{
    // Setup icons according to the theme Settings->
    addInboundBtn->setIcon(QIcon(STYLE_RESX("add")));
    addOutboundBtn->setIcon(QIcon(STYLE_RESX("add")));
    if (StyleManager->isDarkMode())
    {
        QtNodes::NodeStyle::reset();
        QtNodes::FlowViewStyle::reset();
        ConnectionStyle::setConnectionStyle(DarkConnectionStyle);
    }
    else
    {
        QtNodes::NodeStyle::setNodeStyle(LightNodeStyle);
        QtNodes::FlowViewStyle::setStyle(LightViewStyle);
        ConnectionStyle::setConnectionStyle(LightConnectionStyle);
    }
}

RouteEditor::RouteEditor(ProfileContent connection, QWidget *parent) : QvDialog("RouteEditor", parent), root(connection), original(connection)
{
    setupUi(this);
    QvMessageBusConnect();
    //
    isLoading = true;
    setWindowFlags(windowFlags() | Qt::WindowMaximizeButtonHint);
    RouteEditor::updateColorScheme();
    //
    // Do not change the order.
    nodeDispatcher = std::make_shared<NodeDispatcher>();
    ruleWidget = new RoutingEditorWidget(nodeDispatcher, ruleEditorUIWidget);
    chainWidget = new ChainEditorWidget(nodeDispatcher, chainEditorUIWidget);
    dnsWidget = new DnsSettingsWidget(this);
    nodeDispatcher->InitializeScenes(ruleWidget->getScene(), chainWidget->getScene());
    connect(nodeDispatcher.get(), &NodeDispatcher::OnOutboundCreated, this, &RouteEditor::OnDispatcherOutboundCreated);
    connect(nodeDispatcher.get(), &NodeDispatcher::OnOutboundDeleted, this, &RouteEditor::OnDispatcherOutboundDeleted);
    connect(nodeDispatcher.get(), &NodeDispatcher::OnRuleCreated, this, &RouteEditor::OnDispatcherRuleCreated);
    connect(nodeDispatcher.get(), &NodeDispatcher::OnRuleDeleted, this, &RouteEditor::OnDispatcherRuleDeleted);
    connect(nodeDispatcher.get(), &NodeDispatcher::OnInboundOutboundNodeHovered, this, &RouteEditor::OnDispatcherInboundOutboundHovered);
    connect(nodeDispatcher.get(), &NodeDispatcher::RequestEditChain, this, &RouteEditor::OnDispatcherEditChainRequested);
    connect(nodeDispatcher.get(), &NodeDispatcher::OnObjectTagChanged, this, &RouteEditor::OnDispatcherObjectTagChanged);

    const auto SetUpLayout = [](QWidget *parent, QWidget *child) {
        if (!parent->layout())
            parent->setLayout(new QVBoxLayout());
        auto l = parent->layout();
        l->addWidget(child);
        l->setContentsMargins(0, 0, 0, 0);
        l->setSpacing(0);
    };

    SetUpLayout(ruleEditorUIWidget, ruleWidget);
    SetUpLayout(chainEditorUIWidget, chainWidget);
    SetUpLayout(dnsEditorUIWidget, dnsWidget);
    //
    nodeDispatcher->LoadFullConfig(root);
    dnsWidget->SetDNSObject(DNSObject::fromJson(root.extraOptions["dns"].toObject()), FakeDNSObject::fromJson(root.extraOptions["fakedns"].toObject()));
    //
    domainStrategy = root.routing.options[QStringLiteral("domainStrategy")].toString();
    domainStrategyCombo->setCurrentText(domainStrategy);
    //
    // Set default outboung combo text AFTER adding all outbounds.

    if (!root.outbounds.isEmpty())
        defaultOutboundTag = root.outbounds.first().name;
    defaultOutboundCombo->setCurrentText(defaultOutboundTag);
    //
    const auto browserForwarder = root.extraOptions[QStringLiteral("browserForwarder")].toObject();
    bfListenIPTxt->setText(browserForwarder[QStringLiteral("listenAddr")].toString());
    bfListenPortTxt->setValue(browserForwarder[QStringLiteral("listenPort")].toInt());

    const auto observatory = root.extraOptions[QStringLiteral("observatory")].toObject();
    obSubjectSelectorTxt->setPlainText(observatory[QStringLiteral("subjectSelector")].toVariant().toStringList().join(NEWLINE));

    for (const auto &group : QvBaselib->ProfileManager()->GetGroups())
    {
        importGroupBtn->addItem(GetDisplayName(group), group.toString());
    }

#ifndef QT_DEBUG
    debugPainterCB->setVisible(false);
#endif

    isLoading = false;
}

QvMessageBusSlotImpl(RouteEditor)
{
    switch (msg)
    {
        MBShowDefaultImpl;
        MBHideDefaultImpl;
        MBRetranslateDefaultImpl;
        MBUpdateColorSchemeDefaultImpl;
    }
}

void RouteEditor::OnDispatcherInboundOutboundHovered(const QString &tag, const IOBoundData &info)
{
    tagLabel->setText(tag);
    protocolLabel->setText(std::get<0>(info));
    hostLabel->setText(std::get<1>(info));
    portLabel->setNum(std::get<2>(info).from);
}

void RouteEditor::OnDispatcherOutboundCreated(std::shared_ptr<OutboundObject> out, QtNodes::Node &)
{
    if (out->objectType != OutboundObject::BALANCER)
        defaultOutboundCombo->addItem(out->name);
}

void RouteEditor::OnDispatcherRuleCreated(std::shared_ptr<RuleObject> rule, QtNodes::Node &)
{
    ruleListWidget->addItem(rule->name);
}

void RouteEditor::OnDispatcherRuleDeleted(const RuleObject &rule)
{
    const auto items = ruleListWidget->findItems(rule.name, Qt::MatchExactly);
    if (!items.isEmpty())
        ruleListWidget->takeItem(ruleListWidget->row(items.first()));
}

void RouteEditor::OnDispatcherOutboundDeleted(const OutboundObject &data)
{
    const auto id = defaultOutboundCombo->findText(data.name);
    if (id >= 0)
    {
        defaultOutboundCombo->removeItem(id);
    }
}

void RouteEditor::OnDispatcherObjectTagChanged(const NodeItemType &t, const QString original, const QString current)
{
    Q_UNUSED(original)
    Q_UNUSED(current)
    //
    if (t == NodeItemType::INBOUND)
    {
        // Pass
    }
    else if (t == NodeItemType::RULE)
    {
        for (auto i = 0; i < ruleListWidget->count(); i++)
        {
            if (ruleListWidget->item(i)->text() == original)
                ruleListWidget->item(i)->setText(current);
        }
    }
    else if (t == NodeItemType::OUTBOUND)
    {
        const auto id = defaultOutboundCombo->findText(original);
        if (id >= 0)
            defaultOutboundCombo->setItemText(id, current);
    }
}

void RouteEditor::OnDispatcherEditChainRequested(const QString &)
{
    nodesTab->setCurrentIndex(NODE_TAB_CHAIN_EDITOR);
}

ProfileContent RouteEditor::OpenEditor()
{
    auto result = this->exec();
    if (result != QDialog::Accepted)
        return original;

    const auto &[m_inbounds, m_rules, m_outbounds] = nodeDispatcher->GetData();
#pragma message("TODO: Process keys")
    root.inbounds = m_inbounds.values();
    //
    // QJsonArray rules
    QList<RuleObject> rules;
    for (auto i = 0; i < ruleListWidget->count(); i++)
    {
        const auto ruleTag = ruleListWidget->item(i)->text();
        rules << m_rules[ruleTag];
    }

    root.routing.options[QStringLiteral("domainStrategy")] = domainStrategy;
    root.routing.rules = rules;

    root.outbounds.clear();
    for (const auto &out : m_outbounds)
    {
        if (out.name == defaultOutboundTag)
            root.outbounds.prepend(out);
        else
            root.outbounds.append(out);
    }
    // Process DNS
    const auto &[dns, fakedns] = dnsWidget->GetDNSObject();
    root.extraOptions[QStringLiteral("dns")] = dns.toJson();
    root.extraOptions[QStringLiteral("fakedns")] = fakedns.toJson();
    {
        // Process Browser Forwarder
        QJsonObject browserForwarder;
        browserForwarder[QStringLiteral("listenAddr")] = bfListenIPTxt->text();
        browserForwarder[QStringLiteral("listenPort")] = bfListenPortTxt->value();
        root.extraOptions[QStringLiteral("browserForwarder")] = browserForwarder;
    }

    // Process Observatory
    if (!obSubjectSelectorTxt->toPlainText().isEmpty())
    {
        QJsonObject observatory;
        observatory[QStringLiteral("subjectSelector")] = QJsonArray::fromStringList(SplitLines(obSubjectSelectorTxt->toPlainText()));
        root.extraOptions[QStringLiteral("observatory")] = observatory;
    }
    return root;
}

RouteEditor::~RouteEditor()
{
    nodeDispatcher->LockOperation();
}

void RouteEditor::on_insertDirectBtn_clicked()
{
    auto freedom = IOProtocolSettings{ QJsonObject{ { "domainStrategy", "AsIs" } } };
    auto tag = "Freedom_" + QString::number(QTime::currentTime().msecsSinceStartOfDay());
    OutboundObject out;
    out.name = tag;
    out.outboundSettings.protocol = QStringLiteral("freedom");
    out.outboundSettings.protocolSettings = freedom;
    const auto _ = nodeDispatcher->CreateOutbound(out);
    Q_UNUSED(_)
    statusLabel->setText(tr("Added DIRECT outbound"));
}

void RouteEditor::on_addDefaultBtn_clicked()
{
    LOADINGCHECK
    // Add default connection from GlobalConfig
    //
    const auto &inConfig = GlobalConfig->inboundConfig;
    const static QJsonObject sniffingOff{ { "enabled", false } };
    const static QJsonObject sniffingOn{ { "enabled", true }, { "destOverride", QJsonArray{ "http", "tls" } } };
    //
#pragma message("TODO: Metadata only sniffing.")
    if (inConfig->HasHTTP)
    {
        const auto httpIn = InboundObject::Create("HTTP (Global)", "http", inConfig->ListenAddress, inConfig->HTTPConfig->ListenPort);

        httpIn.options[QStringLiteral("sniffing")] = inConfig->SOCKSConfig->Sniffing ? sniffingOn : sniffingOff;
        const auto _ = nodeDispatcher->CreateInbound(httpIn);
        Q_UNUSED(_)
    }
    if (inConfig->HasSOCKS)
    {
        const IOProtocolSettings socks{ QJsonObject{ { "udp", *inConfig->SOCKSConfig->EnableUDP }, { "ip", *inConfig->SOCKSConfig->UDPLocalAddress } } };

        const auto socksIn = InboundObject::Create("Socks (Global)", "socks", inConfig->ListenAddress, inConfig->SOCKSConfig->ListenPort, socks);
        socksIn.options[QStringLiteral("sniffing")] = inConfig->SOCKSConfig->Sniffing ? sniffingOn : sniffingOff;
        const auto _ = nodeDispatcher->CreateInbound(socksIn);
    }

    if (inConfig->HasDokodemoDoor)
    {
#define ts inConfig->DokodemoDoorConfig
        const IOProtocolSettings tproxyInSettings{ QJsonObject{ { "network", "tcp,udp" }, { "followRedirect", true } } };
        const IOStreamSettings streamSettings{ QJsonObject{ { "sockopt", QJsonObject{ { "tproxy", *ts->WorkingMode } } } } };

        {
            const auto tProxyIn = InboundObject::Create(QStringLiteral("tProxy"),                   //
                                                        QStringLiteral("dokodemo-door"),            //
                                                        GlobalConfig->inboundConfig->ListenAddress, //
                                                        ts->ListenPort,                             //
                                                        tproxyInSettings,                           //
                                                        streamSettings);
            tProxyIn.options[QStringLiteral("sniffing")] = sniffingOn;
            auto _ = nodeDispatcher->CreateInbound(tProxyIn);
            Q_UNUSED(_)
        }
        if (!GlobalConfig->inboundConfig->ListenAddressV6->isEmpty())
        {
            const auto tProxyV6In = InboundObject::Create(QStringLiteral("tProxy IPv6"),                //
                                                          QStringLiteral("dokodemo-door"),              //
                                                          GlobalConfig->inboundConfig->ListenAddressV6, //
                                                          ts->ListenPort,                               //
                                                          tproxyInSettings,                             //
                                                          streamSettings);
            tProxyV6In.options[QStringLiteral("sniffing")] = sniffingOn;
            auto _ = nodeDispatcher->CreateInbound(tProxyV6In);
            Q_UNUSED(_)
        }
#undef ts
    }
}

void RouteEditor::on_insertBlackBtn_clicked()
{
    LOADINGCHECK
    auto tag = "BlackHole-" + GenerateRandomString(5);
    OutboundObject outbound;
    outbound.name = tag;
    outbound.outboundSettings.protocol = QStringLiteral("blackhole");
    const auto _ = nodeDispatcher->CreateOutbound(outbound);
    Q_UNUSED(_)
}

void RouteEditor::on_addInboundBtn_clicked()
{
    LOADINGCHECK
    InboundEditor w(InboundObject(), this);
    auto _result = w.OpenEditor();

    if (w.result() == QDialog::Accepted)
    {
        auto _ = nodeDispatcher->CreateInbound(_result);
        Q_UNUSED(_)
    }
}

void RouteEditor::on_addOutboundBtn_clicked()
{
    LOADINGCHECK
    OutboundEditor w(OutboundObject{}, this);
    auto _result = w.OpenEditor();

    if (w.result() == QDialog::Accepted)
    {
        auto _ = nodeDispatcher->CreateOutbound(_result);
    }
}

void RouteEditor::on_domainStrategyCombo_currentIndexChanged(int arg1)
{
    LOADINGCHECK
    domainStrategy = domainStrategyCombo->itemText(arg1);
}

void RouteEditor::on_defaultOutboundCombo_currentTextChanged(const QString &arg1)
{
    LOADINGCHECK
    if (defaultOutboundTag != arg1)
    {
        QvLog() << "Default outbound changed:" << arg1;
        defaultOutboundTag = arg1;
    }
}

void RouteEditor::on_importExistingBtn_clicked()
{
    const auto ImportConnection = [this](const ConnectionId &_id)
    {
        const auto root = QvBaselib->ProfileManager()->GetConnection(_id);
        auto outbound = root.outbounds.first();
        outbound.name = GetDisplayName(_id);
        auto _ = nodeDispatcher->CreateOutbound(outbound);
        Q_UNUSED(_)
    };

    const auto cid = ConnectionId{ importConnBtn->currentData(Qt::UserRole).toString() };
    if (cid.toString() == IMPORT_ALL_CONNECTIONS_SEPARATOR)
        return;
    if (cid.toString() == IMPORT_ALL_CONNECTIONS_FAKE_ID)
    {
        const auto group = GroupId{ importGroupBtn->currentData(Qt::UserRole).toString() };
        if (QvBaselib->Ask(tr("Importing All Connections"), tr("Do you want to import all the connections?")) != Qv2rayBase::MessageOpt::Yes)
            return;
        for (const auto &connId : QvBaselib->ProfileManager()->GetConnections(group))
        {
            ImportConnection(connId);
        }
        return;
    }
    else
    {
        ImportConnection(cid);
    }
}

void RouteEditor::on_linkExistingBtn_clicked()
{
    const auto ImportConnection = [this](const ConnectionId &id)
    {
        const auto root = QvBaselib->ProfileManager()->GetConnection(id);
        if (root.outbounds.size() > 0)
            Q_UNUSED(nodeDispatcher->CreateOutbound(root.outbounds.constFirst()));
    };

    const auto cid = ConnectionId{ importConnBtn->currentData(Qt::UserRole).toString() };
    if (cid.toString() == IMPORT_ALL_CONNECTIONS_SEPARATOR)
        return;
    if (cid.toString() == IMPORT_ALL_CONNECTIONS_FAKE_ID)
    {
        const auto group = GroupId{ importGroupBtn->currentData(Qt::UserRole).toString() };
        if (QvBaselib->Ask(tr("Importing All Connections"), tr("Do you want to import all the connections?")) != Qv2rayBase::MessageOpt::Yes)
            return;
        for (const auto &connId : QvBaselib->ProfileManager()->GetConnections(group))
        {
            ImportConnection(connId);
        }
        return;
    }
    else
    {
        ImportConnection(cid);
    }
}

void RouteEditor::on_importGroupBtn_currentIndexChanged(int)
{
    const auto group = GroupId{ importGroupBtn->currentData(Qt::UserRole).toString() };
    importConnBtn->clear();
    for (const auto &connId : QvBaselib->ProfileManager()->GetConnections(group))
    {
        importConnBtn->addItem(GetDisplayName(connId), connId.toString());
    }
    importConnBtn->addItem("————————————————", IMPORT_ALL_CONNECTIONS_SEPARATOR);
    importConnBtn->addItem(tr("(All Connections)"), IMPORT_ALL_CONNECTIONS_FAKE_ID);
}

void RouteEditor::on_addBalancerBtn_clicked()
{
#pragma message("TODO: Outbound")
    Q_UNUSED(nodeDispatcher->CreateOutbound({}));
}

void RouteEditor::on_addChainBtn_clicked()
{
    Q_UNUSED(nodeDispatcher->CreateOutbound({}));
}

void RouteEditor::on_debugPainterCB_clicked(bool checked)
{
#ifdef QT_DEBUG
    QtNodes::ConnectionPainter::IsDebuggingEnabled = checked;
    ruleWidget->getScene()->update();
    chainWidget->getScene()->update();
#endif
}

void RouteEditor::on_importOutboundBtn_clicked()
{
    LOADINGCHECK
    ImportConfigWindow w(this);
    // True here for not keep the inbounds.
    auto [_, configs] = w.DoImportConnections();

    for (auto it = configs.constKeyValueBegin(); it != configs.constKeyValueEnd(); it++)
    {
        const auto conf = it->second;
        const auto name = it->first;

        if (name.isEmpty())
            continue;

        // conf is rootObject, needs to unwrap it.
        const auto confList = conf.outbounds;
        for (int i = 0; i < confList.count(); i++)
        {
            auto _ = nodeDispatcher->CreateOutbound(confList[i]);
            Q_UNUSED(_)
        }
    }
}
