#include "ChainOutboundWidget.hpp"

ChainOutboundWidget::ChainOutboundWidget(std::shared_ptr<NodeDispatcher> _dispatcher, QWidget *parent) : QvNodeWidget(_dispatcher, parent)
{
    setupUi(this);
    // Simple slot to update UI
    connect(_dispatcher.get(), &NodeDispatcher::OnObjectTagChanged,
            [this](NodeItemType, const QString _t2, const QString _t3)
            {
                if (tagLabel->text() == _t2)
                {
                    tagLabel->setText(_t3);
                    emit OnSizeUpdated();
                }
            });
}

void ChainOutboundWidget::changeEvent(QEvent *e)
{
    QWidget::changeEvent(e);
    switch (e->type())
    {
        case QEvent::LanguageChange: retranslateUi(this); break;
        default: break;
    }
}

void ChainOutboundWidget::setValue(std::shared_ptr<OutboundObject> data)
{
    chain = data;
    tagLabel->setText(data->name);
    chainPortSB->setValue(data->chainSettings.chaining_port);
}

void ChainOutboundWidget::on_chainPortSB_valueChanged(int arg1)
{
    chain->chainSettings.chaining_port = arg1;
}
