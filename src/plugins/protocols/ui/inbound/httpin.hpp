#pragma once

#include "Gui/QvGUIPluginInterface.hpp"
#include "ui_httpin.h"

#include <QJsonArray>

class HTTPInboundEditor
    : public Qv2rayPlugin::Gui::QvPluginEditor
    , private Ui::httpInEditor
{
    Q_OBJECT

  public:
    explicit HTTPInboundEditor(QWidget *parent = nullptr);

    void SetContent(const IOProtocolSettings &content) override;
    const IOProtocolSettings GetContent() const override
    {
        auto newObject = content;
        // Remove useless, misleading 'accounts' array.
        if (newObject["accounts"].toArray().count() == 0)
        {
            newObject.remove("accounts");
        }
        return newObject;
    }

  protected:
    void changeEvent(QEvent *e) override;

  private slots:

    void on_httpTimeoutSpinBox_valueChanged(int arg1);

    void on_httpTransparentCB_stateChanged(int arg1);

    void on_httpRemoveUserBtn_clicked();

    void on_httpAddUserBtn_clicked();
};
