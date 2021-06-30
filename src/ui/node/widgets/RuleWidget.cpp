#include "RuleWidget.hpp"

#include "Qv2rayBase/Common/Utils.hpp"

#define LOAD_FLAG_END isLoading = false;
#define LOAD_FLAG_BEGIN isLoading = true;
#define LOADINGCHECK                                                                                                                                 \
    if (isLoading)                                                                                                                                   \
        return;

#define rule (*(this->ruleptr))

const static auto Split_RemoveDuplicate_Sort = [](const QString &in) {
    auto entries = SplitLines(in);
    entries.removeDuplicates();
    entries.sort();
    return entries;
};

QvNodeRuleWidget::QvNodeRuleWidget(std::shared_ptr<NodeDispatcher> _dispatcher, QWidget *parent) : QvNodeWidget(_dispatcher, parent)
{
    setupUi(this);
    settingsFrame->setVisible(false);
    adjustSize();
}

void QvNodeRuleWidget::changeEvent(QEvent *e)
{
    QWidget::changeEvent(e);
    switch (e->type())
    {
        case QEvent::LanguageChange:
        {
            retranslateUi(this);
            break;
        }
        default: break;
    }
}

void QvNodeRuleWidget::setValue(std::shared_ptr<RuleObject> _ruleptr)
{
    this->ruleptr = _ruleptr;
    // Switch to the detailed page.
    ruleEnableCB->setEnabled(true);
    ruleEnableCB->setChecked(rule.enabled);
    ruleTagLineEdit->setEnabled(true);
    LOAD_FLAG_BEGIN
    ruleTagLineEdit->setText(rule.name);
    isLoading = false;
    // Networks
    const auto network = rule.networks.join(",").toLower();
    netUDPRB->setChecked(network.contains("udp"));
    netTCPRB->setChecked(network.contains("tcp"));
    //
    // Set protocol checkboxes.
    const auto protocol = rule.protocols;
    routeProtocolHTTPCB->setChecked(protocol.contains("http"));
    routeProtocolTLSCB->setChecked(protocol.contains("tls"));
    routeProtocolBTCB->setChecked(protocol.contains("bittorrent"));
    //
    // Port
    routePortTxt->setText(rule.targetPort);
    //
    // ? Users
    const auto sourcePorts = rule.sourcePort;
    //
    // Incoming Sources
    const auto sources = rule.sourceAddresses.join(NEWLINE);
    sourceIPList->setPlainText(sources);
    //
    // Domains
    QString domains = rule.targetDomains.join(NEWLINE);
    hostList->setPlainText(domains);
    //
    // Outcoming IPs
    QString ips = rule.targetIPs.join(NEWLINE);
    ipList->setPlainText(ips);
    LOAD_FLAG_END
}

void QvNodeRuleWidget::on_routeProtocolHTTPCB_stateChanged(int)
{
    LOADINGCHECK
    SetProtocolProperty();
}

void QvNodeRuleWidget::on_routeProtocolTLSCB_stateChanged(int)
{
    LOADINGCHECK
    SetProtocolProperty();
}

void QvNodeRuleWidget::on_routeProtocolBTCB_stateChanged(int)
{
    LOADINGCHECK
    SetProtocolProperty();
}

void QvNodeRuleWidget::on_hostList_textChanged()
{
    LOADINGCHECK
    rule.targetDomains = Split_RemoveDuplicate_Sort(hostList->toPlainText());
}

void QvNodeRuleWidget::on_ipList_textChanged()
{
    LOADINGCHECK
    rule.targetIPs = Split_RemoveDuplicate_Sort(ipList->toPlainText());
}

void QvNodeRuleWidget::on_routePortTxt_textEdited(const QString &arg1)
{
    LOADINGCHECK
    bool canConvert;
    const auto port = arg1.toInt(&canConvert);
    if (canConvert)
        rule.targetPort.from = port, rule.targetPort.to = port;
    else
    {
#pragma message("TODO Check more")
        const auto parts = arg1.split('-');
        rule.targetPort.from = parts[0].toInt();
        rule.targetPort.to = parts[1].toInt();
    }
}

void QvNodeRuleWidget::on_netUDPRB_clicked()
{
    LOADINGCHECK
    SetNetworkProperty();
}

void QvNodeRuleWidget::on_netTCPRB_clicked()
{
    LOADINGCHECK
    SetNetworkProperty();
}

void QvNodeRuleWidget::on_sourceIPList_textChanged()
{
    LOADINGCHECK
    rule.sourceAddresses = Split_RemoveDuplicate_Sort(sourceIPList->toPlainText());
}

void QvNodeRuleWidget::on_ruleEnableCB_stateChanged(int arg1)
{
    bool _isEnabled = arg1 == Qt::Checked;
    rule.enabled = _isEnabled;
    settingsFrame->setEnabled(_isEnabled);
}

void QvNodeRuleWidget::on_toolButton_clicked()
{
    settingsFrame->setVisible(!settingsFrame->isVisible());
    adjustSize();
    emit OnSizeUpdated();
}

void QvNodeRuleWidget::on_ruleTagLineEdit_textEdited(const QString &arg1)
{
    const auto originalTag = rule.name;
    if (originalTag == arg1 || dispatcher->RenameTag<NodeItemType::RULE>(originalTag, arg1))
    {
        BLACK(ruleTagLineEdit);
        rule.name = arg1;
        return;
    }
    RED(ruleTagLineEdit);
}
