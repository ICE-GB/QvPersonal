#pragma once

#include "Utils/BindableProps.hpp"
#include "Utils/JsonConversion.hpp"

struct BrowserForwarderConfig
{
    Bindable<QString> listenAddr;
    Bindable<int> listenPort{ 18888 };
    QJS_JSON(P(listenAddr, listenPort))
};

struct ObservatoryConfig
{
    QStringList subjectSelector;
    QJS_JSON(F(subjectSelector))
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
    Bindable<int> OutboundMark{ 255 };

    Bindable<bool> APIEnabled{ true };
    Bindable<int> APIPort{ 15480 };

    BrowserForwarderConfig BrowserForwarderSettings;
    ObservatoryConfig ObservatorySettings;

    QJS_JSON(P(LogLevel, CorePath, AssetsPath, APIEnabled, APIPort, OutboundMark), F(BrowserForwarderSettings, ObservatorySettings))
};