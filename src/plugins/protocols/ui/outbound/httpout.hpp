#pragma once

#include "Gui/QvGUIPluginInterface.hpp"
#include "V2RayModels.hpp"
#include "ui_httpout.h"

class HttpOutboundEditor
    : public Qv2rayPlugin::Gui::QvPluginEditor
    , private Ui::httpOutEditor
{
    Q_OBJECT

  public:
    explicit HttpOutboundEditor(QWidget *parent = nullptr);

    void SetHostAddress(const QString &server, int port) override
    {
        http.address = server;
        http.port = port;
    }

    QPair<QString, int> GetHostAddress() const override
    {
        return { http.address, http.port };
    }

    void SetContent(const IOProtocolSettings &source) override
    {
        auto servers = source["servers"].toArray();
        if (servers.isEmpty())
            return;
        const auto content = servers.first().toObject();

        http.loadJson(content);
        if (http.users->isEmpty())
            http.users->push_back({});

        http.users->first().user.ReadWriteBind(http_UserNameTxt, "text", &QLineEdit::textEdited);
        http.users->first().pass.ReadWriteBind(http_PasswordTxt, "text", &QLineEdit::textEdited);
    }

    const IOProtocolSettings GetContent() const override
    {
        auto result = http.toJson();
        if (http.users->isEmpty() || (http.users->first().user->isEmpty() && http.users->first().pass->isEmpty()))
            result.remove("users");
        return IOProtocolSettings{ QJsonObject{ { "servers", QJsonArray{ result } } } };
    }

  protected:
    void changeEvent(QEvent *e) override;

  private:
    Qv2ray::Models::HttpClientObject http;
};
