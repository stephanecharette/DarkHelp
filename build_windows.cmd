@echo off

echo This script assumes you've already read through and run the steps described
echo in readme_windows.txt!  The entire "build" directory will be deleted and
echo re-created.
pause

rmdir /q /s build
mkdir build
cd build

set ARCHITECTURE=x64
rem set BUILD_TYPE=Debug
set BUILD_TYPE=Release
set VCPKG_PATH=C:\src\vcpkg
rem set TRIPLET=x64-windows-static
set TRIPLET=x64-windows

cmake -A %ARCHITECTURE% -DCMAKE_BUILD_TYPE=%BUILD_TYPE% -DCMAKE_TOOLCHAIN_FILE=%VCPKG_PATH%/scripts/buildsystems/vcpkg.cmake -DVCPKG_TARGET_TRIPLET=%TRIPLET% -DCMAKE_CUDA_COMPILER="%CUDA_PATH%" ..
if ERRORLEVEL 1 goto END

msbuild.exe /property:Platform=%ARCHITECTURE%;Configuration=%BUILD_TYPE% /target:Build -maxCpuCount -verbosity:normal -detailedSummary DarkHelp.sln
if ERRORLEVEL 1 goto END

msbuild.exe /property:Platform=%ARCHITECTURE%;Configuration=%BUILD_TYPE% PACKAGE.vcxproj
if ERRORLEVEL 1 goto END

:END
cd ..
