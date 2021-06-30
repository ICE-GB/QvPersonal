#pragma once

#include "QvPlugin/Gui/QvGUIPluginInterface.hpp"
#include "ui_loopback.h"

class LoopbackSettingsEditor
    : public Qv2rayPlugin::Gui::QvPluginEditor
    , private Ui::loopback
{
    Q_OBJECT

  public:
    explicit LoopbackSettingsEditor(QWidget *parent = nullptr);

    void SetContent(const IOProtocolSettings &_content)
    {
        content = _content;
        inboundTagTxt->setText(content["inboundTag"].toString());
    }

    const IOProtocolSettings GetContent() const
    {
        return content;
    }

  protected:
    void changeEvent(QEvent *e);

  private slots:
    void on_inboundTagTxt_textEdited(const QString &arg1);
};
