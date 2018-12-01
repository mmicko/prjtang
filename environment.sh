#!/usr/bin/env bash

# Set up PYTHONPATH and other needed environment variables
# This script will also source user_environment.sh where you can specify
# overrides if required for your system

if [ "$0" = "$_" ]; then
echo This script is intended to be invoked using \"source environment.sh\"
echo Calling it as a standalone script will have no effect.
exit 1
fi

SCRIPT_PATH=$(readlink -f "${BASH_SOURCE:-$0}")
SCRIPT_DIR=$(dirname "$SCRIPT_PATH")
PYTHONLIBS_DIR="${SCRIPT_DIR}/util:${SCRIPT_DIR}/util/common"
export PYTHONPATH="${PYTHONLIBS_DIR}:${PYTHONPATH}"