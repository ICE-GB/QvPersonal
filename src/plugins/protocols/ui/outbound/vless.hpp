#pragma once

#include "QvPlugin/Gui/QvGUIPluginInterface.hpp"
#include "V2RayModels.hpp"
#include "ui_vless.h"

class VlessOutboundEditor
    : public Qv2rayPlugin::Gui::QvPluginEditor
    , private Ui::vlessOutEditor
{
    Q_OBJECT

  public:
    explicit VlessOutboundEditor(QWidget *parent = nullptr);

    void SetContent(const IOProtocolSettings &content) override
    {
        this->content = content;
        vless.loadJson(content);
        vless.encryption.ReadWriteBind(vLessSecurityCombo, "currentText", &QComboBox::currentIndexChanged);
        vless.flow.ReadWriteBind(flowCombo, "currentText", &QComboBox::currentIndexChanged);
        vless.id.ReadWriteBind(vLessIDTxt, "text", &QLineEdit::textEdited);
    }

    const IOProtocolSettings GetContent() const override
    {
        return IOProtocolSettings{ vless.toJson() };
    }

  protected:
    void changeEvent(QEvent *e) override;

  private:
    Qv2ray::Models::VLESSClientObject vless;
};
