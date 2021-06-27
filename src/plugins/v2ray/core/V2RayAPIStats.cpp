#include "V2RayAPIStats.hpp"

#include "BuiltinV2RayCorePlugin.hpp"

#include <QStringBuilder>
#include <QThread>

#ifndef QV2RAY_NO_GRPC
using namespace v2ray::core::app::stats::command;
using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;
#endif

#define QV_MODULE_NAME "gRPCBackend"

constexpr auto Qv2ray_GRPC_ERROR_RETCODE = -1;
const std::map<StatisticsObject::StatisticsType, QStringList> DefaultOutboundAPIConfig //
    { { StatisticsObject::PROXY,
        {
            QStringLiteral("http"),        //
            QStringLiteral("shadowsocks"), //
            QStringLiteral("socks"),       //
            QStringLiteral("vmess"),       //
            QStringLiteral("vless"),       //
            QStringLiteral("trojan")       //
        } },
      { StatisticsObject::DIRECT,
        {
            QStringLiteral("freedom"), //
            QStringLiteral("dns"),     //
        } } };

APIWorker::APIWorker()
{
    workThread = new QThread();
    this->moveToThread(workThread);
    QvPluginLog(QStringLiteral("API Worker initialised."));
    connect(workThread, &QThread::started, this, &APIWorker::process);
    connect(workThread, &QThread::finished, [] { QvPluginLog(QStringLiteral("API thread stopped")); });
    started = true;
    workThread->start();
}

void APIWorker::StartAPI(const QMap<QString, QString> &tagProtocolPair)
{
    // Config API
    tagProtocolConfig.clear();
    for (auto it = tagProtocolPair.constKeyValueBegin(); it != tagProtocolPair.constKeyValueEnd(); it++)
    {
        const auto tag = it->first;
        const auto protocol = it->second;
        for (const auto &[type, protocols] : DefaultOutboundAPIConfig)
        {
            if (protocols.contains(protocol))
                tagProtocolConfig[tag] = type;
        }
    }

    running = true;
}

void APIWorker::StopAPI()
{
    running = false;
}

// --- DESTRUCTOR ---
APIWorker::~APIWorker()
{
    StopAPI();
    // Set started signal to false and wait for API thread to stop.
    started = false;
    workThread->wait();
    delete workThread;
}

// API Core Operations
// Start processing data.
void APIWorker::process()
{
    QvPluginLog(QStringLiteral("API Worker started."));
    while (started)
    {
        QThread::msleep(1000);
        bool dialed = false;
        int apiFailCounter = 0;

        while (running)
        {
            if (!dialed)
            {
#ifndef QV2RAY_NO_GRPC
                const QString channelAddress = QStringLiteral("127.0.0.1:") + QString::number(Qv2rayPlugin::TPluginInstance<BuiltinV2RayCorePlugin>()->settings.APIPort);
                QvPluginLog(QStringLiteral("gRPC Version: ") + QString::fromStdString(grpc::Version()));
                grpc_channel = grpc::CreateChannel(channelAddress.toStdString(), grpc::InsecureChannelCredentials());
                stats_service_stub = v2ray::core::app::stats::command::StatsService::NewStub(grpc_channel);
                dialed = true;
#endif
            }
            if (apiFailCounter == QV2RAY_API_CALL_FAILEDCHECK_THRESHOLD)
            {
                QvPluginLog(QStringLiteral("API call failure threshold reached, cancelling further API aclls."));
                emit OnAPIErrored(tr("Failed to get statistics data, please check if V2Ray is running properly"));
                apiFailCounter++;
                QThread::msleep(1000);
                continue;
            }
            else if (apiFailCounter > QV2RAY_API_CALL_FAILEDCHECK_THRESHOLD)
            {
                // Ignored future requests.
                QThread::msleep(1000);
                continue;
            }

            StatisticsObject statsResult;
            bool hasError = false;
            for (const auto &[tag, statType] : tagProtocolConfig)
            {
                const auto value_up = CallStatsAPIByName(QStringLiteral("outbound>>>") + tag + QStringLiteral(">>>traffic>>>uplink"));
                const auto value_down = CallStatsAPIByName(QStringLiteral("outbound>>>") + tag + QStringLiteral(">>>traffic>>>downlink"));
                hasError = hasError || value_up == Qv2ray_GRPC_ERROR_RETCODE || value_down == Qv2ray_GRPC_ERROR_RETCODE;
                if (statType == StatisticsObject::PROXY)
                {
                    statsResult.proxyUp += std::max(value_up, 0LL);
                    statsResult.proxyDown += std::max(value_down, 0LL);
                }
                if (statType == StatisticsObject::DIRECT)
                {
                    statsResult.directUp += std::max(value_up, 0LL);
                    statsResult.directDown += std::max(value_down, 0LL);
                }
            }
            apiFailCounter = hasError ? apiFailCounter + 1 : 0;
            // Changed: Removed isrunning check here
            emit OnAPIDataReady(statsResult);
            QThread::msleep(1000);
        } // end while running
    }     // end while started

    workThread->exit();
}

qint64 APIWorker::CallStatsAPIByName(const QString &name)
{
#ifndef QV2RAY_NO_GRPC
    ClientContext context;
    GetStatsRequest request;
    GetStatsResponse response;
    request.set_name(name.toStdString());
    request.set_reset(true);

    const auto status = stats_service_stub->GetStats(&context, request, &response);
    if (!status.ok())
    {
        QvPluginLog(QStringLiteral("API call returns:") + QString::number(status.error_code()) + QStringLiteral(":") + QString::fromStdString(status.error_message()));
        return Qv2ray_GRPC_ERROR_RETCODE;
    }
    else
    {
        return response.stat().value();
    }
#else
    return 0;
#endif
}
