#!/usr/bin/env bash

export CPP_BUILD_TYPE=${CPP_BUILD_TYPE:-Debug}
export CPP_BUILD_DIR=build/$(mycpp -b)/$CPP_BUILD_TYPE
export OSG_FILE_PATH="$OSG_FILE_PATH:$(pwd)/data"
export LD_LIBRARY_PATH="$(find "$(realpath "$CPP_BUILD_DIR/lib")" -name 'osgPlugins*'  )"
export OSG_THREADING=SingleThreaded

vim
