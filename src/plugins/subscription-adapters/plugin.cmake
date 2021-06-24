set(SUBSCRIPTION_PLUGIN_TARGET QvPlugin-BuiltinSubscriptionSupport)

add_library(${SUBSCRIPTION_PLUGIN_TARGET} SHARED
    ${CMAKE_CURRENT_LIST_DIR}/resx.qrc
    ${CMAKE_CURRENT_LIST_DIR}/BuiltinSubscriptionAdapter.cpp
    ${CMAKE_CURRENT_LIST_DIR}/BuiltinSubscriptionAdapter.hpp
    ${CMAKE_CURRENT_LIST_DIR}/core/SubscriptionAdapter.cpp
    ${CMAKE_CURRENT_LIST_DIR}/core/SubscriptionAdapter.hpp
    )

target_include_directories(${SUBSCRIPTION_PLUGIN_TARGET} PRIVATE ${CMAKE_CURRENT_LIST_DIR})
target_include_directories(${SUBSCRIPTION_PLUGIN_TARGET} PRIVATE ${CMAKE_CURRENT_LIST_DIR}/../PluginsCommon)

qv2ray_configure_plugin(${SUBSCRIPTION_PLUGIN_TARGET} Widgets)
