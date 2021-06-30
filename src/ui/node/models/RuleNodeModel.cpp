#include "RuleNodeModel.hpp"

#include "Qv2rayBase/Common/ProfileHelpers.hpp"
#include "ui/node/widgets/RuleWidget.hpp"

#define QV_MODULE_NAME "Node::RuleNodeModel"

RuleNodeModel::RuleNodeModel(std::shared_ptr<NodeDispatcher> _dispatcher, std::shared_ptr<node_data_t> data) : NodeDataModel()
{
    dataptr = data;
    dispatcher = _dispatcher;
    widget = new QvNodeRuleWidget(dispatcher);
    connect(widget, &QvNodeWidget::OnSizeUpdated, this, &NodeDataModel::embeddedWidgetSizeUpdated);
    ((QvNodeRuleWidget *) widget)->setValue(data);
    widget->setWindowFlags(Qt::FramelessWindowHint);
    widget->setAttribute(Qt::WA_TranslucentBackground);
    //
    const auto renameFunc = [this](NodeItemType mode, const QString originalTag, const QString newTag)
    {
        if (mode == NodeItemType::INBOUND)
        {
            if (dataptr->inboundTags.contains(originalTag))
            {
                dataptr->inboundTags.removeAll(originalTag);
                dataptr->inboundTags.push_back(newTag);
            }
        }
        else if (mode == NodeItemType::OUTBOUND)
        {
            dataptr->outboundTag = newTag;
        }
    };
    connect(dispatcher.get(), &NodeDispatcher::OnObjectTagChanged, renameFunc);
}

void RuleNodeModel::inputConnectionCreated(const QtNodes::Connection &){};
void RuleNodeModel::inputConnectionDeleted(const QtNodes::Connection &c)
{
    if (dispatcher->IsNodeConstructing())
        return;
    const auto &inNodeData = convert_nodedata<InboundNodeData>(c.getNode(PortType::Out));
    const auto inboundTag = inNodeData->GetData()->name;
    dataptr->inboundTags.removeAll(inboundTag);
}

void RuleNodeModel::outputConnectionCreated(const QtNodes::Connection &){};
void RuleNodeModel::outputConnectionDeleted(const QtNodes::Connection &)
{
    if (dispatcher->IsNodeConstructing())
        return;
    dataptr->outboundTag.clear();
}

void RuleNodeModel::setInData(std::vector<std::shared_ptr<NodeData>> indata, PortIndex)
{
    if (dispatcher->IsNodeConstructing())
        return;
    dataptr->inboundTags.clear();
    for (const auto &d : indata)
    {
        if (!d)
        {
            QvLog() << "Invalid inbound nodedata to rule.";
            continue;
        }
        const auto inboundTag = static_cast<InboundNodeData *>(d.get())->GetData()->name;
        dataptr->inboundTags.push_back(inboundTag);
        QvDebug() << "Connecting inbound:" << inboundTag << "to" << dataptr->name;
    }
}

void RuleNodeModel::onNodeHoverEnter(){};
void RuleNodeModel::onNodeHoverLeave(){};
