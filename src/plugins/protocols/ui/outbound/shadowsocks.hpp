#pragma once

#include "Gui/QvGUIPluginInterface.hpp"
#include "V2RayModels.hpp"
#include "ui_shadowsocks.h"

class ShadowsocksOutboundEditor
    : public Qv2rayPlugin::Gui::QvPluginEditor
    , private Ui::shadowsocksOutEditor
{
    Q_OBJECT

  public:
    explicit ShadowsocksOutboundEditor(QWidget *parent = nullptr);

    void SetContent(const IOProtocolSettings &content) override
    {
        shadowsocks.loadJson(content);
        shadowsocks.method.ReadWriteBind(ss_encryptionMethod, "currentText", &QComboBox::currentTextChanged);
        shadowsocks.password.ReadWriteBind(ss_passwordTxt, "text", &QLineEdit::textEdited);
    }
    const IOProtocolSettings GetContent() const override
    {
        auto result = content;
        QJsonArray servers;
        servers.append(shadowsocks.toJson());
        result.insert("servers", servers);
        return result;
    }

  protected:
    void changeEvent(QEvent *e) override;

  private:
    Qv2ray::Models::ShadowSocksClientObject shadowsocks;
};
