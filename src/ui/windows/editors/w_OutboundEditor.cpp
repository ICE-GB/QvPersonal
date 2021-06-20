#include "w_OutboundEditor.hpp"

#include "ui/WidgetUIBase.hpp"
#include "ui/windows/editors/w_JsonEditor.hpp"
#include "ui/windows/editors/w_RoutesEditor.hpp"

#include <QFile>
#include <QIntValidator>

#define QV_MODULE_NAME "OutboundEditor"

constexpr auto OUTBOUND_TAG_PROXY = "Proxy";

OutboundEditor::OutboundEditor(QWidget *parent) : QDialog(parent), tag(OUTBOUND_TAG_PROXY)
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
        case UPDATE_COLORSCHEME: break;
    }
}

OutboundEditor::OutboundEditor(const OutboundObject &outboundEntry, QWidget *parent) : OutboundEditor(parent)
{
    originalConfig = outboundEntry;
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
    auto host = ipLineEdit->text().replace(":", "-").replace("/", "_").replace("\\", "_");
    auto port = portLineEdit->text().replace(":", "-").replace("/", "_").replace("\\", "_");
    return tag.isEmpty() ? outboundType + "@" + host + ":" + port : tag;
}

OutboundObject OutboundEditor::generateConnectionJson()
{
    IOProtocolSettings settings;
    auto streaming = streamSettingsWidget->GetStreamSettings();
    bool processed = false;
    for (const auto &[protocol, widget] : pluginWidgets.toStdMap())
    {
        if (protocol == outboundType)
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
        QvBaselib->Warn(tr("Unknown outbound type."), tr("The specified outbound type is not supported, this may happen due to a plugin failure."));
    }

    OutboundObject out;
    out.name = tag;
    out.protocol = outboundType;
    out.outboundSettings.protocolSettings = settings;
    out.outboundSettings.streamSettings = streaming;
    out.options["mux"] = muxConfig;
    return out;
}

void OutboundEditor::reloadGUI()
{
    tag = originalConfig.name;
    tagTxt->setText(tag);
    outboundType = originalConfig.protocol;
    if (outboundType.isEmpty())
        outboundType = "vmess";

#pragma message("TODO: properly place mux object")
    muxConfig = originalConfig.options["mux"].toObject();
    streamSettingsWidget->SetStreamObject(StreamSettingsObject::fromJson(originalConfig.outboundSettings.streamSettings));
    //
    muxEnabledCB->setChecked(muxConfig["enabled"].toBool());
    muxConcurrencyTxt->setValue(muxConfig["concurrency"].toInt());
    //
    const auto &settings = originalConfig.outboundSettings.protocolSettings;
    //
    bool processed = false;
    for (const auto &[protocol, widget] : pluginWidgets.toStdMap())
    {
        if (protocol == outboundType)
        {
            outBoundTypeCombo->setCurrentIndex(outBoundTypeCombo->findData(protocol));
            widget->SetContent(settings);
            const auto &[_address, _port] = widget->GetHostAddress();
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
        QvLog() << "Outbound type:" << outboundType << " is not supported.";
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
    tag = arg1;
}

void OutboundEditor::on_muxEnabledCB_stateChanged(int arg1)
{
    muxConfig["enabled"] = arg1 == Qt::Checked;
}

void OutboundEditor::on_muxConcurrencyTxt_valueChanged(int arg1)
{
    muxConfig["concurrency"] = arg1;
}

void OutboundEditor::on_outBoundTypeCombo_currentIndexChanged(int)
{
    outboundType = outBoundTypeCombo->currentData().toString();
    auto newWidget = pluginWidgets[outboundType];
    if (!newWidget)
        return;
    outboundTypeStackView->setCurrentWidget(newWidget);
    const auto hasStreamSettings = GetProperty(newWidget, "QV2RAY_INTERNAL_HAS_STREAMSETTINGS");
    streamSettingsWidget->setEnabled(hasStreamSettings);
}
