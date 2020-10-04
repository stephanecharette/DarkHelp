@echo off

echo These build instructions are no longer supported.  See the project page
echo for details.  Once vcpkg can be made to work better with OpenCV, then
echo this script can be revived.
pause

echo This script assumes you've already read through and run the steps described
echo in readme_windows.txt!  The entire "build" directory will be deleted and
echo re-created.
pause

rmdir /q /s build
mkdir build
cd build

set ARCHITECTURE=x64
#set BUILD_TYPE=Release
set BUILD_TYPE=Debug
set VCPKG_PATH=C:\src\vcpkg
set TRIPLET=x64-windows-static

cmake -A %ARCHITECTURE% -DCMAKE_BUILD_TYPE=%BUILD_TYPE% -DCMAKE_TOOLCHAIN_FILE=%VCPKG_PATH%\scripts\buildsystems\vcpkg.cmake -DCMAKE_PREFIX_PATH=%VCPKG_PATH%\installed\%TRIPLET% -DVCPKG_TARGET_TRIPLET=%TRIPLET% ..
msbuild.exe /property:Platform=%ARCHITECTURE%;Configuration=%BUILD_TYPE% /target:Build -maxCpuCount -verbosity:normal -detailedSummary DarkHelp.sln
msbuild.exe /property:Platform=%ARCHITECTURE%;Configuration=%BUILD_TYPE% PACKAGE.vcxproj
