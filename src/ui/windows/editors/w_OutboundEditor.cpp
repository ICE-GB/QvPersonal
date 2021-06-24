#include "w_OutboundEditor.hpp"

#include "ui/WidgetUIBase.hpp"
#include "ui/windows/editors/w_JsonEditor.hpp"
#include "ui/windows/editors/w_RoutesEditor.hpp"

#include <QFile>
#include <QIntValidator>

#define QV_MODULE_NAME "OutboundEditor"

constexpr auto OUTBOUND_TAG_PROXY = "Proxy";

OutboundEditor::OutboundEditor(QWidget *parent) : QDialog(parent), outboundTag(OUTBOUND_TAG_PROXY)
{
    QvMessageBusConnect();
    setupUi(this);
    streamSettingsWidget = new StreamSettingsWidget(this);
    streamSettingsWidget->SetStreamObject({});
    transportFrame->addWidget(streamSettingsWidget);

    for (const auto &[_, plugin] : GUIPluginHost->GUI_QueryByComponent(Qv2rayPlugin::GUI_COMPONENT_OUTBOUND_EDITOR))
    {
        const auto editors = plugin->GetOutboundEditors();
        for (const auto &editorInfo : editors)
        {
            outBoundTypeCombo->addItem(editorInfo.first.DisplayName, editorInfo.first.Protocol);
            outboundTypeStackView->addWidget(editorInfo.second);
            pluginWidgets.insert(editorInfo.first.Protocol, editorInfo.second);
        }
    }

    outBoundTypeCombo->model()->sort(0);
}

QvMessageBusSlotImpl(OutboundEditor)
{
    switch (msg)
    {
        MBShowDefaultImpl;
        MBHideDefaultImpl;
        MBRetranslateDefaultImpl;
        case MessageBus::UPDATE_COLORSCHEME: break;
    }
}

OutboundEditor::OutboundEditor(const OutboundObject &out, QWidget *parent) : OutboundEditor(parent)
{
    originalConfig = out;
    reloadGUI();
}

OutboundEditor::~OutboundEditor()
{
}

OutboundObject OutboundEditor::OpenEditor()
{
    int resultCode = this->exec();
    return resultCode == QDialog::Accepted ? resultConfig : originalConfig;
}

QString OutboundEditor::GetFriendlyName()
{
    const auto host = ipLineEdit->text().replace(':', '-').replace('/', '_').replace('\\', '_');
    const auto port = portLineEdit->text().replace(':', '-').replace('/', '_').replace('\\', '_');
    return outboundTag.isEmpty() ? outboundProtocol + "@" + host + ":" + port : outboundTag;
}

OutboundObject OutboundEditor::generateConnectionJson()
{
    IOProtocolSettings settings;
    auto streaming = streamSettingsWidget->GetStreamSettings();
    bool processed = false;
    for (const auto &[protocol, widget] : pluginWidgets.toStdMap())
    {
        if (protocol == outboundProtocol)
        {
            widget->SetHostAddress(serverAddress, serverPort);
            settings = widget->GetContent();
            const auto hasStreamSettings = GetProperty(widget, "QV2RAY_INTERNAL_HAS_STREAMSETTINGS");
            if (!hasStreamSettings)
                streaming = {};
            processed = true;
            break;
        }
    }
    if (!processed)
    {
        QvBaselib->Warn(tr("Unknown outbound protocol."), tr("The specified protocol is not supported, this may happen due to a plugin failure."));
    }

    OutboundObject out{ IOConnectionSettings{ outboundProtocol, settings, streaming } };
    out.name = outboundTag;
    out.muxSettings = muxConfig;
    return out;
}

void OutboundEditor::reloadGUI()
{
    outboundTag = originalConfig.name;
    tagTxt->setText(outboundTag);
    outboundProtocol = originalConfig.outboundSettings.protocol;

    muxConfig = originalConfig.muxSettings;
    streamSettingsWidget->SetStreamObject(StreamSettingsObject::fromJson(originalConfig.outboundSettings.streamSettings));

    muxEnabledCB->setChecked(muxConfig.enabled);
    muxConcurrencyTxt->setValue(muxConfig.concurrency);

    const auto &settings = originalConfig.outboundSettings.protocolSettings;
    bool processed = false;
    for (auto it = pluginWidgets.constKeyValueBegin(); it != pluginWidgets.constKeyValueEnd(); it++)
    {
        if (it->first == outboundProtocol)
        {
            outBoundTypeCombo->setCurrentIndex(outBoundTypeCombo->findData(it->first));
            it->second->SetContent(settings);
            const auto &[_address, _port] = it->second->GetHostAddress();
            serverAddress = _address;
            serverPort = _port;
            ipLineEdit->setText(_address);
            portLineEdit->setText(QString::number(_port));
            processed = true;
            break;
        }
    }

    if (!processed)
    {
        QvLog() << "Outbound type:" << outboundProtocol << " is not supported.";
        QvBaselib->Warn(tr("Unknown outbound."), tr("The specified outbound type is invalid, this may be caused by a plugin failure.") + NEWLINE +
                                                     tr("Please use the JsonEditor or reload the plugin."));
        reject();
    }
}

void OutboundEditor::on_buttonBox_accepted()
{
    resultConfig = generateConnectionJson();
}

void OutboundEditor::on_ipLineEdit_textEdited(const QString &arg1)
{
    serverAddress = arg1;
}

void OutboundEditor::on_portLineEdit_textEdited(const QString &arg1)
{
    serverPort = arg1.toInt();
}

void OutboundEditor::on_tagTxt_textEdited(const QString &arg1)
{
    outboundTag = arg1;
}

void OutboundEditor::on_muxEnabledCB_stateChanged(int arg1)
{
    muxConfig.enabled = arg1 == Qt::Checked;
}

void OutboundEditor::on_muxConcurrencyTxt_valueChanged(int arg1)
{
    muxConfig.concurrency = arg1;
}

void OutboundEditor::on_outBoundTypeCombo_currentIndexChanged(int)
{
    outboundProtocol = outBoundTypeCombo->currentData().toString();
    auto newWidget = pluginWidgets[outboundProtocol];
    if (!newWidget)
        return;
    outboundTypeStackView->setCurrentWidget(newWidget);
    const auto hasStreamSettings = GetProperty(newWidget, "QV2RAY_INTERNAL_HAS_STREAMSETTINGS");
    streamSettingsWidget->setEnabled(hasStreamSettings);
}
