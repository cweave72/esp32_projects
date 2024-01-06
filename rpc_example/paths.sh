# Define paths needed for helper scripts. Source from another script where
# SCRIPTPATH is defined.
TOOLS_PATH=$(readlink -f $SCRIPTPATH/../../esp32_tools)
COMPONENTS_PATH=$(readlink -f $SCRIPTPATH/../../esp32_components)
