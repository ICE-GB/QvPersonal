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
    QJS_FUNC_COMPARE(HTTPSOCKSUserObject, user, pass, level);
};

//
// Socks, OutBound
struct SocksServerObject
{
    Bindable<QString> address{ "0.0.0.0" };
    Bindable<int> port = 0;
    Bindable<QList<HTTPSOCKSUserObject>> users;
    QJS_FUNC_JSON(P(address, port, users));
    QJS_FUNC_COMPARE(SocksServerObject, address, port, users);
};

//
// Http, OutBound
struct HttpServerObject
{
    Bindable<QString> address{ "0.0.0.0" };
    Bindable<int> port = 0;
    Bindable<QList<HTTPSOCKSUserObject>> users;
    QJS_FUNC_JSON(P(address, port, users))
    QJS_FUNC_COMPARE(HttpServerObject, address, port, users);
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
    QJS_FUNC_COMPARE(ShadowSocksServerObject, method, address, port, password)
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
        QJS_FUNC_COMPARE(UserObject, encryption, id, flow)
    };

    Bindable<QString> address;
    Bindable<int> port = 0;
    Bindable<QList<UserObject>> users;
    QJS_FUNC_JSON(P(address, port, users))
    QJS_FUNC_COMPARE(VLESSServerObject, address, port, users)
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
        QJS_FUNC_COMPARE(UserObject, id, alterId, security, level)
    };

    Bindable<QString> address;
    Bindable<int> port{ 0 };
    Bindable<QList<UserObject>> users;
    QJS_FUNC_JSON(F(address, port, users))
    QJS_FUNC_COMPARE(VMessServerObject, address, port, users)
};
