find_package(Protobuf REQUIRED)

set(PROTO_SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}/3rdparty/v2ray-core/")
file(GLOB_RECURSE PROTO_FILES "${PROTO_SOURCE_DIR}/*.proto")

set(PROTO_GENERATED_DIR "${CMAKE_CURRENT_BINARY_DIR}/v2ray")
make_directory(${PROTO_GENERATED_DIR})

if(QV2RAY_AVOID_GRPC)
    add_compile_definitions(QV2RAY_NO_GRPC)
else()
    find_program(GRPC_CPP_PLUGIN grpc_cpp_plugin)
    find_package(gRPC QUIET)
    if(NOT gRPC_FOUND)
        find_package(PkgConfig REQUIRED)
        if(UNIX AND NOT APPLE)
            pkg_check_modules(GRPC REQUIRED grpc++ grpc)
            set(QV2RAY_BACKEND_LIBRARY ${GRPC_LIBRARIES})
        else()
            find_library(UPB_LIBRARY NAMES upb)
            find_library(ADDRESS_SORTING NAMES address_sorting)
            pkg_check_modules(GRPC REQUIRED grpc++ grpc gpr)
            set(QV2RAY_BACKEND_LIBRARY ${GRPC_LINK_LIBRARIES} ${UPB_LIBRARY} ${ADDRESS_SORTING})
        endif()
    endif()
endif()

foreach(proto ${PROTO_FILES})
    get_filename_component(PROTO_ABS_DIR_PATH ${proto} DIRECTORY)
    file(RELATIVE_PATH _r ${PROTO_SOURCE_DIR} ${proto})
    get_filename_component(PROTO_FILE_DIR ${_r} DIRECTORY)
    get_filename_component(PROTO_FILE_NAME ${_r} NAME_WE)

    set(PROTO_HEADER_FILE "${PROTO_GENERATED_DIR}/${PROTO_FILE_DIR}/${PROTO_FILE_NAME}.pb.h")
    set(PROTO_SOURCE_FILE "${PROTO_GENERATED_DIR}/${PROTO_FILE_DIR}/${PROTO_FILE_NAME}.pb.cc")

    if(PROTO_FILE_DIR MATCHES "/command$" AND NOT QV2RAY_AVOID_GRPC)
        message("Additional gRPC generation required: ${PROTO_FILE_DIR}/${PROTO_FILE_NAME}.proto")
        set(PROTO_GRPC_HEADER_FILE "${PROTO_GENERATED_DIR}/${PROTO_FILE_DIR}/${PROTO_FILE_NAME}.grpc.pb.h")
        set(PROTO_GRPC_SOURCE_FILE "${PROTO_GENERATED_DIR}/${PROTO_FILE_DIR}/${PROTO_FILE_NAME}.grpc.pb.cc")
        add_custom_command(
            OUTPUT "${PROTO_HEADER_FILE}" "${PROTO_SOURCE_FILE}" "${PROTO_GRPC_HEADER_FILE}" "${PROTO_GRPC_SOURCE_FILE}"
            COMMAND ${Protobuf_PROTOC_EXECUTABLE}
            ARGS
                --grpc_out "${PROTO_GENERATED_DIR}"
                --cpp_out "${PROTO_GENERATED_DIR}"
                -I "${PROTO_SOURCE_DIR}"
                --plugin=protoc-gen-grpc="${GRPC_CPP_PLUGIN}"
                "${proto}"
            DEPENDS "${proto}"
            )
    else()
        add_custom_command(
            COMMENT "Generate protobuf files for ${proto}"
            OUTPUT "${PROTO_HEADER_FILE}" "${PROTO_SOURCE_FILE}"
            COMMAND ${Protobuf_PROTOC_EXECUTABLE}
            ARGS
                --cpp_out "${PROTO_GENERATED_DIR}"
                -I "${PROTO_SOURCE_DIR}"
                "${proto}"
            DEPENDS "${proto}"
            )
    endif()

    list(APPEND PROTO_HEADERS ${PROTO_HEADER_FILE} ${PROTO_GRPC_HEADER_FILE})
    list(APPEND PROTO_SOURCES ${PROTO_SOURCE_FILE} ${PROTO_GRPC_SOURCE_FILE})
endforeach()

set_source_files_properties(FILES ${PROTO_HEADERS} ${PROTO_SOURCES} PROPERTIES SKIP_AUTOGEN TRUE)

add_library(QvPlugin-BuiltinV2RaySupport SHARED
    ${PROTO_SOURCES} ${PROTO_HEADERS}
    ${CMAKE_CURRENT_LIST_DIR}/common/SettingsModels.hpp
    ${CMAKE_CURRENT_LIST_DIR}/common/CommonHelpers.hpp
    ${CMAKE_CURRENT_LIST_DIR}/common/CommonHelpers.cpp
    ${CMAKE_CURRENT_LIST_DIR}/BuiltinV2RayCorePlugin.hpp
    ${CMAKE_CURRENT_LIST_DIR}/BuiltinV2RayCorePlugin.cpp
    ${CMAKE_CURRENT_LIST_DIR}/core/V2RayAPIStats.hpp
    ${CMAKE_CURRENT_LIST_DIR}/core/V2RayAPIStats.cpp
    ${CMAKE_CURRENT_LIST_DIR}/core/V2RayKernel.hpp
    ${CMAKE_CURRENT_LIST_DIR}/core/V2RayKernel.cpp
    ${CMAKE_CURRENT_LIST_DIR}/core/V2RayProfileGenerator.hpp
    ${CMAKE_CURRENT_LIST_DIR}/core/V2RayProfileGenerator.cpp
    )

target_compile_definitions(QvPlugin-BuiltinV2RaySupport PRIVATE)
target_include_directories(QvPlugin-BuiltinV2RaySupport PRIVATE ${CMAKE_CURRENT_LIST_DIR})
target_include_directories(QvPlugin-BuiltinV2RaySupport PRIVATE ${CMAKE_CURRENT_LIST_DIR}/../PluginsCommon)
target_include_directories(QvPlugin-BuiltinV2RaySupport PRIVATE ${PROTO_GENERATED_DIR})
target_link_libraries(QvPlugin-BuiltinV2RaySupport
    PRIVATE
    Qt::Core
    Qt::Network
    Qv2ray::QvPluginInterface
    protobuf::libprotobuf
    gRPC::grpc++)

qv2ray_add_plugin_moc_sources(QvPlugin-BuiltinV2RaySupport)
qv2ray_configure_plugin(QvPlugin-BuiltinV2RaySupport)
