#pragma once
#include "Common/CommonTypes.hpp"
#include "GuiPluginHost/GuiPluginHost.hpp"
#include "MessageBus/MessageBus.hpp"
#include "ui/widgets/editors/StreamSettingsWidget.hpp"
#include "ui_w_OutboundEditor.h"

#include <QDialog>
#include <QtCore>

class OutboundEditor
    : public QDialog
    , private Ui::OutboundEditor
{
    Q_OBJECT
  public:
    explicit OutboundEditor(QWidget *parent = nullptr);
    explicit OutboundEditor(const OutboundObject &outboundEntry, QWidget *parent = nullptr);
    ~OutboundEditor();
    OutboundObject OpenEditor();
    QString GetFriendlyName();

  private:
    QvMessageBusSlotDecl;

  private slots:
    void on_buttonBox_accepted();
    void on_ipLineEdit_textEdited(const QString &arg1);
    void on_muxConcurrencyTxt_valueChanged(int arg1);
    void on_muxEnabledCB_stateChanged(int arg1);
    void on_outBoundTypeCombo_currentIndexChanged(int index);
    void on_portLineEdit_textEdited(const QString &arg1);
    void on_tagTxt_textEdited(const QString &arg1);

  private:
    QString tag;
    void reloadGUI();
    OutboundObject generateConnectionJson();
    OutboundObject originalConfig;
    OutboundObject resultConfig;
    QJsonObject muxConfig;
    QString serverAddress;
    int serverPort;
    //
    // Connection Configs
    QString outboundType = "vmess";
    //
    StreamSettingsWidget *streamSettingsWidget;
    //
    QMap<QString, Qv2rayPlugin::Gui::QvPluginEditor *> pluginWidgets;
};
