#!/bin/bash

cd build
make
cd ..
if [ $? -eq 0 ]; then
    ./build/dethrace
else
    echo "Build failed, not running game."
fi