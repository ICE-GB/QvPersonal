#pragma once

#include "Common/CommonTypes.hpp"

#ifndef QV2RAY_NO_GRPC
#include "v2ray/app/stats/command/command.grpc.pb.h"

#include <grpc++/grpc++.h>
#endif

#include <QString>
#include <map>

// Check 10 times before telling user that API has failed.
constexpr auto QV2RAY_API_CALL_FAILEDCHECK_THRESHOLD = 30;

typedef std::map<QString, StatisticsObject::StatisticsType> QvAPITagProtocolConfig;

class APIWorker : public QObject
{
    Q_OBJECT
  public:
    APIWorker();
    ~APIWorker();
    void StartAPI(const QMap<QString, QString> &tagProtocolPair);
    void StopAPI();

  signals:
    void OnAPIDataReady(const StatisticsObject &data);
    void OnAPIErrored(const QString &err);

  private slots:
    void process();

  private:
    qint64 CallStatsAPIByName(const QString &name);
    QvAPITagProtocolConfig tagProtocolConfig;
    QThread *workThread;

    bool started = false;
    bool running = false;
#ifndef QV2RAY_NO_GRPC
    std::shared_ptr<::grpc::Channel> grpc_channel;
    std::unique_ptr<::v2ray::core::app::stats::command::StatsService::Stub> stats_service_stub;
#endif
};
