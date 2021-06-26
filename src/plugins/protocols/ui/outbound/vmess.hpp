#pragma once

#include "Gui/QvGUIPluginInterface.hpp"
#include "V2RayModels.hpp"
#include "ui_vmess.h"

class VmessOutboundEditor
    : public Qv2rayPlugin::Gui::QvPluginEditor
    , private Ui::vmessOutEditor
{
    Q_OBJECT

  public:
    explicit VmessOutboundEditor(QWidget *parent = nullptr);

    void SetContent(const IOProtocolSettings &content) override;
    const IOProtocolSettings GetContent() const override
    {
        return IOProtocolSettings{ vmess.toJson() };
    }

  private:
    Qv2ray::Models::VMessClientObject vmess;

  protected:
    void changeEvent(QEvent *e) override;
};
