#pragma once

#include "MessageBus/MessageBus.hpp"
#include "ui/node/NodeBase.hpp"
#include "ui_RoutingEditorWidget.h"

#include <nodes/FlowScene>
#include <nodes/FlowView>

class NodeDispatcher;

class RoutingEditorWidget
    : public QWidget
    , private Ui::RoutingEditorWidget
{
    Q_OBJECT

  public:
    explicit RoutingEditorWidget(std::shared_ptr<NodeDispatcher> dispatcher, QWidget *parent = nullptr);
    auto getScene()
    {
        return scene;
    }

  protected:
    void changeEvent(QEvent *e);

  private slots:
    void OnDispatcherInboundCreated(std::shared_ptr<InboundObject>, QtNodes::Node &);
    void OnDispatcherOutboundCreated(std::shared_ptr<OutboundObject>, QtNodes::Node &);
    void OnDispatcherRuleCreated(std::shared_ptr<RuleObject>, QtNodes::Node &);
    void on_addRouteBtn_clicked();
    void on_delBtn_clicked();

  private:
    void updateColorScheme();
    QvMessageBusSlotDecl;

  private:
    QtNodes::FlowScene *scene;
    QtNodes::FlowView *view;
    std::shared_ptr<NodeDispatcher> dispatcher;
};
