SCRIPTPATH="$( cd -- "$(dirname "$0")" >/dev/null 2>&1 ; pwd -P )"

#List paths to .proto files used in the project.
PROTO_PATHS=(
"$ESP32_TOOLS/nanopb/generator/proto"
"$COMPONENTS_PATH/ProtoRpc/src"
"$COMPONENTS_PATH/TestRpc/src"
"$COMPONENTS_PATH/RtosUtils/src"
"$COMPONENTS_PATH/Lfs_Part/src"
"$SCRIPTPATH/main"
)
