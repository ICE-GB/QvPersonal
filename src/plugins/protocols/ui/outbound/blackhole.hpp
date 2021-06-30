#pragma once

#include "QvPlugin/Gui/QvGUIPluginInterface.hpp"
#include "ui_blackhole.h"

class BlackholeOutboundEditor
    : public Qv2rayPlugin::Gui::QvPluginEditor
    , private Ui::blackholeOutEditor
{
    Q_OBJECT

  public:
    explicit BlackholeOutboundEditor(QWidget *parent = nullptr);

    void SetContent(const IOProtocolSettings &content) override;
    const IOProtocolSettings GetContent() const override
    {
        return content;
    };

  protected:
    void changeEvent(QEvent *e) override;
  private slots:
    void on_responseTypeCB_currentTextChanged(const QString &arg1);
};
