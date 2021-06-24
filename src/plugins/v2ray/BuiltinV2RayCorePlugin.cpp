#include "BuiltinV2RayCorePlugin.hpp"

#include "core/V2RayKernel.hpp"

const QvPluginMetadata BuiltinV2RayCorePlugin::GetMetadata() const
{
    return { QStringLiteral("Qv2rayBase Builtin V2Ray Core Plugin"),                             //
             QStringLiteral("Qv2rayBase Development Team"),                                      //
             PluginId{ QStringLiteral("qvplugin_builtin_v2ray_core") },                          //
             QStringLiteral("Builtin Latency Test Engine. Provides basic V2Ray kernel support"), //
             QStringLiteral(""),                                                                 //
             { COMPONENT_KERNEL } };
}

BuiltinV2RayCorePlugin::~BuiltinV2RayCorePlugin()
{
}

bool BuiltinV2RayCorePlugin::InitializePlugin()
{
    m_KernelInterface = std::make_shared<V2RayKernelInterface>();
    return true;
}

void BuiltinV2RayCorePlugin::SettingsUpdated()
{
    settings.loadJson(m_Settings);
}
