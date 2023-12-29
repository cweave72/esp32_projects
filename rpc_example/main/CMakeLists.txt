set(srcs "rpc_example.c"
         "ProtoRpcApp.pb.c")

idf_component_register(
    SRCS ${srcs}
    INCLUDE_DIRS "."
    )

set(components_path $ENV{COMPONENTS_PATH})
set(nanopb_path $ENV{ESP32_TOOLS}/nanopb)
set(proto_file "${CMAKE_CURRENT_SOURCE_DIR}/ProtoRpcApp.proto")
set(proto_inc_paths
    ${nanopb_path}/generator/proto
    ${components_path}/ProtoRpc/src
    ${components_path}/TestRpc/src
    )

# Build the C-bindings for proto_file.
include(${nanopb_path}/nanopb.cmake)
nanopb_build(
    ${proto_file}
    PROTO_PATHS ${proto_inc_paths}
    )

set(rpc_data ${CMAKE_CURRENT_SOURCE_DIR}/../rpc_msg.yaml)
set(rpc_binary /tmp/espressif/build_tmp/rpc_msg.bin)

nvs_create_partition_image(
    nvs ../nvs_rpc.csv
    DEPENDS
        ${rpc_binary}
        )

set(rpc_generator ${CMAKE_CURRENT_SOURCE_DIR}/../gen_rpc_msg.sh)

add_custom_command(
    OUTPUT ${rpc_binary}
    COMMAND ${CMAKE_COMMAND} -E echo "[rpc test] Generating RPC msg binary ${rpc_binary}."
    COMMAND ${rpc_generator}
    VERBATIM
    DEPENDS ${proto_file} ${rpc_data}
    )
