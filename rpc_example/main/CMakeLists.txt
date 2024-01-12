set(srcs "rpc_example.c"
         "Config.pb.c"
         "ProtoRpcApp.pb.c"
         )

idf_component_register(
    SRCS ${srcs}
    INCLUDE_DIRS "."
    )

set(components_path $ENV{COMPONENTS_PATH})
set(nanopb_path $ENV{ESP32_TOOLS}/nanopb)

# Build app rpc C bindings.
set(rpc_proto "${CMAKE_CURRENT_SOURCE_DIR}/ProtoRpcApp.proto")
set(rpc_proto_inc
    ${nanopb_path}/generator/proto
    ${components_path}/ProtoRpc/src
    ${components_path}/TestRpc/src
    ${components_path}/RtosUtils/src
    )

include(${nanopb_path}/nanopb.cmake)
nanopb_build(
    ${rpc_proto}
    PROTO_PATHS ${rpc_proto_inc}
    )

# Build the C-bindings for config proto.
set(config_proto ${CMAKE_CURRENT_SOURCE_DIR}/Config.proto)
set(config_data ${CMAKE_CURRENT_SOURCE_DIR}/../config.yaml)
set(config_binary /tmp/espressif/build_tmp/config.bin)
set(config_proto_inc
    ${nanopb_path}/generator/proto
    )

include(${nanopb_path}/nanopb.cmake)
nanopb_build(
    ${config_proto}
    PROTO_PATHS ${config_proto_inc}
    )

# Create the NVS partition for config.
nvs_create_partition_image(
    nvs ../nvs_config.csv
    DEPENDS
        ${config_binary}
        )

# Build the config binary.
set(config_generator ${CMAKE_CURRENT_SOURCE_DIR}/../gen_config.sh)

add_custom_command(
    OUTPUT ${config_binary}
    COMMAND ${CMAKE_COMMAND} -E echo "[config] Generating config binary ${config_binary}."
    COMMAND ${config_generator}
    VERBATIM
    DEPENDS ${config_proto} ${config_data}
    )
