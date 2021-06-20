#pragma once

#include "CommonTypes.hpp"
#include "Gui/QvGUIPluginInterface.hpp"
#include "ui_vmess.h"

class VmessOutboundEditor
    : public Qv2rayPlugin::Gui::QvPluginEditor
    , private Ui::vmessOutEditor
{
    Q_OBJECT

  public:
    explicit VmessOutboundEditor(QWidget *parent = nullptr);

    void SetHostAddress(const QString &addr, int port) override
    {
        vmess.address = addr;
        vmess.port = port;
    }

    QPair<QString, int> GetHostAddress() const override
    {
        return { vmess.address, vmess.port };
    }

    void SetContent(const IOProtocolSettings &content) override;
    const IOProtocolSettings GetContent() const override
    {
        auto result = content;
        QJsonArray vnext;
        vnext.append(vmess.toJson());
        result.insert("vnext", vnext);
        return result;
    }

  private:
    VMessServerObject vmess;

  protected:
    void changeEvent(QEvent *e) override;
};
