#pragma once

#include "ui/node/NodeBase.hpp"
#include "ui_BalancerWidget.h"

class BalancerWidget
    : public QvNodeWidget
    , private Ui::BalancerWidget
{
    Q_OBJECT

  public:
    explicit BalancerWidget(std::shared_ptr<NodeDispatcher> _dispatcher, QWidget *parent = nullptr);
    void setValue(std::shared_ptr<OutboundObject> data);
  private slots:
    void on_balancerAddBtn_clicked();
    void on_balancerDelBtn_clicked();
    void on_balancerTagTxt_textEdited(const QString &arg1);
    void on_showHideBtn_clicked();
    void on_strategyCB_currentIndexChanged(const QString &arg1);

  private:
    void OutboundCreated(std::shared_ptr<OutboundObject>, QtNodes::Node &);
    void OutboundDeleted(const OutboundObject &);
    void OnTagChanged(NodeItemType _t1, const QString _t2, const QString _t3);

  signals:
    void OnSizeUpdated();

  protected:
    void changeEvent(QEvent *e);
    std::shared_ptr<OutboundObject> outboundData;
};
