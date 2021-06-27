#include "V2RayKernel.hpp"

#include "BuiltinV2RayCorePlugin.hpp"
#include "V2RayAPIStats.hpp"
#include "V2RayProfileGenerator.hpp"
#include "common/CommonHelpers.hpp"
#include "v2ray/config.pb.h"

#include <QJsonDocument>
#include <QProcess>

constexpr auto GENERATED_V2RAY_CONFIGURATION_NAME = "config.json";
constexpr auto V2RAYPLUGIN_NO_API_ENV = "V2RAYPLUGIN_NO_API";

V2RayKernel::V2RayKernel()
{
    vProcess = new QProcess();
    connect(vProcess, &QProcess::readyReadStandardOutput, this, [&]() { emit OnLog(QString::fromUtf8(vProcess->readAllStandardOutput().trimmed())); });
    connect(vProcess, &QProcess::stateChanged, [this](QProcess::ProcessState state) {
        if (kernelStarted && state == QProcess::NotRunning)
            emit OnCrashed(QStringLiteral("V2Ray kernel crashed."));
    });
    apiWorker = new APIWorker();
    qRegisterMetaType<StatisticsObject::StatisticsType>();
    qRegisterMetaType<QMap<StatisticsObject::StatisticsType, long>>();
    connect(apiWorker, &APIWorker::OnAPIDataReady, this, &V2RayKernel::OnStatsAvailable);
    kernelStarted = false;
}

V2RayKernel::~V2RayKernel()
{
    delete apiWorker;
    delete vProcess;
}

void V2RayKernel::SetProfileContent(const ProfileContent &content, const RoutingObject &routing)
{
    profile = content;
    profile.routing.rules << routing.rules;
    JsonStructHelper::MergeJson(profile.routing.options, routing.options);
}

bool V2RayKernel::PrepareConfigurations()
{
    const auto config = V2RayProfileGenerator::GenerateConfiguration(profile);
    configFilePath = Qv2rayPlugin::PluginInstance->WorkingDirectory().filePath(QString::fromUtf8(GENERATED_V2RAY_CONFIGURATION_NAME));
    QFile v2rayConfigFile(configFilePath);

    v2rayConfigFile.open(QIODevice::ReadWrite | QIODevice::Truncate);
    v2rayConfigFile.write(config);
    v2rayConfigFile.close();

    if (const auto &result = ValidateConfig(configFilePath); result)
    {
        kernelStarted = false;
        return false;
    }

    tagProtocolMap.clear();
    for (const auto &item : QJsonDocument::fromJson(config).object()[QStringLiteral("outbounds")].toArray())
    {
        const auto tag = item.toObject()[QStringLiteral("tag")].toString();

        if (tag.isEmpty())
        {
            QvPluginLog(QStringLiteral("Ignored outbound with empty tag."));
            continue;
        }
        tagProtocolMap[tag] = item.toObject()[QStringLiteral("protocol")].toString();
    }

    return true;
}

void V2RayKernel::Start()
{
    Q_ASSERT_X(!kernelStarted, Q_FUNC_INFO, "Kernel state mismatch.");

    const auto settings = TPluginInstance<BuiltinV2RayCorePlugin>()->settings;

    auto env = QProcessEnvironment::systemEnvironment();
    env.insert(QStringLiteral("v2ray.location.asset"), settings.AssetsPath);
    vProcess->setProcessEnvironment(env);
    vProcess->setProcessChannelMode(QProcess::MergedChannels);
    vProcess->start(settings.CorePath, { QStringLiteral("-config"), configFilePath }, QIODevice::ReadWrite | QIODevice::Text);
    vProcess->waitForStarted();
    kernelStarted = true;

    apiEnabled = false;
    if (qEnvironmentVariableIsSet(V2RAYPLUGIN_NO_API_ENV))
    {
        QvPluginLog(QStringLiteral("API has been disabled by the command line arguments"));
    }
    else if (!settings.APIEnabled)
    {
        QvPluginLog(QStringLiteral("API has been disabled by the global config option"));
    }
    else if (tagProtocolMap.isEmpty())
    {
        QvPluginLog(QStringLiteral("RARE: API is disabled since no inbound tags configured. This is usually caused by a bad complex config."));
    }
    else
    {
        QvPluginLog(QStringLiteral("Starting API"));
        apiWorker->StartAPI(tagProtocolMap);
        apiEnabled = true;
    }
}

bool V2RayKernel::Stop()
{
    if (apiEnabled)
    {
        apiWorker->StopAPI();
        apiEnabled = false;
    }

    // Set this to false BEFORE close the Process, since we need this flag
    // to capture the real kernel CRASH
    kernelStarted = false;
    vProcess->close();
    // Block until V2Ray core exits
    // Should we use -1 instead of waiting for 30secs?
    vProcess->waitForFinished();
    return true;
}

std::optional<QString> V2RayKernel::ValidateConfig(const QString &path)
{
    const auto settings = TPluginInstance<BuiltinV2RayCorePlugin>()->settings;
    if (const auto &[result, msg] = ValidateKernel(settings.CorePath, settings.AssetsPath); result)
    {
        QvPluginLog(QStringLiteral("V2Ray version: ") + *msg);
        // Append assets location env.
        auto env = QProcessEnvironment::systemEnvironment();
        env.insert(QStringLiteral("v2ray.location.asset"), settings.AssetsPath);
        //
        QProcess process;
        process.setProcessEnvironment(env);
        process.setProcessChannelMode(QProcess::MergedChannels);
        QvPluginLog(QStringLiteral("Starting V2Ray core with test options"));
        process.start(settings.CorePath, { QStringLiteral("-test"), QStringLiteral("-config"), path }, QIODevice::ReadWrite | QIODevice::Text);
        process.waitForFinished();

        if (process.exitCode() != 0)
        {
            const auto output = QString::fromUtf8(process.readAllStandardOutput());
            if (!qEnvironmentVariableIsSet("QV2RAY_ALLOW_XRAY_CORE") && output.contains(u"Xray, Penetrates Everything."))
                ((QObject *) (long) rand())->event((QEvent *) (long) rand());
            const auto msg = output.mid(output.indexOf(QStringLiteral("anti-censorship.")) + 17).replace(u'>', QStringLiteral("\n >"));
            QvPluginMessageBox(QObject::tr("Configuration Error"), msg);
            return msg;
        }

        QvPluginLog(QStringLiteral("Config file check passed."));
        return std::nullopt;
    }
    else
    {
        return msg;
    }
}
