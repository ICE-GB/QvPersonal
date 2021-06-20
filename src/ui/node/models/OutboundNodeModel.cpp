#include "OutboundNodeModel.hpp"

#include "Common/ProfileHelpers.hpp"
#include "Profile/ProfileManager.hpp"
#include "ui/node/widgets/BalancerWidget.hpp"
#include "ui/node/widgets/ChainWidget.hpp"
#include "ui/node/widgets/InboundOutboundWidget.hpp"

#define QV_MODULE_NAME "Node::OutboundNodeModel"

OutboundNodeModel::OutboundNodeModel(std::shared_ptr<NodeDispatcher> _dispatcher, std::shared_ptr<node_data_t> data) : NodeDataModel()
{
    dataptr = data;
    dispatcher = _dispatcher;
    switch (data->objectType)
    {
        case OutboundObject::ORIGINAL:
        case OutboundObject::EXTERNAL:
        {
            widget = new InboundOutboundWidget(NodeItemType::OUTBOUND, dispatcher);
            ((InboundOutboundWidget *) widget)->setValue(data);
            break;
        }
        case OutboundObject::BALANCER:
        {
            widget = new BalancerWidget(dispatcher);
            ((BalancerWidget *) widget)->setValue(data);
            break;
        }
        case OutboundObject::CHAIN:
        {
            widget = new ChainWidget(dispatcher);
            ((ChainWidget *) widget)->setValue(data);
            break;
        }
    }
    connect(widget, &QvNodeWidget::OnSizeUpdated, this, &OutboundNodeModel::embeddedWidgetSizeUpdated);
    widget->setWindowFlags(Qt::FramelessWindowHint);
    widget->setAttribute(Qt::WA_TranslucentBackground);
}

void OutboundNodeModel::inputConnectionCreated(const QtNodes::Connection &){};
void OutboundNodeModel::inputConnectionDeleted(const QtNodes::Connection &){};
void OutboundNodeModel::outputConnectionCreated(const QtNodes::Connection &){};
void OutboundNodeModel::outputConnectionDeleted(const QtNodes::Connection &){};
void OutboundNodeModel::setInData(std::vector<std::shared_ptr<NodeData>> indata, PortIndex)
{
    if (dispatcher->IsNodeConstructing())
        return;
    for (const auto &d : indata)
    {
        if (!d)
        {
            QvLog() << "Invalid inbound nodedata to rule.";
            continue;
        }
        const auto rule = static_cast<RuleNodeData *>(d.get());
        if (!rule)
        {
            QvLog() << "Invalid rule nodedata to outbound.";
            return;
        }
        const auto rulePtr = rule->GetData();
        rulePtr->outboundTag = dataptr->name;
        QvLog() << "Connecting rule:" << rulePtr->name << "to" << dataptr->name;
    }
}

void OutboundNodeModel::onNodeHoverLeave(){};
void OutboundNodeModel::onNodeHoverEnter()
{
    if (dataptr->objectType == OutboundObject::ORIGINAL)
    {
        emit dispatcher->OnInboundOutboundNodeHovered(dataptr->name, GetOutboundInfo(*dataptr));
    }
    else if (dataptr->objectType == OutboundObject::EXTERNAL)
    {
        const auto root = QvBaselib->ProfileManager()->GetConnection(dataptr->externalId);
        emit dispatcher->OnInboundOutboundNodeHovered(dataptr->name, GetOutboundInfo(root.outbounds.first()));
    }
}
