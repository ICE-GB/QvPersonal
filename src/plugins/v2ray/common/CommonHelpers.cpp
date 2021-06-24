#include "CommonHelpers.hpp"

#include <QRegularExpression>

QStringList SplitLines(const QString &_string)
{
    return _string.split(QRegularExpression(QStringLiteral("[\r\n]")), Qt::SkipEmptyParts);
}

std::pair<bool, std::optional<QString>> ValidateKernel(const QString &corePath, const QString &assetsPath)
{
    QFile coreFile(corePath);

    if (!coreFile.exists())
        return { false, QObject::tr("V2Ray core executable not found.") };

    // Use open() here to prevent `executing` a folder, which may have the
    // same name as the V2Ray core.
    if (!coreFile.open(QFile::ReadOnly))
        return { false, QObject::tr("V2Ray core file cannot be opened, please ensure there's a file instead of a folder.") };

    coreFile.close();

    //
    // Check file existance.
    // From: https://www.v2fly.org/chapter_02/env.html#asset-location
    bool hasGeoIP = QDir(assetsPath).entryList().contains("geoip.dat");
    bool hasGeoSite = QDir(assetsPath).entryList().contains("geosite.dat");

    if (!hasGeoIP && !hasGeoSite)
        return { false, QObject::tr("V2Ray assets path is not valid.") };

    if (!hasGeoIP)
        return { false, QObject::tr("No geoip.dat in assets path.") };

    if (!hasGeoSite)
        return { false, QObject::tr("No geosite.dat in assets path.") };

    // Check if V2Ray core returns a version number correctly.
    QProcess proc;
#ifdef Q_OS_WIN32
    // nativeArguments are required for Windows platform, without a
    // reason...
    proc.setProcessChannelMode(QProcess::MergedChannels);
    proc.setProgram(corePath);
    proc.setNativeArguments("--version");
    proc.start();
#else
    proc.start(corePath, { "--version" });
#endif
    proc.waitForStarted();
    proc.waitForFinished();
    auto exitCode = proc.exitCode();

    if (exitCode != 0)
        return { false, QObject::tr("V2Ray core failed with an exit code: ") + QString::number(exitCode) };

    const auto output = proc.readAllStandardOutput();

    if (SplitLines(output).isEmpty())
        return { false, QObject::tr("V2Ray core returns empty string.") };

    return { true, SplitLines(output).at(0) };
}
