#!/bin/bash

export DETHRACE_ROOT_DIR=/home/panos/Desktop/Carmageddon/carmdemo

cd build
make
BUILD_RESULT=$?
cd ..
if [ $BUILD_RESULT -eq 0 ]; then
#    ./build/dethrace > output.log 2>&1
    ./build/dethrace
else
    echo "Build failed, not running game."
    exit 1
fi