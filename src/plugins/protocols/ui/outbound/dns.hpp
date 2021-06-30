#pragma once

#include "QvPlugin/Gui/QvGUIPluginInterface.hpp"
#include "ui_dns.h"

class DnsOutboundEditor
    : public Qv2rayPlugin::Gui::QvPluginEditor
    , private Ui::dnsOutEditor
{
    Q_OBJECT

  public:
    explicit DnsOutboundEditor(QWidget *parent = nullptr);

    void SetContent(const IOProtocolSettings &_content) override
    {
        this->content = _content;
        PLUGIN_EDITOR_LOADING_SCOPE({
            if (content.contains("network"))
            {
                tcpCB->setChecked(content["network"] == "tcp");
                udpCB->setChecked(content["network"] == "udp");
            }
            else
            {
                originalCB->setChecked(true);
            }
            if (content.contains("address"))
                addressTxt->setText(content["address"].toString());
            if (content.contains("port"))
                portSB->setValue(content["port"].toInt());
        })
    };
    const IOProtocolSettings GetContent() const override
    {
        return content;
    };

  protected:
    void changeEvent(QEvent *e) override;

  private slots:
    void on_tcpCB_clicked();
    void on_udpCB_clicked();
    void on_originalCB_clicked();
    void on_addressTxt_textEdited(const QString &arg1);
    void on_portSB_valueChanged(int arg1);
};
