#pragma once

#include "Gui/QvGUIPluginInterface.hpp"
#include "Plugin/PluginAPIHost.hpp"

namespace Qv2ray::ui::common
{
    class GuiPluginAPIHost
    {
      public:
        GuiPluginAPIHost(Qv2rayBase::Plugin::PluginAPIHost *host);
        QList<std::pair<Qv2rayPlugin::Qv2rayInterface *, Qv2rayPlugin::Gui::PluginGUIInterface *>> GUI_QueryByComponent(Qv2rayPlugin::PLUGIN_GUI_COMPONENT_TYPE c) const;

      private:
        Qv2rayBase::Plugin::PluginAPIHost *apiHost;
    };
} // namespace Qv2ray::ui::common

inline Qv2ray::ui::common::GuiPluginAPIHost *GUIPluginHost;
