#include "httpin.hpp"

#include "BuiltinProtocolPlugin.hpp"

HTTPInboundEditor::HTTPInboundEditor(QWidget *parent) : Qv2rayPlugin::Gui::QvPluginEditor(parent)
{
    setupUi(this);
    setProperty("QV2RAY_INTERNAL_HAS_STREAMSETTINGS", true);
}

void HTTPInboundEditor::changeEvent(QEvent *e)
{
    QWidget::changeEvent(e);
    switch (e->type())
    {
        case QEvent::LanguageChange: retranslateUi(this); break;
        default: break;
    }
}

void HTTPInboundEditor::SetContent(const IOProtocolSettings &content)
{
    PLUGIN_EDITOR_LOADING_SCOPE({
        this->content = content; // HTTP
        httpTimeoutSpinBox->setValue(content[QStringLiteral("timeout")].toInt());
        httpTransparentCB->setChecked(content[QStringLiteral("allowTransparent")].toBool());
        httpAccountListBox->clear();

        for (const auto &user : content[QStringLiteral("accounts")].toArray())
        {
            httpAccountListBox->addItem(user.toObject()[QStringLiteral("user")].toString() + ":" + user.toObject()[QStringLiteral("pass")].toString());
        }
    })
}

void HTTPInboundEditor::on_httpTimeoutSpinBox_valueChanged(int arg1)
{
    PLUGIN_EDITOR_LOADING_GUARD
    content[QStringLiteral("timeout")] = arg1;
}

void HTTPInboundEditor::on_httpTransparentCB_stateChanged(int arg1)
{
    PLUGIN_EDITOR_LOADING_GUARD
    content[QStringLiteral("allowTransparent")] = arg1 == Qt::Checked;
}

void HTTPInboundEditor::on_httpRemoveUserBtn_clicked()
{
    PLUGIN_EDITOR_LOADING_GUARD
    if (httpAccountListBox->currentRow() < 0)
    {
        PluginInstance->PluginErrorMessageBox(tr("Removing a user"), tr("You haven't selected a user yet."));
        return;
    }
    const auto item = httpAccountListBox->currentItem();
    auto list = content[QStringLiteral("accounts")].toArray();

    for (int i = 0; i < list.count(); i++)
    {
        const auto user = list[i].toObject();
        const auto entry = user[QStringLiteral("user")].toString() + ":" + user[QStringLiteral("pass")].toString();
        if (entry == item->text().trimmed())
        {
            list.removeAt(i);
            content[QStringLiteral("accounts")] = list;
            httpAccountListBox->takeItem(httpAccountListBox->currentRow());
            return;
        }
    }
}

void HTTPInboundEditor::on_httpAddUserBtn_clicked()
{
    PLUGIN_EDITOR_LOADING_GUARD
    const auto user = httpAddUserTxt->text();
    const auto pass = httpAddPasswordTxt->text();
    //
    auto list = content[QStringLiteral("accounts")].toArray();

    for (int i = 0; i < list.count(); i++)
    {
        const auto _user = list[i].toObject();
        if (_user[QStringLiteral("user")].toString() == user)
        {
            PluginInstance->PluginErrorMessageBox(tr("Add a user"), tr("This user exists already."));
            return;
        }
    }

    httpAddUserTxt->clear();
    httpAddPasswordTxt->clear();
    list.append(QJsonObject{ { "user", user }, { "pass", pass } });
    httpAccountListBox->addItem(user + ":" + pass);
    content[QStringLiteral("accounts")] = list;
}
