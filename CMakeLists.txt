cmake_minimum_required(VERSION 3.10.1)
project(qv2ray VERSION 3.0.0)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(BUILD_SHARED_LIBS ON)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

option(USE_MINGW "Use MinGW on Windows" OFF)

if(WIN32)
    include(cmake/versioninfo/generate_product_version.cmake)
    generate_product_version(
        QV2RAY_RC
        NAME               "Qv2ray"
        BUNDLE             "Qv2ray Project Family"
        ICON               "${CMAKE_SOURCE_DIR}/assets/icons/qv2ray.ico"
        VERSION_MAJOR      ${CMAKE_PROJECT_VERSION_MAJOR}
        VERSION_MINOR      ${CMAKE_PROJECT_VERSION_MINOR}
        VERSION_PATCH      ${CPACK_PACKAGE_VERSION_PATCH}
        VERSION_REVISION   ${QV2RAY_BUILD_VERSION}
        COMPANY_NAME       "Qv2ray Workgroup"
        COMPANY_COPYRIGHT  "Qv2ray Workgroup 2021"
        FILE_DESCRIPTION   "Qv2ray Application"
        )
    add_definitions(-DUNICODE -D_UNICODE -DNOMINMAX)
    set(GUI_TYPE WIN32)
    if(USE_MINGW)
        if(NOT DEFINED MinGW_ROOT)
            set(MinGW_ROOT "C:/msys64/mingw64")
        endif()
    else()
        add_compile_options("/utf-8")
        add_definitions(-D_WIN32_WINNT=0x600 -D_SCL_SECURE_NO_WARNINGS -D_CRT_SECURE_NO_WARNINGS)
    endif()
endif()

if(CMAKE_VERSION VERSION_GREATER_EQUAL "3.17.0")
    cmake_policy(SET CMP0100 NEW)
endif()


# ==================================================================================
# Qv2ray Build Info
# ==================================================================================
set(QV2RAY_CODENAME "Qv2ray 3.0, the new era.")

if(QV2RAY_BUILD_INFO)
    set(QV2RAY_BUILD_INFO "${QV2RAY_BUILD_INFO}")
elseif(DEFINED ENV{QV2RAY_BUILD_INFO})
    set(QV2RAY_BUILD_INFO "$ENV{QV2RAY_BUILD_INFO}")
else()
    set(QV2RAY_BUILD_INFO "Qv2ray (Made By Yourself)")
endif()

if(QV2RAY_BUILD_EXTRA_INFO)
    set(QV2RAY_BUILD_EXTRA_INFO "${QV2RAY_BUILD_EXTRA_INFO}")
elseif(DEFINED ENV{QV2RAY_BUILD_EXTRA_INFO})
    set(QV2RAY_BUILD_EXTRA_INFO "$ENV{QV2RAY_BUILD_EXTRA_INFO}")
else()
    set(QV2RAY_BUILD_EXTRA_INFO "${CMAKE_PROJECT_VERSION}")
endif()

add_definitions(-DQV2RAY_BUILD_INFO="${QV2RAY_BUILD_INFO}")
add_definitions(-DQV2RAY_BUILD_EXTRA_INFO="${QV2RAY_BUILD_EXTRA_INFO} - ${QV2RAY_CODENAME}")
message("Qv2ray Version: ${QV2RAY_BUILD_INFO} - ${QV2RAY_BUILD_EXTRA_INFO}")

# ==================================================================================
# Qv2ray compile arguments
# ==================================================================================

set(QV2RAY_DEFAULT_VASSETS_PATH "unset" CACHE STRING "Default V2Ray assets path")
set(QV2RAY_DEFAULT_VCORE_PATH "unset" CACHE STRING "Default V2Ray core path")

set(QV2RAY_QV2RAYBASE_PROVIDER        "module" CACHE STRING "Qv2rayBase Provider")
set(QV2RAY_SINGLEAPPLICATION_PROVIDER "module" CACHE STRING "SingleApplication Provider")
set(QV2RAY_QNODEEDITOR_PROVIDER       "module" CACHE STRING "QNodeEditor Provider")

option(QV2RAY_HAS_BUILTIN_THEMES  "Build with builtin themes" ON)

# ==================================================================================
# Default Core/Assets Path
# ==================================================================================
if(QV2RAY_DEFAULT_VCORE_PATH AND NOT QV2RAY_DEFAULT_VCORE_PATH STREQUAL "unset")
    add_definitions(-DQV2RAY_DEFAULT_VCORE_PATH="${QV2RAY_DEFAULT_VCORE_PATH}")
endif()
if(QV2RAY_DEFAULT_VASSETS_PATH AND NOT QV2RAY_DEFAULT_VASSETS_PATH STREQUAL "unset")
    add_definitions(-DQV2RAY_DEFAULT_VASSETS_PATH="${QV2RAY_DEFAULT_VASSETS_PATH}")
endif()

# ==================================================================================
# Qv2rayBase, Qt Libraries, Qv2ray GUI Libraries, Threads
# ==================================================================================
cmake_policy(SET CMP0071 NEW)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Threads REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(Qt6 COMPONENTS Core Widgets Svg Gui Network REQUIRED)

set(QAPPLICATION_CLASS "QApplication")
set(QT_DEFAULT_MAJOR_VERSION 6)

if(QV2RAY_QV2RAYBASE_PROVIDER STREQUAL "package")
    find_package(QvPluginInterface REQUIRED)
    find_package(Qv2rayBase REQUIRED)
else()
    add_subdirectory(src/Qv2rayBase)
endif()

if(QV2RAY_SINGLEAPPLICATION_PROVIDER STREQUAL "package")
    find_library(SINGLEAPPLICATION_LIBRARY NAMES SingleApplication)
    find_path(SINGLEAPPLICATION_DIR NAMES singleapplication.h PATH_SUFFIXES singleapplication)
else()
    add_subdirectory(3rdparty/SingleApplication)
endif()

if(QV2RAY_QNODEEDITOR_PROVIDER STREQUAL "package")
    find_package(NodeEditor REQUIRED)
else()
    add_subdirectory(3rdparty/QNodeEditor)
endif()

# ==================================================================================
# Qv2ray Builtin Plugins
# ==================================================================================
include(src/plugins/protocols/plugin.cmake)
include(src/plugins/subscription-adapters/plugin.cmake)
include(src/plugins/v2ray/plugin.cmake)

# ==================================================================================
# Qv2ray
# ==================================================================================
include(cmake/qrencode.cmake)
include(cmake/qv2ray-sources.cmake)

if (QV2RAY_HAS_BUILTIN_THEMES)
    include(3rdparty/uistyles/uistyles.cmake)
endif()

message(" ")
message("Qv2ray Version: ${CMAKE_PROJECT_VERSION}")
message("Qv2ray Build Version: ${QV2RAY_BUILD_VERSION}")
message("|-------------------------------------------------|")
message("| Qv2ray, A Cross Platform v2ray Qt GUI Client.   |")
message("| Licenced under GPLv3.                           |")
message("|                                                 |")
message("| You may only use this program to the extent     |")
message("| permitted by local law.                         |")
message("|                                                 |")
message("| See: https://www.gnu.org/licenses/gpl-3.0.html  |")
message("|-------------------------------------------------|")
message("| This is the original Qv2ray project by the      |")
message("| author who contributed over 95% code and other  |")
message("| contributors.                                   |")
message("|-------------------------------------------------|")
message("| Unlike Qv2ray/Qv2ray whose control has been     |")
message("| forcibly taken away from the author by another  |")
message("| organization owner.                             |")
message("|-------------------------------------------------|")
message(" ")

#return()

qt6_add_executable(qv2ray
    ${GUI_TYPE} ${QV2RAY_RC}
    ${QV2RAY_FULL_SOURCES}
    ${PROTO_SRCS} ${PROTO_HDRS}
    ${UISTYLE_QRCS}
    ${CMAKE_CURRENT_LIST_DIR}/assets/icons/resources.qrc)

if(BUILD_SHARED_LIBS)
    # This is a dirty hack, in the static lib build, these generated sources
    # would have been included by Qv2rayBase, however in a shared build, those
    # symbols are not exported, we need to have our own.
    qv2ray_add_plugin_moc_sources(qv2ray)
endif()

target_compile_definitions(qv2ray PRIVATE -DQV2RAY_VERSION_STRING="${CMAKE_PROJECT_VERSION}")
target_include_directories(qv2ray PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_include_directories(qv2ray PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/components)

target_link_libraries(qv2ray PRIVATE
    Qv2ray::QREncode
    Qv2ray::Qv2rayBase
    Qv2ray::QvPluginInterface
    NodeEditor::nodes
    OpenSSL::SSL
    OpenSSL::Crypto
    SingleApplication
    )

# ==================================================================================
# Platform-dependent installation process and deployment
# ==================================================================================
if(APPLE)
    include(cmake/platforms/macos.cmake)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    include(cmake/platforms/linux.cmake)
elseif(WIN32)
    include(cmake/platforms/windows.cmake)
endif()
