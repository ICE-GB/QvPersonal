#pragma once
#include "Common/CommonTypes.hpp"

#include <QMap>
#include <nodes/FlowScene>

enum class NodeItemType
{
    INBOUND,
    OUTBOUND,
    RULE,
};

const auto OUTBOUND_META_OBJECT_TYPE_KEY{ QStringLiteral("OutboundMetaType") };

class NodeDispatcher
    : public QObject
    , public std::enable_shared_from_this<NodeDispatcher>
{
    Q_OBJECT
    using FullConfig = std::tuple<QMap<QString, InboundObject>, QMap<QString, RuleObject>, QMap<QString, OutboundObject>>;

  public:
    explicit NodeDispatcher(QObject *parent = nullptr);
    ~NodeDispatcher();
    void InitializeScenes(QtNodes::FlowScene *rule, QtNodes::FlowScene *chain)
    {
        ruleScene = rule;
        chainScene = chain;
        connect(ruleScene, &QtNodes::FlowScene::nodeDeleted, this, &NodeDispatcher::OnNodeDeleted);
    }

  public:
    FullConfig GetData() const;
    void LoadFullConfig(const ProfileContent &);
    [[nodiscard]] QString CreateInbound(InboundObject);
    [[nodiscard]] QString CreateOutbound(OutboundObject);
    [[nodiscard]] QString CreateRule(RuleObject);
    bool IsNodeConstructing() const
    {
        return isOperationLocked;
    }
    void LockOperation()
    {
        isOperationLocked = true;
    }

  public:
    const inline QStringList GetInboundTags() const
    {
        return inbounds.keys();
    }
    const inline QStringList GetOutboundTags() const
    {
        return outbounds.keys();
    }
    const inline QStringList GetRuleTags() const
    {
        return rules.keys();
    }
    const inline QStringList GetRealOutboundTags() const
    {
        QStringList l;
        for (auto it = outbounds.constKeyValueBegin(); it != outbounds.constKeyValueEnd(); it++)
        {
            const auto &[k, v] = it.operator*();
            k.toStdString();
            if (v->objectType != OutboundObject::BALANCER)
                l << k;
        }
        return l;
    }
    inline int InboundsCount() const
    {
        return inbounds.count();
    }
    inline int RulesCount() const
    {
        return rules.count();
    }
    inline int OutboundsCount() const
    {
        return outbounds.count();
    }

  public:
    void OnNodeDeleted(const QtNodes::Node &node);

    template<NodeItemType t>
    inline bool RenameTag(const QString originalTag, const QString newTag)
    {
#define PROCESS(type)                                                                                                                                                    \
    bool hasExisting = type##s.contains(newTag);                                                                                                                         \
    if (hasExisting)                                                                                                                                                     \
        return false;                                                                                                                                                    \
    type##s[newTag] = type##s.take(originalTag);                                                                                                                         \
    type##Nodes[newTag] = type##Nodes.take(originalTag);

        if constexpr (t == NodeItemType::INBOUND)
        {
            if (newTag.isEmpty())
                return false;
            PROCESS(inbound);
        }
        else if constexpr (t == NodeItemType::OUTBOUND)
        {
            if (newTag.isEmpty())
                return false;
            PROCESS(outbound)
        }
        else if constexpr (t == NodeItemType::RULE)
        {
            PROCESS(rule)
        }
        else
        {
            Q_UNREACHABLE();
        }
        emit OnObjectTagChanged(t, originalTag, newTag);
        return true;
#undef PROCESS
    }

  signals:
    void OnFullConfigLoadCompleted();
    void RequestEditChain(const QString &id);
    void OnInboundCreated(std::shared_ptr<InboundObject>, QtNodes::Node &);
    //
    void OnOutboundCreated(std::shared_ptr<OutboundObject>, QtNodes::Node &);
    void OnOutboundDeleted(const OutboundObject &);
    //
    void OnRuleCreated(std::shared_ptr<RuleObject>, QtNodes::Node &);
    void OnRuleDeleted(const RuleObject &);
    //
    void OnChainedCreated(std::shared_ptr<OutboundObject>);
    void OnChainedDeleted(const OutboundObject &);
    void OnChainedOutboundCreated(std::shared_ptr<OutboundObject>, QtNodes::Node &);
    void OnChainedOutboundDeleted(const OutboundObject &);
    //
    void OnObjectTagChanged(const NodeItemType &, const QString originalTag, const QString newTag);

  signals:
    void OnInboundOutboundNodeHovered(const QString &tag, const PluginIOBoundData &);

  private:
    QString defaultOutbound;
    QMap<QString, QUuid> inboundNodes;
    QMap<QString, QUuid> outboundNodes;
    QMap<QString, QUuid> ruleNodes;
    //
    QtNodes::FlowScene *ruleScene;
    QtNodes::FlowScene *chainScene;
    //
    bool isOperationLocked;
    QMap<QString, std::shared_ptr<InboundObject>> inbounds;
    QMap<QString, std::shared_ptr<RuleObject>> rules;
    QMap<QString, std::shared_ptr<OutboundObject>> outbounds;
};
