#pragma once

#include "QvPlugin/Gui/QvGUIPluginInterface.hpp"
#include "ui_dokodemo-door.h"

class DokodemoDoorInboundEditor
    : public Qv2rayPlugin::Gui::QvPluginEditor
    , private Ui::dokodemodoorInEditor
{
    Q_OBJECT

  public:
    explicit DokodemoDoorInboundEditor(QWidget *parent = nullptr);

    void SetContent(const IOProtocolSettings &content) override;
    const IOProtocolSettings GetContent() const override;

  protected:
    void changeEvent(QEvent *e) override;

  private slots:
    void on_dokoFollowRedirectCB_stateChanged(int arg1);
    void on_dokoIPAddrTxt_textEdited(const QString &arg1);
    void on_dokoPortSB_valueChanged(int arg1);
    void on_dokoTCPCB_stateChanged(int arg1);
    void on_dokoUDPCB_stateChanged(int arg1);
    void on_dokoTimeoutSB_valueChanged(int arg1);
};
