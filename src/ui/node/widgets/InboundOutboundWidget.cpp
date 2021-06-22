#include "InboundOutboundWidget.hpp"

#include "Profile/ProfileManager.hpp"
#include "ui/windows/editors/w_InboundEditor.hpp"
#include "ui/windows/editors/w_JsonEditor.hpp"
#include "ui/windows/editors/w_OutboundEditor.hpp"

InboundOutboundWidget::InboundOutboundWidget(NodeItemType mode, std::shared_ptr<NodeDispatcher> _d, QWidget *parent) : QvNodeWidget(_d, parent)
{
    workingMode = mode;
    setupUi(this);
    editBtn->setIcon(QIcon(STYLE_RESX("edit")));
    editJsonBtn->setIcon(QIcon(STYLE_RESX("code")));
}

void InboundOutboundWidget::setValue(std::shared_ptr<InboundObject> data)
{
    assert(workingMode == NodeItemType::INBOUND);
    inboundObject = data;
    tagTxt->setText(data->name);
}

void InboundOutboundWidget::setValue(std::shared_ptr<OutboundObject> data)
{
    assert(workingMode == NodeItemType::OUTBOUND);
    outboundObject = data;
    tagTxt->setText(outboundObject->name);
    isExternalOutbound = outboundObject->objectType == OutboundObject::EXTERNAL;
    statusLabel->setText(isExternalOutbound ? tr("External Config") : "");
    tagTxt->setEnabled(!isExternalOutbound);
}

void InboundOutboundWidget::changeEvent(QEvent *e)
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

void InboundOutboundWidget::on_editBtn_clicked()
{
    if (workingMode == NodeItemType::INBOUND)
    {
        InboundEditor editor{ *inboundObject, parentWidget() };
        *inboundObject = editor.OpenEditor();
        // Set tag
        const auto newTag = inboundObject->name;
        tagTxt->setText(newTag);
        inboundObject->name = newTag;
    }
    else
    {
        if (isExternalOutbound)
        {
            QvBaselib->Warn(tr("Editing Extenal Connection"), tr("All inbound and routing information will be lost after saving!"));
            const auto externalId = outboundObject->externalId;
            const auto root = QvBaselib->ProfileManager()->GetConnection(externalId);
            OutboundEditor editor{ root.outbounds.first(), parentWidget() };
            const auto newoutbound = editor.OpenEditor();
            if (editor.result() == QDialog::Accepted)
                QvBaselib->ProfileManager()->UpdateConnection(externalId, ProfileContent{ newoutbound });
        }
        else
        {
            OutboundEditor editor{ *outboundObject, parentWidget() };
            *outboundObject = editor.OpenEditor();
            const auto newTag = outboundObject->name;
            tagTxt->setText(newTag);
        }
    }
}

void InboundOutboundWidget::on_editJsonBtn_clicked()
{
    if (workingMode == NodeItemType::INBOUND)
    {
        JsonEditor editor{ inboundObject->toJson(), parentWidget() };
        inboundObject->loadJson(editor.OpenEditor());
        const auto newTag = inboundObject->name;
        tagTxt->setText(newTag);
    }
    else
    {
        if (isExternalOutbound)
        {
            const auto externalId = outboundObject->externalId;
            auto root = QvBaselib->ProfileManager()->GetConnection(externalId);

            JsonEditor editor{ root.outbounds.first().toJson(), parentWidget() };
            root.outbounds.first().loadJson(editor.OpenEditor());
            if (editor.result() == QDialog::Accepted)
                QvBaselib->ProfileManager()->UpdateConnection(externalId, root);
        }
        else
        {
            // Open Editor
            JsonEditor editor{ outboundObject->toJson(), parentWidget() };
            outboundObject->loadJson(editor.OpenEditor());
            //
            // Set tag (only for local connections)
            const auto newTag = outboundObject->name;
            tagTxt->setText(newTag);
        }
    }
}

void InboundOutboundWidget::on_tagTxt_textEdited(const QString &arg1)
{
    if (workingMode == NodeItemType::INBOUND)
    {
        const auto originalTag = inboundObject->name;
        if (originalTag == arg1 || dispatcher->RenameTag<NodeItemType::INBOUND>(originalTag, arg1))
        {
            BLACK(tagTxt);
            inboundObject->name = arg1;
            return;
        }
        RED(tagTxt);
    }
    else
    {
        const auto originalTag = outboundObject->name;
        if (originalTag == arg1 || dispatcher->RenameTag<NodeItemType::OUTBOUND>(originalTag, arg1))
        {
            BLACK(tagTxt);
            outboundObject->name = arg1;
            return;
        }
        RED(tagTxt);
    }
}
