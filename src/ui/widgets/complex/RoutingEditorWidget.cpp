#include "RoutingEditorWidget.hpp"

#include "ui/WidgetUIBase.hpp"
#include "ui/node/NodeBase.hpp"

constexpr auto GRAPH_GLOBAL_OFFSET_X = -100;
constexpr auto GRAPH_GLOBAL_OFFSET_Y = -100;

RoutingEditorWidget::RoutingEditorWidget(std::shared_ptr<NodeDispatcher> dispatcher, QWidget *parent) : QWidget(parent), dispatcher(dispatcher)
{
    setupUi(this);
    QvMessageBusConnect();
    //
    scene = new QtNodes::FlowScene(this);
    connect(dispatcher.get(), &NodeDispatcher::OnInboundCreated, this, &RoutingEditorWidget::OnDispatcherInboundCreated);
    connect(dispatcher.get(), &NodeDispatcher::OnOutboundCreated, this, &RoutingEditorWidget::OnDispatcherOutboundCreated);
    connect(dispatcher.get(), &NodeDispatcher::OnRuleCreated, this, &RoutingEditorWidget::OnDispatcherRuleCreated);
    //
    view = new QtNodes::FlowView(scene, this);
    view->scaleDown();

    if (!viewWidget->layout())
    {
        // The QWidget will take ownership of layout.
        viewWidget->setLayout(new QVBoxLayout());
    }
    auto l = viewWidget->layout();
    l->addWidget(view);
    l->setContentsMargins(0, 0, 0, 0);
    l->setSpacing(0);
}

void RoutingEditorWidget::updateColorScheme()
{
    addRouteBtn->setIcon(QIcon(STYLE_RESX("add")));
    delBtn->setIcon(QIcon(STYLE_RESX("ashbin")));
}

QvMessageBusSlotImpl(RoutingEditorWidget)
{
    switch (msg)
    {
        MBRetranslateDefaultImpl;
        MBUpdateColorSchemeDefaultImpl;
        default: break;
    }
}

void RoutingEditorWidget::changeEvent(QEvent *e)
{
    QWidget::changeEvent(e);
    switch (e->type())
    {
        case QEvent::LanguageChange: retranslateUi(this); break;
        default: break;
    }
}
void RoutingEditorWidget::OnDispatcherInboundCreated(std::shared_ptr<InboundObject>, QtNodes::Node &node)
{
    QPoint pos{ 0 + GRAPH_GLOBAL_OFFSET_X, dispatcher->InboundsCount() * 130 + GRAPH_GLOBAL_OFFSET_Y };
    scene->setNodePosition(node, pos);
}

void RoutingEditorWidget::OnDispatcherOutboundCreated(std::shared_ptr<OutboundObject>, QtNodes::Node &node)
{
    auto pos = this->pos();
    pos.setX(pos.x() + 850 + GRAPH_GLOBAL_OFFSET_X);
    pos.setY(pos.y() + dispatcher->OutboundsCount() * 120 + GRAPH_GLOBAL_OFFSET_Y);
    scene->setNodePosition(node, pos);
}

void RoutingEditorWidget::OnDispatcherRuleCreated(std::shared_ptr<RuleObject>, QtNodes::Node &node)
{
    auto pos = this->pos();
    pos.setX(pos.x() + 350 + GRAPH_GLOBAL_OFFSET_X);
    pos.setY(pos.y() + dispatcher->RulesCount() * 120 + GRAPH_GLOBAL_OFFSET_Y);
    scene->setNodePosition(node, pos);
}

void RoutingEditorWidget::on_addRouteBtn_clicked()
{
    const auto _ = dispatcher->CreateRule(RuleObject{});
}

void RoutingEditorWidget::on_delBtn_clicked()
{
    if (scene->selectedNodes().empty())
    {
        QvBaselib->Warn(tr("Remove Items"), tr("Please select a node from the graph to continue."));
        return;
    }

    const auto selecteNodes = scene->selectedNodes();
    if (selecteNodes.empty())
    {
        QvBaselib->Warn(tr("Deleting a node"), tr("You need to select a node first"));
        return;
    }
    scene->removeNode(*selecteNodes.front());
}
