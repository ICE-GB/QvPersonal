#include "w_InboundEditor.hpp"

#include "Qv2rayBase/Common/ProfileHelpers.hpp"
#include "Qv2rayBase/Common/Utils.hpp"
#include "ui/WidgetUIBase.hpp"
#include "ui/widgets/editors/StreamSettingsWidget.hpp"

#include <QGridLayout>

#define QV_MODULE_NAME "InboundEditor"
#define CHECKLOADING                                                                                                                                                     \
    if (isLoading)                                                                                                                                                       \
        return;

InboundEditor::InboundEditor(const InboundObject &source, QWidget *parent) : QDialog(parent), original(source)
{
    QvMessageBusConnect();
    setupUi(this);
    streamSettingsWidget = new StreamSettingsWidget(this);

    if (!transportFrame->layout())
    {
        auto l = new QGridLayout();
        l->setHorizontalSpacing(0);
        l->setVerticalSpacing(0);
        transportFrame->setLayout(l);
    }

    transportFrame->layout()->addWidget(streamSettingsWidget);
    this->current = source;

    inboundProtocol = current.inboundSettings.protocol;

    allocateSettings = current.options[QStringLiteral("allocate")].toObject();
    sniffingSettings = current.options[QStringLiteral("sniffing")].toObject();

    isLoading = true;
    for (const auto &[_, plugin] : GUIPluginHost->GUI_QueryByComponent(Qv2rayPlugin::GUI_COMPONENT_INBOUND_EDITOR))
    {
        const auto editors = plugin->GetInboundEditors();
        for (const auto &editorInfo : editors)
        {
            inboundProtocolCombo->addItem(editorInfo.first.DisplayName, editorInfo.first.Protocol);
            stackedWidget->addWidget(editorInfo.second);
            pluginWidgets.insert(editorInfo.first.Protocol, editorInfo.second);
        }
    }
    inboundProtocolCombo->model()->sort(0);
    isLoading = false;
    loadUI();
}

QvMessageBusSlotImpl(InboundEditor)
{
    switch (msg)
    {
        MBShowDefaultImpl;
        MBHideDefaultImpl;
        MBRetranslateDefaultImpl;
        case MessageBus::UPDATE_COLORSCHEME: break;
    }
}

InboundObject InboundEditor::OpenEditor()
{
    int resultCode = this->exec();
    return resultCode == QDialog::Accepted ? getResult() : original;
}

InboundObject InboundEditor::getResult()
{
    if (inboundProtocol.isEmpty())
        return {};

    InboundObject newRoot = current;
    for (auto it = pluginWidgets.constKeyValueBegin(); it != pluginWidgets.constKeyValueEnd(); it++)
    {
        if (it->first == inboundProtocol)
        {
            newRoot.inboundSettings.protocolSettings = it->second->GetContent();
            break;
        }
    }

    if (streamSettingsWidget->isEnabled())
        newRoot.inboundSettings.streamSettings = streamSettingsWidget->GetStreamSettings();

    newRoot.inboundSettings.protocol = inboundProtocol;
    newRoot.options[QStringLiteral("sniffing")] = sniffingSettings;
    newRoot.options[QStringLiteral("allocate")] = allocateSettings;
    return newRoot;
}

void InboundEditor::loadUI()
{
    isLoading = true;
    streamSettingsWidget->SetStreamObject(StreamSettingsObject::fromJson(original.inboundSettings.streamSettings));
    {
        inboundTagTxt->setText(current.name);
        inboundHostTxt->setText(current.inboundSettings.address);
        inboundPortTxt->setText(current.inboundSettings.port);
    }
    {
        const auto allocationStrategy = allocateSettings["strategy"].toString();
        allocateSettings["strategy"] = allocationStrategy.isEmpty() ? "always" : allocationStrategy;
        strategyCombo->setCurrentText(allocationStrategy);
        refreshNumberBox->setValue(allocateSettings["refresh"].toInt());
        concurrencyNumberBox->setValue(allocateSettings["concurrency"].toInt());
    }
    {
        sniffingGroupBox->setChecked(sniffingSettings["enabled"].toBool());
        sniffMetaDataOnlyCB->setChecked(sniffingSettings["metadataOnly"].toBool());
        const auto data = sniffingSettings["DestinationOverride"].toArray();
        sniffHTTPCB->setChecked(data.contains("http"));
        sniffTLSCB->setChecked(data.contains("tls"));
        sniffFakeDNSCB->setChecked(data.contains("fakedns"));
        sniffFakeDNSOtherCB->setChecked(data.contains("fakedns+others"));
    }
    bool processed = false;
    const auto settings = current.inboundSettings.protocolSettings;
    for (const auto &[protocol, widget] : pluginWidgets.toStdMap())
    {
        if (protocol == inboundProtocol)
        {
            widget->SetContent(settings);
            inboundProtocolCombo->setCurrentIndex(inboundProtocolCombo->findData(protocol));
            processed = true;
            break;
        }
    }

    if (!processed)
    {
        QvLog() << "Inbound protocol:" << inboundProtocol << "is not supported.";
        QvBaselib->Warn(tr("Unknown inbound."), tr("The specified inbound type is invalid, this may be caused by a plugin failure.") + NEWLINE +
                                                    tr("Please use the JsonEditor or reload the plugin."));
        reject();
    }
    isLoading = false;
    on_stackedWidget_currentChanged(0);
}

InboundEditor::~InboundEditor()
{
}

void InboundEditor::on_inboundProtocolCombo_currentIndexChanged(int)
{
    CHECKLOADING
    on_stackedWidget_currentChanged(0);
}

void InboundEditor::on_inboundTagTxt_textEdited(const QString &arg1)
{
    CHECKLOADING
    current.name = arg1;
}
void InboundEditor::on_strategyCombo_currentIndexChanged(int arg1)
{
    CHECKLOADING
    allocateSettings["strategy"] = strategyCombo->itemText(arg1).toLower();
}

void InboundEditor::on_refreshNumberBox_valueChanged(int arg1)
{
    CHECKLOADING
    allocateSettings["refresh"] = arg1;
}

void InboundEditor::on_concurrencyNumberBox_valueChanged(int arg1)
{
    CHECKLOADING
    allocateSettings["concurrency"] = arg1;
}

void InboundEditor::on_inboundHostTxt_textEdited(const QString &arg1)
{
    CHECKLOADING
    current.inboundSettings.address = arg1;
}

void InboundEditor::on_inboundPortTxt_textEdited(const QString &arg1)
{
    CHECKLOADING
    current.inboundSettings.port = arg1.toInt();
}

void InboundEditor::on_sniffingGroupBox_clicked(bool checked)
{
    CHECKLOADING
    sniffingSettings["enabled"] = checked;
}

void InboundEditor::on_sniffMetaDataOnlyCB_clicked(bool checked)
{
    CHECKLOADING
    sniffingSettings["metadataOnly"] = checked;
}

#define SET_SNIFF_DEST_OVERRIDE                                                                                                                                          \
    do                                                                                                                                                                   \
    {                                                                                                                                                                    \
        const auto hasHTTP = sniffHTTPCB->isChecked();                                                                                                                   \
        const auto hasTLS = sniffTLSCB->isChecked();                                                                                                                     \
        const auto hasFakeDNS = sniffFakeDNSCB->isChecked();                                                                                                             \
        const auto hasFakeDNSOthers = sniffFakeDNSOtherCB->isChecked();                                                                                                  \
        QStringList list;                                                                                                                                                \
        if (hasHTTP)                                                                                                                                                     \
            list << "http";                                                                                                                                              \
        if (hasTLS)                                                                                                                                                      \
            list << "tls";                                                                                                                                               \
        if (hasFakeDNS)                                                                                                                                                  \
            list << "fakedns";                                                                                                                                           \
        if (hasFakeDNSOthers)                                                                                                                                            \
            list << "fakedns+others";                                                                                                                                    \
        sniffingSettings["DestinationOverride"] = QJsonArray::fromStringList(list);                                                                                      \
    } while (0)

void InboundEditor::on_sniffHTTPCB_stateChanged(int)
{
    CHECKLOADING
    SET_SNIFF_DEST_OVERRIDE;
}

void InboundEditor::on_sniffTLSCB_stateChanged(int)
{
    CHECKLOADING
    SET_SNIFF_DEST_OVERRIDE;
}
void InboundEditor::on_sniffFakeDNSOtherCB_stateChanged(int)
{
    CHECKLOADING
    SET_SNIFF_DEST_OVERRIDE;
}

void InboundEditor::on_sniffFakeDNSCB_stateChanged(int)
{
    CHECKLOADING
    SET_SNIFF_DEST_OVERRIDE;
}

void InboundEditor::on_stackedWidget_currentChanged(int)
{
    CHECKLOADING
    inboundProtocol = inboundProtocolCombo->currentData().toString();
    auto widget = pluginWidgets[inboundProtocol];
    if (!widget)
        return;
    stackedWidget->setCurrentWidget(widget);
    const auto hasStreamSettings = GetProperty(widget, "QV2RAY_INTERNAL_HAS_STREAMSETTINGS");
    streamSettingsWidget->setEnabled(hasStreamSettings);
}
