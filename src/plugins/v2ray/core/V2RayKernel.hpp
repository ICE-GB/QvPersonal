#pragma once

#include "Handlers/KernelHandler.hpp"

class QProcess;
class APIWorker;
class V2RayProfileGenerator;

const inline KernelId v2ray_kernel_id{ QStringLiteral("v2ray_kernel") };

class V2RayKernel : public Qv2rayPlugin::Kernel::PluginKernel
{
    Q_OBJECT
  public:
    V2RayKernel();
    ~V2RayKernel();

  public:
    virtual void SetConnectionSettings(const QMap<Qv2rayPlugin::Kernel::KernelOptionFlags, QVariant> &, const IOConnectionSettings &) override{};
    virtual void SetProfileContent(const ProfileContent &, const RoutingObject &) override;
    virtual bool PrepareConfigurations() override;
    virtual void Start() override;
    virtual bool Stop() override;
    virtual KernelId GetKernelId() const override
    {
        return v2ray_kernel_id;
    }

  signals:
    void OnCrashed(const QString &);
    void OnLog(const QString &);
    void OnStatsAvailable(StatisticsObject);

  private:
    std::optional<QString> ValidateConfig(const QString &path);

  private:
    ProfileContent profile;
    APIWorker *apiWorker;
    QProcess *vProcess;
    bool apiEnabled;
    bool kernelStarted = false;

    QString configFilePath;
    V2RayProfileGenerator *generator;
};

class V2RayKernelInterface : public Qv2rayPlugin::Kernel::IKernelHandler
{
  public:
    V2RayKernelInterface() : Qv2rayPlugin::Kernel::IKernelHandler(){};
    virtual QList<Qv2rayPlugin::Kernel::KernelFactory> PluginKernels() const override
    {
        Qv2rayPlugin::Kernel::KernelFactory v2ray;
        v2ray.Capabilities.setFlag(Qv2rayPlugin::Kernel::KERNELCAP_ROUTER);
        v2ray.Id = v2ray_kernel_id;
        v2ray.Name = QStringLiteral("V2Ray Core");
        v2ray.Create = std::function{ []() { return std::make_unique<V2RayKernel>(); } };
        v2ray.SupportedProtocols << QStringLiteral("blackhole")   //
                                 << QStringLiteral("dns")         //
                                 << QStringLiteral("freedom")     //
                                 << QStringLiteral("http")        //
                                 << QStringLiteral("loopback")    //
                                 << QStringLiteral("shadowsocks") //
                                 << QStringLiteral("socks")       //
                                 << QStringLiteral("trojan")      //
                                 << QStringLiteral("vless")       //
                                 << QStringLiteral("vmess");
        return { v2ray };
    }
};
