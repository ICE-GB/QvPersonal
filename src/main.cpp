#include "Qv2rayApplication.hpp"

#include <csignal>
#include <iostream>

#ifndef Q_OS_WIN
#include <unistd.h>
#endif

#define QV_MODULE_NAME "Init"

int globalArgc;
char **globalArgv;
void init_msgbox(const QString &title, const QString &text);
void signal_handler(int signum);

int main(int argc, char *argv[])
{
    globalArgc = argc;
    globalArgv = argv;
    // Register signal handlers.
    signal(SIGABRT, signal_handler);
    signal(SIGSEGV, signal_handler);
    signal(SIGTERM, signal_handler);
#ifndef Q_OS_WIN
    signal(SIGHUP, signal_handler);
    signal(SIGKILL, signal_handler);
#endif
    //
    // This line must be called before any other ones, since we are using these
    // values to identify instances.
    QCoreApplication::setApplicationVersion(QStringLiteral(QV2RAY_VERSION_STRING));

#ifdef QT_DEBUG
    QCoreApplication::setApplicationName(QStringLiteral("qv2ray_debug"));
#else
    QCoreApplication::setApplicationName("qv2ray");
#endif

    QApplication::setApplicationDisplayName(QStringLiteral("Qv2ray"));

#ifdef QT_DEBUG
    std::cerr << "WARNING: ================ This is a debug build, many features are not stable enough. ================" << std::endl;
#endif

    if (qEnvironmentVariableIsSet("QV2RAY_NO_SCALE_FACTORS"))
    {
        QvLog() << "Force set QT_SCALE_FACTOR to 1.";
        QvDebug() << "Original QT_SCALE_FACTOR was:" << qEnvironmentVariable("QT_SCALE_FACTOR");
        qputenv("QT_SCALE_FACTOR", "1");
    }
    else
    {
        QvDebug() << "High DPI scaling is enabled.";
        QGuiApplication::setHighDpiScaleFactorRoundingPolicy(Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);
    }

    Qv2rayApplication app(argc, argv);
    if (!app.Initialize())
    {
        const auto reason = app.GetExitReason();
        if (reason == EXIT_INITIALIZATION_FAILED)
        {
            init_msgbox(QStringLiteral("Qv2ray Initialization Failed"), "PreInitialization Failed." NEWLINE "For more information, please see the log.");
            QvLog() << "Qv2ray initialization failed:" << reason;
        }
        return reason;
    }

    app.RunQv2ray();

    const auto reason = app.GetExitReason();
    if (reason == EXIT_NEW_VERSION_TRIGGER)
    {
        QvLog() << "Starting new version of Qv2ray:" << app.StartupArguments._qvNewVersionPath;
        QProcess::startDetached(app.StartupArguments._qvNewVersionPath, {});
    }
    return reason;
}

void init_msgbox(const QString &title, const QString &text)
{
    if (qApp)
    {
        QMessageBox::warning(nullptr, title, text);
    }
    else
    {
        QApplication p(globalArgc, globalArgv);
        QMessageBox::warning(nullptr, title, text);
    }
}

void signal_handler(int signum)
{
    std::cout << "Signal: " << signum << std::endl;
#ifndef Q_OS_WIN
    if (signum == SIGTRAP)
    {
        exit(-99);
        return;
    }
#endif
    if (signum == SIGTERM)
    {
        if (qApp)
            qApp->exit();
        return;
    }
#if defined Q_OS_WIN || defined QT_DEBUG
    exit(-99);
#else
    kill(getpid(), SIGTRAP);
#endif
}
