#pragma once

#include "QvPlugin/Gui/QvGUIPluginInterface.hpp"
#include "V2RayModels.hpp"
#include "ui_socksout.h"

class SocksOutboundEditor
    : public Qv2rayPlugin::Gui::QvPluginEditor
    , private Ui::socksOutEditor
{
    Q_OBJECT

  public:
    explicit SocksOutboundEditor(QWidget *parent = nullptr);

    void SetContent(const IOProtocolSettings &source) override
    {
        socks.loadJson(source);
        socks.user.ReadWriteBind(socks_UserNameTxt, "text", &QLineEdit::textEdited);
        socks.pass.ReadWriteBind(socks_PasswordTxt, "text", &QLineEdit::textEdited);
    }

    const IOProtocolSettings GetContent() const override
    {
        return IOProtocolSettings{ socks.toJson() };
    }

  protected:
    void changeEvent(QEvent *e) override;

  private:
    Qv2ray::Models::HTTPSOCKSObject socks;
};
