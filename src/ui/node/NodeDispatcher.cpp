#include "NodeDispatcher.hpp"

#include "Qv2rayBase/Common/ProfileHelpers.hpp"
#include "Qv2rayBase/Common/Utils.hpp"
#include "models/InboundNodeModel.hpp"
#include "models/OutboundNodeModel.hpp"
#include "models/RuleNodeModel.hpp"

#include <nodes/Node>

#define QV_MODULE_NAME "NodeDispatcher"

NodeDispatcher::NodeDispatcher(QObject *parent) : QObject(parent)
{
}

NodeDispatcher::~NodeDispatcher()
{
}

std::tuple<QMap<QString, InboundObject>, QMap<QString, RuleObject>, QMap<QString, OutboundObject>> NodeDispatcher::GetData() const
{
    QMap<QString, InboundObject> _inbounds;
    QMap<QString, RuleObject> _rules;
    QMap<QString, OutboundObject> _outbounds;
    for (const auto &[key, val] : inbounds.toStdMap())
        val->name = key, _inbounds[key] = *val;
    for (const auto &[key, val] : rules.toStdMap())
        val->name = key, _rules[key] = *val;
    for (const auto &[key, val] : outbounds.toStdMap())
        val->name = key, _outbounds[key] = *val;
    return std::make_tuple(_inbounds, _rules, _outbounds);
}

void NodeDispatcher::LoadFullConfig(const ProfileContent &root)
{
    isOperationLocked = true;
    // Show connections in the node graph
    for (auto in : root.inbounds)
    {
        if (in.name.isEmpty())
            in.name = GenerateRandomString();
        auto _ = CreateInbound(in);
    }

    for (auto out : root.outbounds)
    {
        if (out.objectType == OutboundObject::ORIGINAL && out.name.isEmpty())
            out.name = GenerateRandomString();
        auto _ = CreateOutbound(out);
    }

    for (const auto &item : root.routing.rules)
    {
        auto _ = CreateRule(item);
    }

    //    for (const auto &balancer : root.routing.balancers)
    //    {
    //        const auto array = balancer["selector"].toArray();
    //        QStringList selector;
    //        for (const auto &item : array)
    //        {
    //            selector << item.toString();
    //        }
    //        QString strategyType = balancer.toObject()["strategy"].toObject()["type"].toString("random");
    //        const auto meta = make_balancer_outbound(selector, strategyType, balancer.toObject()["tag"].toString());
    //        auto _ = CreateOutbound(meta);
    //    }

    for (const auto &rule : rules)
    {
        if (!ruleNodes.contains(rule->name))
        {
            QvLog() << "Could not find rule:" << rule->name;
            continue;
        }
        const auto ruleNodeId = ruleNodes[rule->name];
        // Process inbounds.
        for (const auto &inboundTag : rule->inboundTags)
        {
            if (!inboundNodes.contains(inboundTag))
            {
                QvLog() << "Could not find inbound:" << inboundTag;
                continue;
            }
            const auto inboundNodeId = inboundNodes.value(inboundTag);
            ruleScene->createConnection(*ruleScene->node(ruleNodeId), 0, *ruleScene->node(inboundNodeId), 0);
        }

        if (!outboundNodes.contains(rule->outboundTag))
        {
            QvLog() << "Could not find outbound:" << rule->outboundTag;
            continue;
        }
        const auto &outboundNodeId = outboundNodes[rule->outboundTag];
        ruleScene->createConnection(*ruleScene->node(outboundNodeId), 0, *ruleScene->node(ruleNodeId), 0);
    }
    isOperationLocked = false;
    emit OnFullConfigLoadCompleted();
}

void NodeDispatcher::OnNodeDeleted(const QtNodes::Node &node)
{
    if (isOperationLocked)
        return;
    const auto nodeId = node.id();
    bool isInbound = inboundNodes.values().contains(nodeId);
    bool isOutbound = outboundNodes.values().contains(nodeId);
    bool isRule = ruleNodes.values().contains(nodeId);
    // Lots of duplicated checks.
    {
        Q_ASSERT_X(isInbound || isOutbound || isRule, "Delete Node", "Node type error.");
        if (isInbound)
            Q_ASSERT_X(!isOutbound && !isRule, "Delete Node", "Node Type Error");
        if (isOutbound)
            Q_ASSERT_X(!isInbound && !isRule, "Delete Node", "Node Type Error");
        if (isRule)
            Q_ASSERT_X(!isInbound && !isOutbound, "Delete Node", "Node Type Error");
    }

#define CLEANUP(type)                                                                                                                                                    \
    if (!type##Nodes.values().contains(nodeId))                                                                                                                          \
    {                                                                                                                                                                    \
        QvLog() << "Could not find a(n) " #type " with id:" << nodeId.toString();                                                                                        \
        return;                                                                                                                                                          \
    }                                                                                                                                                                    \
    const auto type##Tag = type##Nodes.key(nodeId);                                                                                                                      \
    const auto type = type##s.value(type##Tag);                                                                                                                          \
                                                                                                                                                                         \
    type##Nodes.remove(type##Tag);                                                                                                                                       \
    type##s.remove(type##Tag);

    if (isInbound)
    {
        CLEANUP(inbound);
    }
    else if (isOutbound)
    {
        CLEANUP(outbound);
        const auto object = *outbound;
        if (object.objectType == OutboundObject::CHAIN)
        {
            emit OnChainedOutboundDeleted(object);
        }
        emit OnOutboundDeleted(object);
    }
    else if (isRule)
    {
        CLEANUP(rule);
        emit OnRuleDeleted(*rule);
    }
    else
    {
        Q_UNREACHABLE();
    }
#undef CLEANUP
}

QString NodeDispatcher::CreateInbound(InboundObject in)
{
    while (inbounds.contains(in.name))
    {
        in.name += "_" + GenerateRandomString(5);
    }
    auto dataPtr = std::make_shared<InboundObject>(std::move(in));
    inbounds.insert(dataPtr->name, dataPtr);
    // Create node and emit signals.
    {
        auto nodeData = std::make_unique<InboundNodeModel>(shared_from_this(), dataPtr);
        auto &node = ruleScene->createNode(std::move(nodeData));
        inboundNodes.insert(dataPtr->name, node.id());
        emit OnInboundCreated(dataPtr, node);
    }
    return dataPtr->name;
}

QString NodeDispatcher::CreateOutbound(OutboundObject out)
{
    // In case the tag is duplicated:
    while (outbounds.contains(out.name))
    {
        out.name += "_" + GenerateRandomString(5);
    }
    auto dataPtr = std::make_shared<OutboundObject>(out);
    outbounds.insert(dataPtr->name, dataPtr);
    // Create node and emit signals.
    {
        auto nodeData = std::make_unique<OutboundNodeModel>(shared_from_this(), dataPtr);
        auto &node = ruleScene->createNode(std::move(nodeData));
        outboundNodes.insert(dataPtr->name, node.id());
        emit OnOutboundCreated(dataPtr, node);
    }

    // Create node and emit signals.
    if (dataPtr->objectType == OutboundObject::EXTERNAL || dataPtr->objectType == OutboundObject::ORIGINAL)
    {
        auto nodeData = std::make_unique<ChainOutboundNodeModel>(shared_from_this(), dataPtr);
        auto &node = chainScene->createNode(std::move(nodeData));
        emit OnChainedOutboundCreated(dataPtr, node);
    }
    else if (dataPtr->objectType == OutboundObject::CHAIN)
    {
        emit OnChainedCreated(dataPtr);
    }
    else
    {
        QvLog() << "Ignored non-connection outbound for Chain Editor.";
    }

    return dataPtr->name;
}

QString NodeDispatcher::CreateRule(RuleObject rule)
{
    while (rules.contains(rule.name))
    {
        rule.name += "_" + GenerateRandomString(5);
    }
    auto dataPtr = std::make_shared<RuleObject>(std::move(rule));
    rules.insert(dataPtr->name, dataPtr);
    // Create node and emit signals.
    {
        auto nodeData = std::make_unique<RuleNodeModel>(shared_from_this(), dataPtr);
        auto &node = ruleScene->createNode(std::move(nodeData));
        ruleNodes.insert(dataPtr->name, node.id());
        emit OnRuleCreated(dataPtr, node);
    }
    return dataPtr->name;
}
