#!/bin/bash

echo This script assumes you''ve already read through and run the steps described in readme_linux.txt!
echo The entire "build" directory will be deleted and re-created.
read -rn1 -p "Press any key to continue..."

rm -rf build
mkdir build
cd build

#BUILD_TYPE=Release
BUILD_TYPE=Debug
VCPKG_PATH=~/src/vcpkg
TRIPLET=x64-linux

cmake -DCMAKE_BUILD_TYPE=${BUILD_TYPE} -DCMAKE_TOOLCHAIN_FILE=${VCPKG_PATH}/scripts/buildsystems/vcpkg.cmake -DCMAKE_PREFIX_PATH=${VCPKG_PATH}/installed/${TRIPLET} -DVCPKG_TARGET_TRIPLET=${TRIPLET} ..
make -j $(nproc)

#msbuild.exe /property:Platform=%ARCHITECTURE%;Configuration=%BUILD_TYPE% /target:Build -maxCpuCount -verbosity:normal -detailedSummary DarkHelp.sln
#msbuild.exe /property:Platform=%ARCHITECTURE%;Configuration=%BUILD_TYPE% PACKAGE.vcxproj
