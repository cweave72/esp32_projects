#!/bin/bash
SCRIPTPATH="$( cd -- "$(dirname "$0")" >/dev/null 2>&1 ; pwd -P )"
source $SCRIPTPATH/proto_files.sh

loglevel=debug
proto_file=$SCRIPTPATH/main/ProtoRpcApp.proto
libpath=$SCRIPTPATH/python/rpc/lib
pkgname=rpc

PROTO_INCS=""
for path in ${PROTO_PATHS[@]}; do
    PROTO_INCS+="-i $path "
done

#echo "PROTO_INCS = $PROTO_INCS"

(\
cd python && . init_env.sh && \
api_gen \
    --loglevel=$loglevel \
    --libpath=$libpath \
    --pkgname=$pkgname \
    $PROTO_INCS \
    build \
    $proto_file
)
