#include "Qv2rayApplication.hpp"

#include "Common/Utils.hpp"
#include "GuiPluginHost/GuiPluginHost.hpp"
#include "Profile/KernelManager.hpp"
#include "Profile/ProfileManager.hpp"
#include "StyleManager/StyleManager.hpp"
#include "models/SettingsModels.hpp"
#include "ui/windows/w_MainWindow.hpp"

#include <QDesktopServices>
#include <QSessionManager>
#include <QSslSocket>

#define QV_MODULE_NAME "PlatformApplication"

#ifdef QT_DEBUG
const static inline auto QV2RAY_URL_SCHEME = QStringLiteral("qv2ray-debug");
#else
const static inline QString QV2RAY_URL_SCHEME = "qv2ray";
#endif

Qv2rayApplication::Qv2rayApplication(int &argc, char *argv[]) : SingleApplication(argc, argv, true, User | ExcludeAppPath | ExcludeAppVersion)
{
    baseLibrary = new Qv2rayBase::Qv2rayBaseLibrary;
}

Qv2rayApplication::~Qv2rayApplication()
{
    delete baseLibrary;
}

Qv2rayExitReason Qv2rayApplication::GetExitReason() const
{
    return _exitReason;
}

QStringList Qv2rayApplication::CheckPrerequisites()
{
    QStringList errors;
    if (!QSslSocket::supportsSsl())
    {
        // Check OpenSSL version for auto-update and subscriptions
        const auto osslReqVersion = QSslSocket::sslLibraryBuildVersionString();
        const auto osslCurVersion = QSslSocket::sslLibraryVersionString();
        QvLog() << "Current OpenSSL version:" << osslCurVersion;
        QvLog() << "Required OpenSSL version:" << osslReqVersion;
        errors << QStringLiteral("Qv2ray cannot run without OpenSSL.");
        errors << QStringLiteral("This is usually caused by using the wrong version of OpenSSL");
        errors << "Required=" + osslReqVersion + "Current=" + osslCurVersion;
    }
    return errors;
}

bool Qv2rayApplication::Initialize()
{
    QString errorMessage;
    bool canContinue;
    const auto hasError = parseCommandLine(&errorMessage, &canContinue);
    if (hasError)
    {
        QvLog() << "Command line:" << errorMessage;
        if (!canContinue)
        {
            QvLog() << "Fatal, Qv2ray cannot continue.";
            return false;
        }
    }

#ifdef Q_OS_WIN
    const auto appPath = QDir::toNativeSeparators(QCoreApplication::applicationFilePath());
    const auto regPath = "HKEY_CURRENT_USER\\Software\\Classes\\" + QV2RAY_URL_SCHEME;
    QSettings reg(regPath, QSettings::NativeFormat);
    reg.setValue("Default", "Qv2ray");
    reg.setValue("URL Protocol", "");
    reg.beginGroup("DefaultIcon");
    reg.setValue("Default", QString("%1,1").arg(appPath));
    reg.endGroup();
    reg.beginGroup("shell");
    reg.beginGroup("open");
    reg.beginGroup("command");
    reg.setValue("Default", appPath + " %1");
#endif

    connect(this, &QApplication::aboutToQuit, this, &Qv2rayApplication::quitInternal);
    connect(this, &SingleApplication::receivedMessage, this, &Qv2rayApplication::onMessageReceived, Qt::QueuedConnection);
    if (isSecondary())
    {
        StartupArguments.version = QV2RAY_VERSION_STRING;
        //        StartupArguments.buildVersion = QV2RAY_VERSION_BUILD;
        StartupArguments.fullArgs = arguments();
        if (StartupArguments.arguments.isEmpty())
            StartupArguments.arguments << Qv2rayStartupArguments::NORMAL;
        bool status = sendMessage(JsonToString(StartupArguments.toJson(), QJsonDocument::Compact).toUtf8());
        if (!status)
            QvLog() << "Cannot send message.";
        _exitReason = EXIT_SECONDARY_INSTANCE;
        return false;
    }

    const auto result = baseLibrary->Initialize(StartupArguments.noPlugins ? Qv2rayBase::START_NO_PLUGINS : Qv2rayBase::START_NORMAL, this);
    GlobalConfig = new (std::remove_reference_t<decltype(*GlobalConfig)>);

#ifdef Q_OS_LINUX
    connect(this, &QGuiApplication::commitDataRequest,
            []
            {
#pragma message("TODO")
                // RouteManager->SaveRoutes();
                // SaveConnectionConfig();
                //        SaveGlobalSettings();
            });
#endif

#ifdef Q_OS_WIN
    SetCurrentDirectory(QCoreApplication::applicationDirPath().toStdWString().c_str());
    // Set font
    QFont font;
    font.setPointSize(9);
    font.setFamily("Microsoft YaHei");
    QGuiApplication::setFont(font);
#endif
    return true;
}

Qv2rayExitReason Qv2rayApplication::RunQv2ray()
{
    setQuitOnLastWindowClosed(false);
    GUIPluginHost = new Qv2ray::ui::common::GuiPluginAPIHost(baseLibrary->PluginAPIHost());
    hTray = new QSystemTrayIcon();
    StyleManager = new QvStyleManager();
    StyleManager->ApplyStyle(GlobalConfig->appearanceConfig->UITheme);
    // Show MainWindow
    mainWindow = new MainWindow();
    if (StartupArguments.arguments.contains(Qv2rayStartupArguments::QV2RAY_LINK))
    {
        for (const auto &link : StartupArguments.links)
        {
            const auto url = QUrl::fromUserInput(link);
            const auto command = url.host();
            auto subcommands = url.path().split(QStringLiteral("/"));
            subcommands.removeAll("");
            QMap<QString, QString> args;
            for (const auto &kvp : QUrlQuery(url).queryItems())
            {
                args.insert(kvp.first, kvp.second);
            }
            if (command == "open")
            {
                mainWindow->ProcessCommand(command, subcommands, args);
            }
        }
    }
#ifdef Q_OS_MACOS
    connect(this, &QApplication::applicationStateChanged, [this](Qt::ApplicationState state) {
        switch (state)
        {
            case Qt::ApplicationActive:
            {
                mainWindow->show();
                mainWindow->raise();
                mainWindow->activateWindow();
                break;
            }
            case Qt::ApplicationHidden:
            case Qt::ApplicationInactive:
            case Qt::ApplicationSuspended: break;
        }
    });
#endif
    isInitialized = true;
    return (Qv2rayExitReason) exec();
}

QSystemTrayIcon **Qv2rayApplication::TrayIcon()
{
    return &hTray;
}

void Qv2rayApplication::quitInternal()
{
    delete mainWindow;
    delete hTray;
    delete StyleManager;
    delete GUIPluginHost;
    QvBaselib->Shutdown();
}

bool Qv2rayApplication::parseCommandLine(QString *errorMessage, bool *canContinue)
{
    *canContinue = true;
    QStringList filteredArgs;
    for (const auto &arg : QCoreApplication::arguments())
    {
#ifdef Q_OS_MACOS
        if (arg.contains("-psn"))
            continue;
#endif
        filteredArgs << arg;
    }
    QCommandLineParser parser;
    //
    QCommandLineOption noAPIOption("noAPI", QObject::tr("Disable gRPC API subsystem"));
    QCommandLineOption noPluginsOption("noPlugin", QObject::tr("Disable plugins feature"));
    QCommandLineOption debugLogOption("debug", QObject::tr("Enable debug output"));
    QCommandLineOption noAutoConnectionOption("noAutoConnection", QObject::tr("Do not automatically connect"));
    QCommandLineOption disconnectOption("disconnect", QObject::tr("Stop current connection"));
    QCommandLineOption reconnectOption("reconnect", QObject::tr("Reconnect last connection"));
    QCommandLineOption exitOption("exit", QObject::tr("Exit Qv2ray"));
    //
    parser.setApplicationDescription(QObject::tr("Qv2ray - A cross-platform Qt frontend for V2Ray."));
    parser.setSingleDashWordOptionMode(QCommandLineParser::ParseAsLongOptions);
    //
    parser.addOption(noAPIOption);
    parser.addOption(noPluginsOption);
    parser.addOption(debugLogOption);
    parser.addOption(noAutoConnectionOption);
    parser.addOption(disconnectOption);
    parser.addOption(reconnectOption);
    parser.addOption(exitOption);
    //
    const auto helpOption = parser.addHelpOption();
    const auto versionOption = parser.addVersionOption();

    if (!parser.parse(filteredArgs))
    {
        *canContinue = true;
        *errorMessage = parser.errorText();
        return false;
    }

    if (parser.isSet(versionOption))
    {
        parser.showVersion();
        return true;
    }

    if (parser.isSet(helpOption))
    {
        parser.showHelp();
        return true;
    }

    for (const auto &arg : parser.positionalArguments())
    {
        if (arg.startsWith(QV2RAY_URL_SCHEME + "://"))
        {
            StartupArguments.arguments << Qv2rayStartupArguments::QV2RAY_LINK;
            StartupArguments.links << arg;
        }
    }

    if (parser.isSet(exitOption))
    {
        QvDebug() << "exitOption is set.";
        StartupArguments.arguments << Qv2rayStartupArguments::EXIT;
    }

    if (parser.isSet(disconnectOption))
    {
        QvDebug() << "disconnectOption is set.";
        StartupArguments.arguments << Qv2rayStartupArguments::DISCONNECT;
    }

    if (parser.isSet(reconnectOption))
    {
        QvDebug() << "reconnectOption is set.";
        StartupArguments.arguments << Qv2rayStartupArguments::RECONNECT;
    }

#define ProcessExtraStartupOptions(option)                                                                                                                               \
    QvDebug() << "Startup Options:" << #option << parser.isSet(option##Option);                                                                                          \
    StartupArguments.option = parser.isSet(option##Option);

    ProcessExtraStartupOptions(noAPI);
    ProcessExtraStartupOptions(debugLog);
    ProcessExtraStartupOptions(noAutoConnection);
    ProcessExtraStartupOptions(noPlugins);
    return true;
}

void Qv2rayApplication::p_OpenURL(const QUrl &url)
{
    QDesktopServices::openUrl(url);
}

void Qv2rayApplication::p_MessageBoxWarn(const QString &title, const QString &text)
{
    QMessageBox::warning(nullptr, title, text, QMessageBox::Ok);
}

void Qv2rayApplication::p_MessageBoxInfo(const QString &title, const QString &text)
{
    QMessageBox::information(nullptr, title, text, QMessageBox::Ok);
}

Qv2rayBase::MessageOpt Qv2rayApplication::p_MessageBoxAsk(const QString &title, const QString &text, const QList<Qv2rayBase::MessageOpt> &buttons)
{
    QFlags<QMessageBox::StandardButton> btns;
    for (const auto &b : buttons)
    {
        btns.setFlag(MessageBoxButtonMap.value(b));
    }
    return MessageBoxButtonMap.key(QMessageBox::question(nullptr, title, text, btns));
}

void Qv2rayApplication::ShowTrayMessage(const QString &m, int msecs)
{
    hTray->showMessage("Qv2ray", m, QIcon(":/assets/icons/qv2ray.png"), msecs);
}

bool Qv2rayApplication::isDarkMode()
{
    return true;
}

void Qv2rayApplication::onMessageReceived(quint32 clientId, QByteArray _msg)
{
    // Sometimes SingleApplication will send message with clientId == 0, ignore them.
    if (clientId == instanceId())
        return;

    if (!isInitialized)
        return;

    Qv2rayStartupArguments msg;
    msg.loadJson(JsonFromString(_msg));
    QvLog() << "Received message, version:" << msg.buildVersion << "From client ID:" << clientId;
    QvLog() << _msg;
    //
#pragma message("TODO")
    //    if (msg.buildVersion > QV2RAY_VERSION_BUILD)
    //    {
    //        const auto newPath = msg.fullArgs.first();
    //        QString message;
    //        message += tr("A new version of Qv2ray is starting:") + NEWLINE;
    //        message += NEWLINE;
    //        message += tr("New version information: ") + NEWLINE;
    //        message += tr("Version: %1:%2").arg(msg.version).arg(msg.buildVersion) + NEWLINE;
    //        message += tr("Path: %1").arg(newPath) + NEWLINE;
    //        message += NEWLINE;
    //        message += tr("Do you want to exit and launch that new version?");

    //        const auto result = p_MessageBoxAsk(tr("New version detected"), message, { Qv2rayBase::MessageOpt::Yes, Qv2rayBase::MessageOpt::No });
    //        if (result == Qv2rayBase::MessageOpt::Yes)
    //        {
    //            StartupArguments._qvNewVersionPath = newPath;
    //            _exitReason = EXIT_NEW_VERSION_TRIGGER;
    //            QCoreApplication::quit();
    //        }
    //    }

    for (const auto &argument : msg.arguments)
    {
        switch (argument)
        {
            case Qv2rayStartupArguments::EXIT:
            {
                _exitReason = EXIT_NORMAL;
                quit();
                break;
            }
            case Qv2rayStartupArguments::NORMAL:
            {
                mainWindow->show();
                mainWindow->raise();
                mainWindow->activateWindow();
                break;
            }
            case Qv2rayStartupArguments::RECONNECT:
            {
                QvBaselib->ProfileManager()->StartConnection(QvBaselib->KernelManager()->CurrentConnection());
                break;
            }
            case Qv2rayStartupArguments::DISCONNECT:
            {
                QvBaselib->ProfileManager()->StopConnection();
                break;
            }
            case Qv2rayStartupArguments::QV2RAY_LINK:
            {
                for (const auto &link : msg.links)
                {
                    const auto url = QUrl::fromUserInput(link);
                    const auto command = url.host();
                    auto subcommands = url.path().split("/");
                    subcommands.removeAll("");
                    QMap<QString, QString> args;
                    for (const auto &kvp : QUrlQuery(url).queryItems())
                    {
                        args.insert(kvp.first, kvp.second);
                    }
                    if (command == "open")
                    {
                        mainWindow->ProcessCommand(command, subcommands, args);
                    }
                }
                break;
            }
        }
    }
}
