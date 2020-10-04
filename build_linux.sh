#!/bin/bash

echo This script assumes you''ve already read through and run the steps described in readme_linux.txt!
echo The entire "build" directory will be deleted and re-created.
read -rn1 -p "Press any key to continue..."

rm -rf build
mkdir build
cd build

#BUILD_TYPE=Release
BUILD_TYPE=Debug

# vcpkg is no longer used due to problems with how it builds OpenCV
#VCPKG_PATH=~/src/vcpkg
#TRIPLET=x64-linux
#cmake -DCMAKE_BUILD_TYPE=${BUILD_TYPE} -DCMAKE_TOOLCHAIN_FILE=${VCPKG_PATH}/scripts/buildsystems/vcpkg.cmake -DCMAKE_PREFIX_PATH=${VCPKG_PATH}/installed/${TRIPLET} -DVCPKG_TARGET_TRIPLET=${TRIPLET} ..

cmake -DCMAKE_BUILD_TYPE=${BUILD_TYPE} ..
make -j $(nproc)
make package

