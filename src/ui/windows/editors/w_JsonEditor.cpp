#include "w_JsonEditor.hpp"

#include "Common/Utils.hpp"
#include "ui/WidgetUIBase.hpp"

JsonEditor::JsonEditor(QJsonObject rootObject, QWidget *parent) : QDialog(parent)
{
    setupUi(this);
    QvMessageBusConnect();
    //
    original = rootObject;
    final = rootObject;
    QString jsonString = JsonToString(rootObject);

    if (VerifyJsonString(jsonString).has_value())
    {
        QvBaselib->Warn(tr("Json Contains Syntax Errors"), tr("Original Json may contain syntax errors. Json tree is disabled."));
    }
    else
    {
        jsonTree->setModel(&model);
        model.loadJson(QJsonDocument(rootObject).toJson());
    }

    jsonEditor->setText(JsonToString(rootObject));
    jsonTree->expandAll();
    jsonTree->resizeColumnToContents(0);
}

QvMessageBusSlotImpl(JsonEditor)
{
    switch (msg)
    {
        MBShowDefaultImpl;
        MBHideDefaultImpl;
        MBRetranslateDefaultImpl;
        case MessageBus::UPDATE_COLORSCHEME: break;
    }
}

QJsonObject JsonEditor::OpenEditor()
{
    int resultCode = this->exec();
    auto string = jsonEditor->toPlainText();

    while (resultCode == QDialog::Accepted && VerifyJsonString(string).has_value())
    {
        QvBaselib->Warn(tr("Json Contains Syntax Errors"), tr("You must correct these errors before continuing."));
        resultCode = this->exec();
        string = jsonEditor->toPlainText();
    }

    return resultCode == QDialog::Accepted ? final : original;
}

JsonEditor::~JsonEditor()
{
}

void JsonEditor::on_jsonEditor_textChanged()
{
    auto string = jsonEditor->toPlainText();
    auto VerifyResult = VerifyJsonString(string);

    if (VerifyResult)
    {
        jsonValidateStatus->setText(*VerifyResult);
        RED(jsonEditor);
    }
    else
    {
        BLACK(jsonEditor);
        final = JsonFromString(string);
        model.loadJson(QJsonDocument(final).toJson());
        jsonTree->expandAll();
        jsonTree->resizeColumnToContents(0);
    }
}

void JsonEditor::on_formatJsonBtn_clicked()
{
    auto string = jsonEditor->toPlainText();
    auto VerifyResult = VerifyJsonString(string);

    if (VerifyResult)
    {
        jsonValidateStatus->setText(*VerifyResult);
        RED(jsonEditor);
        QvBaselib->Warn(tr("Syntax Errors"), tr("Please fix the JSON errors or remove the comments before continue"));
    }
    else
    {
        BLACK(jsonEditor);
        jsonEditor->setPlainText(JsonToString(JsonFromString(string)));
        model.loadJson(QJsonDocument(JsonFromString(string)).toJson());
        jsonTree->setModel(&model);
        jsonTree->expandAll();
        jsonTree->resizeColumnToContents(0);
    }
}

void JsonEditor::on_removeCommentsBtn_clicked()
{
    jsonEditor->setPlainText(JsonToString(JsonFromString(jsonEditor->toPlainText())));
}
