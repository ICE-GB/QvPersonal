#pragma once

#include "MessageBus/MessageBus.hpp"
#include "ui/node/NodeBase.hpp"
#include "ui_ChainEditorWidget.h"

#include <nodes/FlowScene>
#include <nodes/FlowView>
#include <tuple>

class NodeDispatcher;

class ChainEditorWidget
    : public QWidget
    , private Ui::ChainEditorWidget
{
    Q_OBJECT

  public:
    explicit ChainEditorWidget(std::shared_ptr<NodeDispatcher> dispatcher, QWidget *parent = nullptr);
    ~ChainEditorWidget()
    {
        connectionSignalBlocked = true;
    }
    auto getScene()
    {
        return scene;
    }

  protected:
    void changeEvent(QEvent *e);

  private slots:
    void on_chainComboBox_currentIndexChanged(int arg1);
    //
    void OnDispatcherChainedOutboundCreated(std::shared_ptr<OutboundObject>, QtNodes::Node &);
    void OnDispatcherChainedOutboundDeleted(const OutboundObject &);
    void OnDispatcherChainCreated(std::shared_ptr<OutboundObject>);
    void OnDispatcherChainDeleted(const OutboundObject &);
    //
    void OnDispatcherObjectTagChanged(NodeItemType, const QString originalTag, const QString newTag);
    //
    void OnSceneConnectionCreated(const QtNodes::Connection &);
    void OnSceneConnectionRemoved(const QtNodes::Connection &);

  private:
    void BeginEditChain(const QString &chain);
    void updateColorScheme(){};
    void ShowChainLinkedList();
    std::tuple<bool, QString, QStringList> VerifyChainLinkedList(const QUuid &ignoredConnectionId);
    void TrySaveChainOutboudData(const QUuid &ignoredConnectionId = QUuid());
    QvMessageBusSlotDecl;

  private:
    bool connectionSignalBlocked = false;
    QtNodes::FlowScene *scene;
    QtNodes::FlowView *view;
    std::shared_ptr<NodeDispatcher> dispatcher;
    //
    QMap<QString, QUuid> outboundNodes;
    std::shared_ptr<OutboundObject> currentChainOutbound;
    QMap<QString, std::shared_ptr<OutboundObject>> chains;
};
