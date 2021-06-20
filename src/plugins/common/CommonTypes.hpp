#pragma once
#include "Utils/BindableProps.hpp"
#include "Utils/JsonConversion.hpp"

// GUI TOOLS
#define RED(obj)                                                                                                                                     \
    {                                                                                                                                                \
        auto _temp = obj->palette();                                                                                                                 \
        _temp.setColor(QPalette::Text, Qt::red);                                                                                                     \
        obj->setPalette(_temp);                                                                                                                      \
    }

#define BLACK(obj) obj->setPalette(QWidget::palette());

struct HTTPSOCKSUserObject
{
    Bindable<QString> user;
    Bindable<QString> pass;
    Bindable<int> level{ 0 };
    QJS_FUNC_JSON(P(user, pass, level))
    bool operator==(const HTTPSOCKSUserObject &) const = default;
};

//
// Socks, OutBound
struct SocksServerObject
{
    Bindable<QString> address{ "0.0.0.0" };
    Bindable<int> port = 0;
    Bindable<QList<HTTPSOCKSUserObject>> users;
    QJS_FUNC_JSON(P(address, port, users))
};

//
// Http, OutBound
struct HttpServerObject
{
    Bindable<QString> address{ "0.0.0.0" };
    Bindable<int> port = 0;
    Bindable<QList<HTTPSOCKSUserObject>> users;
    QJS_FUNC_JSON(P(address, port, users))
};

//
// ShadowSocks Server
struct ShadowSocksServerObject
{
    Bindable<QString> address{ "0.0.0.0" };
    Bindable<QString> method{ "aes-256-gcm" };
    Bindable<QString> password;
    Bindable<int> port{ 0 };
    QJS_FUNC_JSON(P(method, address, port, password))
};

//
// VLESS Server
struct VLESSServerObject
{
    struct UserObject
    {
        Bindable<QString> id;
        Bindable<QString> encryption{ "none" };
        Bindable<QString> flow;
        QJS_FUNC_JSON(P(encryption, id, flow))
        bool operator==(const UserObject &) const = default;
    };

    Bindable<QString> address;
    Bindable<int> port = 0;
    Bindable<QList<UserObject>> users;
    QJS_FUNC_JSON(P(address, port, users))
};

//
// VMess Server
constexpr auto VMESS_USER_ALTERID_DEFAULT = 0;
struct VMessServerObject
{
    struct UserObject
    {
        Bindable<QString> id;
        Bindable<int> alterId{ VMESS_USER_ALTERID_DEFAULT };
        Bindable<QString> security{ "auto" };
        Bindable<int> level{ 0 };
        QJS_FUNC_JSON(F(id, alterId, security, level))
    };

    Bindable<QString> address;
    Bindable<int> port{ 0 };
    Bindable<QList<UserObject>> users;
    QJS_FUNC_JSON(F(address, port, users))
};
