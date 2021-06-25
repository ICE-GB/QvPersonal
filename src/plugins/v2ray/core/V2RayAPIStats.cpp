#include "V2RayAPIStats.hpp"

#include "QvPluginInterface.hpp"

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
const QvAPIDataTypeConfig DefaultInboundAPIConfig{ { StatisticsObject::API_ALL_INBOUND,
                                                     {
                                                         QStringLiteral("dokodemo-door"), //
                                                         QStringLiteral("http"),          //
                                                         QStringLiteral("socks")          //
                                                     } } };

const QvAPIDataTypeConfig DefaultOutboundAPIConfig{ { StatisticsObject::API_OUTBOUND_PROXY,
                                                      {
                                                          QStringLiteral("dns"),         //
                                                          QStringLiteral("http"),        //
                                                          QStringLiteral("shadowsocks"), //
                                                          QStringLiteral("socks"),       //
                                                          QStringLiteral("vmess"),       //
                                                          QStringLiteral("vless"),       //
                                                          QStringLiteral("trojan")       //
                                                      } },
                                                    { StatisticsObject::API_OUTBOUND_DIRECT, { QStringLiteral("freedom") } } };

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

void APIWorker::StartAPI(const QMap<bool, QMap<QString, QString>> &tagProtocolPair)
{
    // Config API
    tagProtocolConfig.clear();
    for (auto it = tagProtocolPair.constKeyValueBegin(); it != tagProtocolPair.constKeyValueEnd(); it++)
    {
        const auto config = it->first ? DefaultOutboundAPIConfig : DefaultInboundAPIConfig;
        for (const auto &[tag, protocol] : it->second.toStdMap())
        {
            for (const auto &[type, protocols] : config)
            {
                if (protocols.contains(protocol))
                    tagProtocolConfig[tag] = { protocol, type };
            }
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
                const QString channelAddress = QStringLiteral("127.0.0.1:1919810"); // + QSTRN(GlobalConfig.kernelConfig->statsPort);
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
            for (const auto &[tag, config] : tagProtocolConfig)
            {
                const auto prefix = config.type == StatisticsObject::API_ALL_INBOUND ? QStringLiteral("inbound") : QStringLiteral("outbound");
                const auto value_up = CallStatsAPIByName(prefix + QStringLiteral(">>>") + tag + QStringLiteral(">>>traffic>>>uplink"));
                const auto value_down = CallStatsAPIByName(prefix + QStringLiteral(">>>") + tag + QStringLiteral(">>>traffic>>>downlink"));
                hasError = hasError || value_up == Qv2ray_GRPC_ERROR_RETCODE || value_down == Qv2ray_GRPC_ERROR_RETCODE;
                statsResult[config.type].up += std::max(value_up, 0LL);
                statsResult[config.type].down += std::max(value_down, 0LL);
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
