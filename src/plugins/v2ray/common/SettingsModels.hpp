#pragma once

#include "Utils/BindableProps.hpp"
#include "Utils/JsonConversion.hpp"

struct BrowserForwarderConfig
{
    Bindable<QString> ListeningAddress;
    Bindable<int> ListeningPort;
    QJS_FUNC_JSON(F(ListeningAddress, ListeningPort))
};

struct ProfileConnectionConfig
{
    Bindable<bool> UseV2RayDNSForDirectConnections;
    Bindable<bool> DNSInterception;
    QJS_FUNC_JSON(F(UseV2RayDNSForDirectConnections, DNSInterception))
};

struct V2RayCorePluginSettings
{
    enum V2RayLogLevel
    {
        None,
        Error,
        Warning,
        Info,
        Debug
    };

    Bindable<V2RayLogLevel> LogLevel;
    Bindable<QString> CorePath;
    Bindable<QString> AssetsPath;

    Bindable<bool> APIEnabled;
    Bindable<int> APIPort;

    ProfileConnectionConfig ConnectionSettings;
    BrowserForwarderConfig BrowserForwarderSettings;

    QJS_FUNC_JSON(F(LogLevel, CorePath, AssetsPath, APIEnabled, APIPort, ConnectionSettings, BrowserForwarderSettings))
};
