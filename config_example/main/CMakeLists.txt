set(srcs "config_example.c"
         "Config.pb.c")

idf_component_register(SRCS ${srcs}
                       INCLUDE_DIRS "."
                       )

set(config_proto ${CMAKE_CURRENT_SOURCE_DIR}/Config.proto)
set(config_data ${CMAKE_CURRENT_SOURCE_DIR}/../config.yaml)
set(config_binary /tmp/espressif/build_tmp/config.bin)

# Build the C-bindings for Config.proto
include($ENV{ESP32_TOOLS}/nanopb/nanopb.cmake)
nanopb_build(
    ${config_proto}
    PROTO_PATHS
        ${CMAKE_SOURCE_DIR}/esp32_tools/nanopb/generator/proto
    )

nvs_create_partition_image(
    nvs ../nvs_config.csv
    DEPENDS
        ${config_binary}
        )

set(config_generator ${CMAKE_CURRENT_SOURCE_DIR}/../gen_config.sh)

add_custom_command(
    OUTPUT ${config_binary}
    COMMAND ${CMAKE_COMMAND} -E echo "[config] Generating config binary ${config_binary}."
    COMMAND ${config_generator}
    VERBATIM
    DEPENDS ${config_proto} ${config_data}
    )
