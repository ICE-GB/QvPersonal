#pragma once

#include "Gui/QvGUIPluginInterface.hpp"
#include "ui_freedom.h"

class FreedomOutboundEditor
    : public Qv2rayPlugin::Gui::QvPluginEditor
    , private Ui::freedomOutEditor
{
    Q_OBJECT

  public:
    explicit FreedomOutboundEditor(QWidget *parent = nullptr);

    void SetContent(const IOProtocolSettings &content) override
    {
        this->content = content;
        PLUGIN_EDITOR_LOADING_SCOPE({
            DSCB->setCurrentText(content["domainStrategy"].toString());
            redirectTxt->setText(content["redirect"].toString());
        })
    };
    const IOProtocolSettings GetContent() const override
    {
        return content;
    };

  protected:
    void changeEvent(QEvent *e) override;

  private slots:
    void on_DSCB_currentTextChanged(const QString &arg1);
    void on_redirectTxt_textEdited(const QString &arg1);
};
