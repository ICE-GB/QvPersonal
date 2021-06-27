#pragma once

#include "Utils/BindableProps.hpp"
#include "Utils/JsonConversion.hpp"

struct BrowserForwarderConfig
{
    Bindable<QString> listenAddr;
    Bindable<int> listenPort;
    QJS_FUNC_JSON(F(listenAddr, listenPort))
};

struct ObservatoryConfig
{
    QStringList subjectSelector;
    QJS_FUNC_JSON(F(subjectSelector))
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

    BrowserForwarderConfig BrowserForwarderSettings;
    ObservatoryConfig ObservatorySettings;

    QJS_FUNC_JSON(F(LogLevel, CorePath, AssetsPath, APIEnabled, APIPort, BrowserForwarderSettings, ObservatorySettings))
};
