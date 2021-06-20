#include "ProxyConfigurator.hpp"

#include "Common/Utils.hpp"
#include "Handlers/EventHandler.hpp"
#include "Plugin/PluginAPIHost.hpp"

#include <Plugin/PluginAPIHost.hpp>
#include <QHostAddress>
#include <Qv2rayBaseLibrary.hpp>

#ifdef Q_OS_WIN
//
#include <Windows.h>
//
#include <WinInet.h>
#include <ras.h>
#include <raserror.h>
#include <vector>
#endif

#define QV_MODULE_NAME "SystemProxy"

namespace Qv2ray::components::proxy
{

    using ProcessArgument = QPair<QString, QStringList>;
#ifdef Q_OS_MACOS
    QStringList macOSgetNetworkServices()
    {
        QProcess p;
        p.setProgram("/usr/sbin/networksetup");
        p.setArguments(QStringList{ "-listallnetworkservices" });
        p.start();
        p.waitForStarted();
        p.waitForFinished();
        // LOG(p.errorString());
        auto str = p.readAllStandardOutput();
        auto lines = SplitLines(str);
        QStringList result;

        // Start from 1 since first line is unneeded.
        for (auto i = 1; i < lines.count(); i++)
        {
            // * means disabled.
            if (!lines[i].contains("*"))
            {
                result << lines[i];
            }
        }

        // LOG("Found " + QString::number(result.size()) + " network services: " + result.join(";"));
        return result;
    }
#endif
#ifdef Q_OS_WIN
#define NO_CONST(expr) const_cast<wchar_t *>(expr)
    // static auto DEFAULT_CONNECTION_NAME =
    // NO_CONST(L"DefaultConnectionSettings");
    ///
    /// INTERNAL FUNCTION
    bool __QueryProxyOptions()
    {
        INTERNET_PER_CONN_OPTION_LIST List;
        INTERNET_PER_CONN_OPTION Option[5];
        //
        unsigned long nSize = sizeof(INTERNET_PER_CONN_OPTION_LIST);
        Option[0].dwOption = INTERNET_PER_CONN_AUTOCONFIG_URL;
        Option[1].dwOption = INTERNET_PER_CONN_AUTODISCOVERY_FLAGS;
        Option[2].dwOption = INTERNET_PER_CONN_FLAGS;
        Option[3].dwOption = INTERNET_PER_CONN_PROXY_BYPASS;
        Option[4].dwOption = INTERNET_PER_CONN_PROXY_SERVER;
        //
        List.dwSize = sizeof(INTERNET_PER_CONN_OPTION_LIST);
        List.pszConnection = nullptr; // NO_CONST(DEFAULT_CONNECTION_NAME);
        List.dwOptionCount = 5;
        List.dwOptionError = 0;
        List.pOptions = Option;

        if (!InternetQueryOption(nullptr, INTERNET_OPTION_PER_CONNECTION_OPTION, &List, &nSize))
        {
            // LOG("InternetQueryOption failed, GLE=" + QString::number(GetLastError()));
        }

        // LOG("System default proxy info:");

        if (Option[0].Value.pszValue != nullptr)
        {
            // LOG(QString::fromWCharArray(Option[0].Value.pszValue));
        }

        if ((Option[2].Value.dwValue & PROXY_TYPE_AUTO_PROXY_URL) == PROXY_TYPE_AUTO_PROXY_URL)
        {
            // LOG("PROXY_TYPE_AUTO_PROXY_URL");
        }

        if ((Option[2].Value.dwValue & PROXY_TYPE_AUTO_DETECT) == PROXY_TYPE_AUTO_DETECT)
        {
            // LOG("PROXY_TYPE_AUTO_DETECT");
        }

        if ((Option[2].Value.dwValue & PROXY_TYPE_DIRECT) == PROXY_TYPE_DIRECT)
        {
            // LOG("PROXY_TYPE_DIRECT");
        }

        if ((Option[2].Value.dwValue & PROXY_TYPE_PROXY) == PROXY_TYPE_PROXY)
        {
            // LOG("PROXY_TYPE_PROXY");
        }

        if (!InternetQueryOption(nullptr, INTERNET_OPTION_PER_CONNECTION_OPTION, &List, &nSize))
        {
            // LOG("InternetQueryOption failed,GLE=" + QString::number(GetLastError()));
        }

        if (Option[4].Value.pszValue != nullptr)
        {
            // LOG(QString::fromStdWString(Option[4].Value.pszValue));
        }

        INTERNET_VERSION_INFO Version;
        nSize = sizeof(INTERNET_VERSION_INFO);
        InternetQueryOption(nullptr, INTERNET_OPTION_VERSION, &Version, &nSize);

        if (Option[0].Value.pszValue != nullptr)
        {
            GlobalFree(Option[0].Value.pszValue);
        }

        if (Option[3].Value.pszValue != nullptr)
        {
            GlobalFree(Option[3].Value.pszValue);
        }

        if (Option[4].Value.pszValue != nullptr)
        {
            GlobalFree(Option[4].Value.pszValue);
        }

        return false;
    }
    bool __SetProxyOptions(LPWSTR proxy_full_addr, bool isPAC)
    {
        INTERNET_PER_CONN_OPTION_LIST list;
        DWORD dwBufSize = sizeof(list);
        // Fill the list structure.
        list.dwSize = sizeof(list);
        // NULL == LAN, otherwise connectoid name.
        list.pszConnection = nullptr;

        if (nullptr == proxy_full_addr)
        {
            // LOG("Clearing system proxy");
            //
            list.dwOptionCount = 1;
            list.pOptions = new INTERNET_PER_CONN_OPTION[1];

            // Ensure that the memory was allocated.
            if (nullptr == list.pOptions)
            {
                // Return if the memory wasn't allocated.
                return false;
            }

            // Set flags.
            list.pOptions[0].dwOption = INTERNET_PER_CONN_FLAGS;
            list.pOptions[0].Value.dwValue = PROXY_TYPE_DIRECT;
        }
        else if (isPAC)
        {
            // LOG("Setting system proxy for PAC");
            //
            list.dwOptionCount = 2;
            list.pOptions = new INTERNET_PER_CONN_OPTION[2];

            if (nullptr == list.pOptions)
            {
                return false;
            }

            // Set flags.
            list.pOptions[0].dwOption = INTERNET_PER_CONN_FLAGS;
            list.pOptions[0].Value.dwValue = PROXY_TYPE_DIRECT | PROXY_TYPE_AUTO_PROXY_URL;
            // Set proxy name.
            list.pOptions[1].dwOption = INTERNET_PER_CONN_AUTOCONFIG_URL;
            list.pOptions[1].Value.pszValue = proxy_full_addr;
        }
        else
        {
            // LOG("Setting system proxy for Global Proxy");
            //
            list.dwOptionCount = 2;
            list.pOptions = new INTERNET_PER_CONN_OPTION[2];

            if (nullptr == list.pOptions)
            {
                return false;
            }

            // Set flags.
            list.pOptions[0].dwOption = INTERNET_PER_CONN_FLAGS;
            list.pOptions[0].Value.dwValue = PROXY_TYPE_DIRECT | PROXY_TYPE_PROXY;
            // Set proxy name.
            list.pOptions[1].dwOption = INTERNET_PER_CONN_PROXY_SERVER;
            list.pOptions[1].Value.pszValue = proxy_full_addr;
            // Set proxy override.
            // list.pOptions[2].dwOption = INTERNET_PER_CONN_PROXY_BYPASS;
            // auto localhost = L"localhost";
            // list.pOptions[2].Value.pszValue = NO_CONST(localhost);
        }

        // Set proxy for LAN.
        if (!InternetSetOption(nullptr, INTERNET_OPTION_PER_CONNECTION_OPTION, &list, dwBufSize))
        {
            // LOG("InternetSetOption failed for LAN, GLE=" + QString::number(GetLastError()));
        }

        RASENTRYNAME entry;
        entry.dwSize = sizeof(entry);
        std::vector<RASENTRYNAME> entries;
        DWORD size = sizeof(entry), count;
        LPRASENTRYNAME entryAddr = &entry;
        auto ret = RasEnumEntries(nullptr, nullptr, entryAddr, &size, &count);
        if (ERROR_BUFFER_TOO_SMALL == ret)
        {
            entries.resize(count);
            entries[0].dwSize = sizeof(RASENTRYNAME);
            entryAddr = entries.data();
            ret = RasEnumEntries(nullptr, nullptr, entryAddr, &size, &count);
        }
        if (ERROR_SUCCESS != ret)
        {
            // LOG("Failed to list entry names");
            return false;
        }

        // Set proxy for each connectoid.
        for (DWORD i = 0; i < count; ++i)
        {
            list.pszConnection = entryAddr[i].szEntryName;
            if (!InternetSetOption(nullptr, INTERNET_OPTION_PER_CONNECTION_OPTION, &list, dwBufSize))
            {
                // LOG("InternetSetOption failed for connectoid " + QString::fromWCharArray(list.pszConnection) + ", GLE=" + QString::number(GetLastError()));
            }
        }

        delete[] list.pOptions;
        InternetSetOption(nullptr, INTERNET_OPTION_SETTINGS_CHANGED, nullptr, 0);
        InternetSetOption(nullptr, INTERNET_OPTION_REFRESH, nullptr, 0);
        return true;
    }
#endif

    void SetSystemProxy(const QString &address, int httpPort, int socksPort)
    {
        QvLog() << "Setting up System Proxy";
        bool hasHTTP = (httpPort > 0 && httpPort < 65536);
        bool hasSOCKS = (socksPort > 0 && socksPort < 65536);

#ifdef Q_OS_WIN
        if (!hasHTTP)
        {
            // LOG("Nothing?");
            return;
        }
        else
        {
            // LOG("Qv2ray will set system proxy to use HTTP");
        }
#else
        if (!hasHTTP && !hasSOCKS)
        {
            Qv// LOG() << "Nothing?";
            return;
        }

        if (hasHTTP)
        {
            Qv// LOG() << "Qv2ray will set system proxy to use HTTP";
        }

        if (hasSOCKS)
        {
            Qv// LOG() << "Qv2ray will set system proxy to use SOCKS";
        }
#endif

#ifdef Q_OS_WIN
        QString __a;
        const QHostAddress ha(address);
        const auto type = ha.protocol();
        if (type == QAbstractSocket::IPv6Protocol)
        {
            // many software do not recognize IPv6 proxy server string though
            const auto str = ha.toString(); // RFC5952
            __a = "[" + str + "]:" + QString::number(httpPort);
        }
        else
        {
            __a = address + ":" + QString::number(httpPort);
        }

        // LOG("Windows proxy string: " + __a);
        auto proxyStrW = new WCHAR[__a.length() + 1];
        wcscpy(proxyStrW, __a.toStdWString().c_str());
        //
        __QueryProxyOptions();

        if (!__SetProxyOptions(proxyStrW, false))
        {
            // LOG("Failed to set proxy.");
        }

        __QueryProxyOptions();
#elif defined(Q_OS_LINUX)
        QList<ProcessArgument> actions;
        actions << ProcessArgument{ "gsettings", { "set", "org.gnome.system.proxy", "mode", "manual" } };
        //
        bool isKDE = qEnvironmentVariable("XDG_SESSION_DESKTOP") == "KDE" || qEnvironmentVariable("XDG_SESSION_DESKTOP") == "plasma";
        const auto configPath = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation);

        //
        // Configure HTTP Proxies for HTTP, FTP and HTTPS
        if (hasHTTP)
        {
            // iterate over protocols...
            for (const auto &protocol : QStringList{ "http", "ftp", "https" })
            {
                // for GNOME:
                {
                    actions << ProcessArgument{ "gsettings", { "set", "org.gnome.system.proxy." + protocol, "host", address } };
                    actions << ProcessArgument{ "gsettings", { "set", "org.gnome.system.proxy." + protocol, "port", QString::number(httpPort) } };
                }

                // for KDE:
                if (isKDE)
                {
                    actions << ProcessArgument{ "kwriteconfig5",
                                                { "--file", configPath + "/kioslaverc", //
                                                  "--group", "Proxy Settings",          //
                                                  "--key", protocol + "Proxy",          //
                                                  "http://" + address + " " + QString::number(httpPort) } };
                }
            }
        }

        // Configure SOCKS5 Proxies
        if (hasSOCKS)
        {
            // for GNOME:
            {
                actions << ProcessArgument{ "gsettings", { "set", "org.gnome.system.proxy.socks", "host", address } };
                actions << ProcessArgument{ "gsettings", { "set", "org.gnome.system.proxy.socks", "port", QString::number(socksPort) } };

                // for KDE:
                if (isKDE)
                {
                    actions << ProcessArgument{ "kwriteconfig5",
                                                { "--file", configPath + "/kioslaverc", //
                                                  "--group", "Proxy Settings",          //
                                                  "--key", "socksProxy",                //
                                                  "socks://" + address + " " + QString::number(socksPort) } };
                }
            }
        }
        // Setting Proxy Mode to Manual
        {
            // for GNOME:
            {
                actions << ProcessArgument{ "gsettings", { "set", "org.gnome.system.proxy", "mode", "manual" } };
            }

            // for KDE:
            if (isKDE)
            {
                actions << ProcessArgument{ "kwriteconfig5",
                                            { "--file", configPath + "/kioslaverc", //
                                              "--group", "Proxy Settings",          //
                                              "--key", "ProxyType", "1" } };
            }
        }

        // Notify kioslaves to reload system proxy configuration.
        if (isKDE)
        {
            actions << ProcessArgument{ "dbus-send",
                                        { "--type=signal", "/KIO/Scheduler",                 //
                                          "org.kde.KIO.Scheduler.reparseSlaveConfiguration", //
                                          "string:''" } };
        }
        // Execute them all!
        //
        // note: do not use std::all_of / any_of / none_of,
        // because those are short-circuit and cannot guarantee atomicity.
        QList<bool> results;
        for (const auto &action : actions)
        {
            // execute and get the code
            const auto returnCode = QProcess::execute(action.first, action.second);
            // print out the commands and result codes
            QvDebug() << action.first << action.second.join(";") << returnCode;
            // give the code back
            results << (returnCode == QProcess::NormalExit);
        }

        if (results.count(true) != actions.size())
        {
            Qv// LOG() << "Something wrong when setting proxies.";
        }
#else

        for (const auto &service : macOSgetNetworkServices())
        {
            // LOG("Setting proxy for interface: " + service);
            if (hasHTTP)
            {
                QProcess::execute("/usr/sbin/networksetup", { "-setwebproxystate", service, "on" });
                QProcess::execute("/usr/sbin/networksetup", { "-setsecurewebproxystate", service, "on" });
                QProcess::execute("/usr/sbin/networksetup", { "-setwebproxy", service, address, QString::number(httpPort) });
                QProcess::execute("/usr/sbin/networksetup", { "-setsecurewebproxy", service, address, QString::number(httpPort) });
            }

            if (hasSOCKS)
            {
                QProcess::execute("/usr/sbin/networksetup", { "-setsocksfirewallproxystate", service, "on" });
                QProcess::execute("/usr/sbin/networksetup", { "-setsocksfirewallproxy", service, address, QString::number(socksPort) });
            }
        }

#endif
        //
        // Trigger plugin events
        QMap<Qv2rayPlugin::SystemProxy::SystemProxyType, int> portSettings;
        if (hasHTTP)
            portSettings.insert(Qv2rayPlugin::SystemProxy::SystemProxyType::SystemProxy_HTTP, httpPort);
        if (hasSOCKS)
            portSettings.insert(Qv2rayPlugin::SystemProxy::SystemProxyType::SystemProxy_SOCKS, socksPort);
        QvBaselib->PluginAPIHost()->Event_Send<Qv2rayPlugin::SystemProxy>({ portSettings, Qv2rayPlugin::SystemProxy::SetProxy });
    }

    void ClearSystemProxy()
    {
        QvLog() << "Clearing System Proxy";

#ifdef Q_OS_WIN
        if (!__SetProxyOptions(nullptr, false))
        {
            // LOG("Failed to clear proxy.");
        }
#elif defined(Q_OS_LINUX)
        QList<ProcessArgument> actions;
        const bool isKDE = qEnvironmentVariable("XDG_SESSION_DESKTOP") == "KDE" || qEnvironmentVariable("XDG_SESSION_DESKTOP") == "plasma";
        const auto configRoot = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation);

        // Setting System Proxy Mode to: None
        {
            // for GNOME:
            {
                actions << ProcessArgument{ "gsettings", { "set", "org.gnome.system.proxy", "mode", "none" } };
            }

            // for KDE:
            if (isKDE)
            {
                actions << ProcessArgument{ "kwriteconfig5",
                                            { "--file", configRoot + "/kioslaverc", //
                                              "--group", "Proxy Settings",          //
                                              "--key", "ProxyType", "0" } };
            }
        }

        // Notify kioslaves to reload system proxy configuration.
        if (isKDE)
        {
            actions << ProcessArgument{ "dbus-send",
                                        { "--type=signal", "/KIO/Scheduler",                 //
                                          "org.kde.KIO.Scheduler.reparseSlaveConfiguration", //
                                          "string:''" } };
        }

        // Execute the Actions
        for (const auto &action : actions)
        {
            // execute and get the code
            const auto returnCode = QProcess::execute(action.first, action.second);
            // print out the commands and result codes
            QvDebug() << action.first << action.second.join(";") << returnCode;
        }

#else
        for (const auto &service : macOSgetNetworkServices())
        {
            // LOG("Clearing proxy for interface: " + service);
            QProcess::execute("/usr/sbin/networksetup", { "-setautoproxystate", service, "off" });
            QProcess::execute("/usr/sbin/networksetup", { "-setwebproxystate", service, "off" });
            QProcess::execute("/usr/sbin/networksetup", { "-setsecurewebproxystate", service, "off" });
            QProcess::execute("/usr/sbin/networksetup", { "-setsocksfirewallproxystate", service, "off" });
        }

#endif
        //
        // Trigger plugin events
        QvBaselib->PluginAPIHost()->Event_Send<Qv2rayPlugin::Event::SystemProxy>({ {}, Qv2rayPlugin::Event::SystemProxy::ClearProxy });
    }
} // namespace Qv2ray::components::proxy
