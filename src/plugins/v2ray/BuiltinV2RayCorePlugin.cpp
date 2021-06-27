#include "BuiltinV2RayCorePlugin.hpp"

#include "Gui/QvGUIPluginInterface.hpp"
#include "core/V2RayKernel.hpp"
#include "ui/w_V2RayKernelSettings.hpp"

class GuiInterface : public Qv2rayPlugin::Gui::PluginGUIInterface
{

    // PluginGUIInterface interface
  public:
    virtual QIcon Icon() const override
    {
        return QIcon(QStringLiteral(":/qv2ray.png"));
    }
    virtual QList<PLUGIN_GUI_COMPONENT_TYPE> GetComponents() const override
    {
        return { GUI_COMPONENT_SETTINGS };
    }

  protected:
    virtual std::unique_ptr<Gui::PluginSettingsWidget> createSettingsWidgets() const override
    {
        return std::make_unique<V2RayKernelSettings>();
    }
    virtual QList<typed_plugin_editor> createInboundEditors() const override
    {
        return {};
    }
    virtual QList<typed_plugin_editor> createOutboundEditors() const override
    {
        return {};
    }
    virtual std::unique_ptr<Gui::PluginMainWindowWidget> createMainWindowWidget() const override
    {
        return nullptr;
    }
};

const QvPluginMetadata BuiltinV2RayCorePlugin::GetMetadata() const
{
    return { QStringLiteral("Qv2rayBase Builtin V2Ray Core Plugin"),                             //
             QStringLiteral("Qv2rayBase Development Team"),                                      //
             PluginId{ QStringLiteral("builtin_v2ray_support") },                                //
             QStringLiteral("Builtin Latency Test Engine. Provides basic V2Ray kernel support"), //
             QStringLiteral(""),                                                                 //
             { COMPONENT_KERNEL, COMPONENT_GUI } };
}

BuiltinV2RayCorePlugin::~BuiltinV2RayCorePlugin()
{
}

bool BuiltinV2RayCorePlugin::InitializePlugin()
{
    m_KernelInterface = std::make_shared<V2RayKernelInterface>();
    m_GUIInterface = new GuiInterface;
    return true;
}

void BuiltinV2RayCorePlugin::SettingsUpdated()
{
    settings.loadJson(m_Settings);
}
