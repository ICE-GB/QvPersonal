#include "BalancerWidget.hpp"

BalancerWidget::BalancerWidget(std::shared_ptr<NodeDispatcher> _dispatcher, QWidget *parent) : QvNodeWidget(_dispatcher, parent)
{
    setupUi(this);
    balancerAddBtn->setIcon(QIcon(STYLE_RESX("add")));
    balancerDelBtn->setIcon(QIcon(STYLE_RESX("ashbin")));
    connect(dispatcher.get(), &NodeDispatcher::OnOutboundCreated, this, &BalancerWidget::OutboundCreated);
    connect(dispatcher.get(), &NodeDispatcher::OnOutboundDeleted, this, &BalancerWidget::OutboundDeleted);
    connect(dispatcher.get(), &NodeDispatcher::OnObjectTagChanged, this, &BalancerWidget::OnTagChanged);
}

void BalancerWidget::setValue(std::shared_ptr<OutboundObject> data)
{
    outboundData = data;
    balancerSelectionCombo->clear();
    balancerSelectionCombo->addItems(dispatcher->GetRealOutboundTags());
    balancerTagTxt->setText(data->name);
#pragma message("TODO")
    //    balancerList->addItems(data->outboundTags);
    strategyCB->setCurrentText(data->balancerSettings.selectorType);
}

void BalancerWidget::changeEvent(QEvent *e)
{
    QWidget::changeEvent(e);
    switch (e->type())
    {
        case QEvent::LanguageChange: retranslateUi(this); break;
        default: break;
    }
}

void BalancerWidget::on_balancerAddBtn_clicked()
{
    const auto balancerTx = balancerSelectionCombo->currentText().trimmed();

    if (!balancerTx.isEmpty())
    {
#pragma message("TODO: Balancer")
        //        outboundData->outboundTags.append(balancerSelectionCombo->currentText());
        balancerList->addItem(balancerTx);
        balancerSelectionCombo->setEditText("");
    }
}

void BalancerWidget::OutboundCreated(std::shared_ptr<OutboundObject> data, QtNodes::Node &)
{
    if (data->objectType != OutboundObject::BALANCER)
        balancerSelectionCombo->addItem(data->name);
}

void BalancerWidget::OutboundDeleted(const OutboundObject &data)
{
    if (data.objectType != OutboundObject::BALANCER)
        balancerSelectionCombo->removeItem(balancerSelectionCombo->findText(data.name));
}

void BalancerWidget::OnTagChanged(NodeItemType type, const QString originalTag, const QString newTag)
{
    if (type != NodeItemType::OUTBOUND)
        return;
    const auto index = balancerSelectionCombo->findText(originalTag);
    if (index >= 0)
        balancerSelectionCombo->setItemText(index, newTag);
}

void BalancerWidget::on_balancerDelBtn_clicked()
{
    if (balancerList->currentRow() < 0)
        return;

#pragma message("TODO: Balancer")
    //    outboundData->outboundTags.removeOne(balancerList->currentItem()->text());
    balancerList->takeItem(balancerList->currentRow());
}

void BalancerWidget::on_balancerTagTxt_textEdited(const QString &arg1)
{
    const auto originalName = outboundData->name;
    if (originalName == arg1 || dispatcher->RenameTag<NodeItemType::OUTBOUND>(originalName, arg1))
    {
        outboundData->name = arg1;
        BLACK(balancerTagTxt);
    }
    else
    {
        RED(balancerTagTxt);
    }
}

void BalancerWidget::on_showHideBtn_clicked()
{
    tabWidget->setVisible(!tabWidget->isVisible());
}

void BalancerWidget::on_strategyCB_currentIndexChanged(const QString &arg1)
{
    outboundData->balancerSettings.selectorType = arg1;
}
