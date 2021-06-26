#pragma once

#include "Gui/QvGUIPluginInterface.hpp"
#include "V2RayModels.hpp"
#include "ui_httpout.h"

class HttpOutboundEditor
    : public Qv2rayPlugin::Gui::QvPluginEditor
    , private Ui::httpOutEditor
{
    Q_OBJECT

  public:
    explicit HttpOutboundEditor(QWidget *parent = nullptr);

    void SetContent(const IOProtocolSettings &source) override
    {
        http.loadJson(source);
        http.user.ReadWriteBind(http_UserNameTxt, "text", &QLineEdit::textEdited);
        http.pass.ReadWriteBind(http_PasswordTxt, "text", &QLineEdit::textEdited);
    }

    const IOProtocolSettings GetContent() const override
    {
        return IOProtocolSettings{ http.toJson() };
    }

  protected:
    void changeEvent(QEvent *e) override;

  private:
    Qv2ray::Models::HTTPSOCKSObject http;
};
