#pragma once

#include "QvPlugin/Gui/QvGUIPluginInterface.hpp"
#include "ui_socksin.h"

#include <QJsonArray>

class SocksInboundEditor
    : public Qv2rayPlugin::Gui::QvPluginEditor
    , private Ui::socksInEditor
{
    Q_OBJECT

  public:
    explicit SocksInboundEditor(QWidget *parent = nullptr);

    void SetContent(const IOProtocolSettings &content) override;
    const IOProtocolSettings GetContent() const override
    {
        return content;
    };

  private slots:
    void on_socksUDPCB_stateChanged(int arg1);
    void on_socksUDPIPAddrTxt_textEdited(const QString &arg1);
    void on_socksRemoveUserBtn_clicked();
    void on_socksAddUserBtn_clicked();
    void on_socksAuthCombo_currentIndexChanged(int arg1);

  protected:
    void changeEvent(QEvent *e) override;
};
