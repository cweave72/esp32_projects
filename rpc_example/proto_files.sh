SCRIPTPATH="$( cd -- "$(dirname "$0")" >/dev/null 2>&1 ; pwd -P )"
source paths.sh

#List paths to .proto files used in the project.
PROTO_PATHS=(
"$TOOLS_PATH/nanopb/generator/proto"
"$COMPONENTS_PATH/ProtoRpc/src"
"$COMPONENTS_PATH/TestRpc/src"
"$SCRIPTPATH/main"
)
