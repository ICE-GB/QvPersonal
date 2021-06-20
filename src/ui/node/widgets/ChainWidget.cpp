#include "ChainWidget.hpp"

ChainWidget::ChainWidget(std::shared_ptr<NodeDispatcher> _dispatcher, QWidget *parent) : QvNodeWidget(_dispatcher, parent)
{
    setupUi(this);
    editChainBtn->setIcon(QIcon(QV2RAY_COLORSCHEME_FILE("edit")));
}

void ChainWidget::setValue(std::shared_ptr<OutboundObject> data)
{
    dataptr = data;
    displayNameTxt->setText(data->name);
}

void ChainWidget::changeEvent(QEvent *e)
{
    QWidget::changeEvent(e);
    switch (e->type())
    {
        case QEvent::LanguageChange: retranslateUi(this); break;
        default: break;
    }
}

void ChainWidget::on_displayNameTxt_textEdited(const QString &arg1)
{
    const auto originalTag = dataptr->name;
    if (originalTag == arg1 || dispatcher->RenameTag<NodeItemType::OUTBOUND>(originalTag, arg1))
    {
        dataptr->name = arg1;
        BLACK(displayNameTxt);
    }
    else
    {
        RED(displayNameTxt);
    }
}

void ChainWidget::on_editChainBtn_clicked()
{
    emit dispatcher->RequestEditChain(dataptr->name);
}
