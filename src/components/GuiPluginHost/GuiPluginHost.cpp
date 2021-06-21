#include "GuiPluginHost.hpp"

#include "Plugin/PluginManagerCore.hpp"
#include "Qv2rayBaseLibrary.hpp"

namespace Qv2ray::ui::common
{
    GuiPluginAPIHost::GuiPluginAPIHost()
    {
        apiHost = QvBaselib->PluginAPIHost();
    }

    QList<std::pair<Qv2rayPlugin::Qv2rayInterface *, Qv2rayPlugin::Gui::PluginGUIInterface *>> GuiPluginAPIHost::GUI_QueryByComponent(
        Qv2rayPlugin::PLUGIN_GUI_COMPONENT_TYPE c) const
    {
        QList<std::pair<Qv2rayPlugin::Qv2rayInterface *, Qv2rayPlugin::Gui::PluginGUIInterface *>> guiInterfaces;
        for (const auto &plugin : QvBaselib->PluginManagerCore()->GetPlugins(Qv2rayPlugin::COMPONENT_GUI))
        {
            const auto guiInterface = plugin->pinterface->GetGUIInterface();
            if (guiInterface->GetComponents().contains(c))
                guiInterfaces << std::make_pair(plugin->pinterface, guiInterface);
        }
        return guiInterfaces;
    }
} // namespace Qv2ray::ui::common
