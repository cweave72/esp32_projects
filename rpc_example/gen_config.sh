#!/bin/bash
SCRIPTPATH="$( cd -- "$(dirname "$0")" >/dev/null 2>&1 ; pwd -P )"

PROTO_INC="\
-i $ESP32_TOOLS/nanopb/generator/proto \
-i $SCRIPTPATH/main \
"

loglevel=debug
yaml_data=$SCRIPTPATH/config.yaml
proto_file=$SCRIPTPATH/main/Config.proto
libpath=$SCRIPTPATH/python/config/lib
pkgname=config
msgclass=Config
# Must use a absolute path that can be specified in nvs_config.csv
out_file=/tmp/espressif/build_tmp/config.bin

#echo "PROTO_INC = $PROTO_INC"
echo "SCRIPTPATH=$SCRIPTPATH"

(\
cd $SCRIPTPATH/python && . init_env.sh && \
api_gen \
    --loglevel=$loglevel \
    --libpath=$libpath \
    --pkgname=$pkgname \
    $PROTO_INC \
    write \
    --msgcls $msgclass \
    --yamlfile $yaml_data \
    --out $out_file \
    $proto_file
)
