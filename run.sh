#!/bin/bash

export DETHRACE_ROOT_DIR=/home/panos/Desktop/Carmageddon/carmdemo

cd build
make
cd ..
if [ $? -eq 0 ]; then
    ./build/dethrace > output.log 2>&1
else
    echo "Build failed, not running game."
fi