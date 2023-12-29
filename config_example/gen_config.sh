#!/bin/bash
SCRIPTPATH="$( cd -- "$(dirname "$0")" >/dev/null 2>&1 ; pwd -P )"

TOOLS_PATH=$(readlink -f $SCRIPTPATH/../../esp32_tools)

PROTO_INC="\
-i $TOOLS_PATH/nanopb/generator/proto \
-i $SCRIPTPATH/main \
"

loglevel=info
yaml_data=$SCRIPTPATH/config.yaml
proto_file=$SCRIPTPATH/main/Config.proto
msgclass=Config
# Must use a absolute path that can be specified in nvs_config.csv
out_file=/tmp/espressif/build_tmp/config.bin

#echo "PROTO_INC = $PROTO_INC"

(\
cd $TOOLS_PATH && \
. ${TOOLS_PATH}/init_env.sh && \
config_gen \
    --loglevel=$loglevel \
    write \
    $PROTO_INC \
    --msgcls $msgclass \
    --yamlfile $yaml_data \
    --out $out_file \
    $proto_file
)
