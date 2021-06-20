#pragma once

#include "ui/node/NodeBase.hpp"
#include "ui_InboundOutboundWidget.h"

class InboundOutboundWidget
    : public QvNodeWidget
    , private Ui::InboundOutboundWidget
{
    Q_OBJECT

  public:
    explicit InboundOutboundWidget(NodeItemType mode, std::shared_ptr<NodeDispatcher> _dispatcher, QWidget *parent = nullptr);
    void setValue(std::shared_ptr<InboundObject>);
    void setValue(std::shared_ptr<OutboundObject> data);

  protected:
    void changeEvent(QEvent *e) override;

  private slots:
    void on_editBtn_clicked();
    void on_editJsonBtn_clicked();
    void on_tagTxt_textEdited(const QString &arg1);

  private:
    NodeItemType workingMode;
    std::shared_ptr<InboundObject> inboundObject;
    std::shared_ptr<OutboundObject> outboundObject;
    bool isExternalOutbound = false;
};
