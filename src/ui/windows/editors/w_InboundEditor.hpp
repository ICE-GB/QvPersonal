#pragma once

#include "GuiPluginHost/GuiPluginHost.hpp"
#include "MessageBus/MessageBus.hpp"
#include "ui_w_InboundEditor.h"

#include <QDialog>

class StreamSettingsWidget;

class InboundEditor
    : public QDialog
    , private Ui::InboundEditor
{
    Q_OBJECT

  public:
    explicit InboundEditor(InboundObject root, QWidget *parent = nullptr);
    ~InboundEditor();
    InboundObject OpenEditor();

  private:
    QvMessageBusSlotDecl;

  private slots:
    void on_inboundProtocolCombo_currentIndexChanged(int index);
    void on_inboundTagTxt_textEdited(const QString &arg1);
    void on_strategyCombo_currentIndexChanged(int arg1);
    void on_refreshNumberBox_valueChanged(int arg1);
    void on_concurrencyNumberBox_valueChanged(int arg1);
    void on_inboundHostTxt_textEdited(const QString &arg1);
    void on_inboundPortTxt_textEdited(const QString &arg1);
    void on_sniffingGroupBox_clicked(bool checked);
    void on_sniffHTTPCB_stateChanged(int arg1);
    void on_sniffTLSCB_stateChanged(int arg1);
    void on_stackedWidget_currentChanged(int arg1);
    void on_sniffMetaDataOnlyCB_clicked(bool checked);
    void on_sniffFakeDNSOtherCB_stateChanged(int arg1);
    void on_sniffFakeDNSCB_stateChanged(int arg1);

  private:
    StreamSettingsWidget *streamSettingsWidget;
    InboundObject getResult();
    void loadUI();
    InboundObject original;
    InboundObject current;
    //
    bool isLoading;
    QJsonObject sniffingSettings;
    QJsonObject allocateSettings;
    QString inboundProtocol;
    QMap<QString, Qv2rayPlugin::Gui::QvPluginEditor *> pluginWidgets;
};
