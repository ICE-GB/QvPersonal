#pragma once

#include "MessageBus/MessageBus.hpp"
#include "ui/WidgetUIBase.hpp"
#include "ui_w_PreferencesWindow.h"

class RouteSettingsMatrixWidget;
class DnsSettingsWidget;

class PreferencesWindow
    : public QvDialog
    , private Ui::PreferencesWindow
{
    Q_OBJECT

  public:
    explicit PreferencesWindow(QWidget *parent = nullptr);
    ~PreferencesWindow();
    void processCommands(QString command, QStringList commands, QMap<QString, QString>) override
    {
        const static QMap<QString, int> indexMap{
            { "general", 0 },    //
            { "kernel", 1 },     //
            { "inbound", 2 },    //
            { "connection", 3 }, //
            { "dns", 4 },        //
            { "route", 5 },      //
            { "about", 6 }       //
        };

        if (commands.isEmpty())
            return;
        if (command == "open")
        {
            const auto c = commands.takeFirst();
            tabWidget->setCurrentIndex(indexMap[c]);
        }
    }

  private:
    void updateColorScheme() override{};
    QvMessageBusSlotDecl override;

  private slots:
    void on_aboutQt_clicked();
    void on_autoStartConnCombo_currentIndexChanged(int arg1);
    void on_autoStartSubsCombo_currentIndexChanged(int arg1);
    void on_buttonBox_accepted();
    void on_bypassBTCb_stateChanged(int arg1);
    void on_cancelIgnoreVersionBtn_clicked();

    void on_checkVCoreSettings_clicked();
    void on_dnsFreedomCb_stateChanged(int arg1);
    void on_dnsIntercept_toggled(bool checked);
    void on_enableAPI_stateChanged(int arg1);
    void on_fixedAutoConnectRB_clicked();

    void on_httpOverrideFakeDNSCB_stateChanged(int arg1);
    void on_httpOverrideHTTPCB_stateChanged(int arg1);
    void on_httpOverrideTLSCB_stateChanged(int arg1);
    void on_httpPortLE_valueChanged(int arg1);
    void on_jumpListCountSB_valueChanged(int arg1);
    void on_lastConnectedRB_clicked();

    void on_listenIPTxt_textEdited(const QString &arg1);
    void on_logLevelComboBox_currentIndexChanged(int index);
    void on_maxLogLinesSB_valueChanged(int arg1);
    void on_noAutoConnectRB_clicked();
    void on_openConfigDirCB_clicked();
    void on_outboundMark_valueChanged(int arg1);
    void on_pluginKernelPortAllocateCB_valueChanged(int arg1);
    void on_proxyDefaultCb_stateChanged(int arg1);

    void on_qvNetworkUATxt_editTextChanged(const QString &arg1);
    void on_qvProxyAddressTxt_textEdited(const QString &arg1);
    void on_qvProxyCustomProxy_clicked();
    void on_qvProxyNoProxy_clicked();
    void on_qvProxyPortCB_valueChanged(int arg1);
    void on_qvProxySystemProxy_clicked();
    void on_qvProxyTypeCombo_currentTextChanged(const QString &arg1);

    void on_selectVAssetBtn_clicked();
    void on_selectVCoreBtn_clicked();
    void on_socksOverrideFakeDNSCB_stateChanged(int arg1);
    void on_socksOverrideHTTPCB_stateChanged(int arg1);
    void on_socksOverrideTLSCB_stateChanged(int arg1);
    void on_socksPortLE_valueChanged(int arg1);
    void on_socksUDPCB_stateChanged(int arg1);
    void on_socksUDPIP_textEdited(const QString &arg1);

    void on_startWithLoginCB_stateChanged(int arg1);

    void on_statsPortBox_valueChanged(int arg1);
    void on_tproxyMode_currentTextChanged(const QString &arg1);
    void on_tproxyOverrideFakeDNSCB_stateChanged(int arg1);
    void on_tproxyOverrideHTTPCB_stateChanged(int arg1);
    void on_tproxyOverrideTLSCB_stateChanged(int arg1);
    void on_tProxyPort_valueChanged(int arg1);

    void on_updateChannelCombo_currentIndexChanged(int index);
    void on_vCoreAssetsPathTxt_textEdited(const QString &arg1);
    void on_vCorePathTxt_textEdited(const QString &arg1);

    // NEW
    void on_httpSniffingCB_currentIndexChanged(int index);

    void on_dokoSniffingCB_currentIndexChanged(int index);

    void on_socksSniffingCB_currentIndexChanged(int index);

  private:
    DnsSettingsWidget *dnsSettingsWidget;
    RouteSettingsMatrixWidget *routeSettingsWidget;
    void SetAutoStartButtonsState(bool isAutoStart);
    //
    bool NeedRestart = false;
    bool finishedLoading = false;
    Qv2ray::Models::Qv2rayApplicationConfigObject AppConfig;
    Qv2rayBase::Models::Qv2rayBaseConfigObject BaselibConfig;
};
