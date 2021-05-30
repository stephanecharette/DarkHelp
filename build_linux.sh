#!/bin/bash

echo This script assumes you''ve already read through and run the steps described in readme_linux.txt!
echo The entire "build" directory will be deleted and re-created.
read -rn1 -p "Press any key to continue..."

rm -rf build
mkdir build
cd build

BUILD_TYPE=Release
#BUILD_TYPE=Debug

cmake -DCMAKE_BUILD_TYPE=${BUILD_TYPE} ..
make -j $(nproc)
make package

