set(QV2RAY_FULL_SOURCES "")

macro(qv2ray_add_class CLASS)
    list(APPEND QV2RAY_FULL_SOURCES
        ${CMAKE_SOURCE_DIR}/src/${CLASS}.hpp
        ${CMAKE_SOURCE_DIR}/src/${CLASS}.cpp)
endmacro()


macro(qv2ray_add_component COMP)
    qv2ray_add_class(components/${COMP}/${COMP})
endmacro()

macro(qv2ray_add_widget WIDGET)
    list(APPEND QV2RAY_FULL_SOURCES
        ${CMAKE_SOURCE_DIR}/src/ui/${WIDGET}.hpp
        ${CMAKE_SOURCE_DIR}/src/ui/${WIDGET}.cpp
        ${CMAKE_SOURCE_DIR}/src/ui/${WIDGET}.ui)
endmacro()

macro(qv2ray_add_window WINDOW)
    qv2ray_add_widget(windows/${WINDOW})
endmacro()

list(APPEND QV2RAY_FULL_SOURCES
    ${CMAKE_SOURCE_DIR}/src/main.cpp
    ${CMAKE_SOURCE_DIR}/src/models/SettingsModels.hpp
    ${CMAKE_SOURCE_DIR}/src/plugins/PluginsCommon/V2RayModels.hpp
    ${CMAKE_SOURCE_DIR}/src/ui/WidgetUIBase.hpp
    ${CMAKE_SOURCE_DIR}/src/ui/windows/w_MainWindow_extra.cpp
    ${CMAKE_SOURCE_DIR}/src/components/GeositeReader/picoproto.h
    ${CMAKE_SOURCE_DIR}/src/components/GeositeReader/picoproto.cc
    ${CMAKE_SOURCE_DIR}/src/plugins/internal/InternalPlugin.cpp
    ${CMAKE_SOURCE_DIR}/src/plugins/internal/InternalPlugin.hpp
    ${CMAKE_SOURCE_DIR}/src/plugins/internal/InternalProfilePreprocessor.cpp
    ${CMAKE_SOURCE_DIR}/src/plugins/internal/InternalProfilePreprocessor.hpp
    )

qv2ray_add_class(Qv2rayApplication)
qv2ray_add_class(ui/node/models/ChainOutboundNodeModel)
qv2ray_add_class(ui/node/models/InboundNodeModel)
qv2ray_add_class(ui/node/models/OutboundNodeModel)
qv2ray_add_class(ui/node/models/RuleNodeModel)
qv2ray_add_class(ui/node/NodeBase)
qv2ray_add_class(ui/node/NodeDispatcher)
qv2ray_add_class(ui/widgets/AutoCompleteTextEdit)


qv2ray_add_component(AutoLaunchHelper)
qv2ray_add_component(ConnectionModelHelper)
qv2ray_add_component(DarkmodeDetector)
qv2ray_add_component(GeositeReader)
qv2ray_add_component(GuiPluginHost)
qv2ray_add_component(LogHighlighter)
qv2ray_add_component(MessageBus)
qv2ray_add_component(PortDetector)
qv2ray_add_component(ProxyConfigurator)
qv2ray_add_component(QJsonModel)
qv2ray_add_component(QRCodeHelper)
qv2ray_add_component(RouteSchemeIO)
qv2ray_add_component(SpeedWidget)
qv2ray_add_component(StyleManager)

qv2ray_add_window(w_AboutWindow)
qv2ray_add_window(w_GroupManager)
qv2ray_add_window(w_ImportConfig)
qv2ray_add_window(w_MainWindow)
qv2ray_add_window(w_PluginManager)
qv2ray_add_window(w_PreferencesWindow)
qv2ray_add_window(w_ScreenShot_Core)

qv2ray_add_window(editors/w_InboundEditor)
qv2ray_add_window(editors/w_JsonEditor)
qv2ray_add_window(editors/w_OutboundEditor)
qv2ray_add_window(editors/w_RoutesEditor)

qv2ray_add_widget(widgets/complex/ChainEditorWidget)
qv2ray_add_widget(widgets/complex/RoutingEditorWidget)
qv2ray_add_widget(widgets/ConnectionInfoWidget)
qv2ray_add_widget(widgets/ConnectionItemWidget)
#qv2ray_add_widget(widgets/editors/CertificateItemWidget)
qv2ray_add_widget(widgets/editors/DnsSettingsWidget)
qv2ray_add_widget(widgets/editors/RouteSettingsMatrix)
qv2ray_add_widget(widgets/editors/StreamSettingsWidget)

qv2ray_add_widget(node/widgets/BalancerWidget)
qv2ray_add_widget(node/widgets/ChainOutboundWidget)
qv2ray_add_widget(node/widgets/ChainWidget)
qv2ray_add_widget(node/widgets/InboundOutboundWidget)
qv2ray_add_widget(node/widgets/RuleWidget)
