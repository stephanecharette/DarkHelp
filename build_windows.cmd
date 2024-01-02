@echo off

if not exist build (
        echo You need to manually run through the build steps at least once before you use this script.
        echo Please see readme.md for details.
        exit /b 1
)

cd build

set ARCHITECTURE=x64
rem set BUILD_TYPE=Debug
set BUILD_TYPE=Release
set VCPKG_PATH=C:/src/vcpkg
rem set TRIPLET=x64-windows-static
set TRIPLET=x64-windows

cmake -A %ARCHITECTURE% -DCMAKE_BUILD_TYPE=%BUILD_TYPE% -DCMAKE_TOOLCHAIN_FILE=%VCPKG_PATH%/scripts/buildsystems/vcpkg.cmake -DVCPKG_TARGET_TRIPLET=%TRIPLET% -DDarknet="C:/Program Files/Darknet/lib/darknet.lib" ..
if %ERRORLEVEL% neq 0 goto END

msbuild.exe /property:Platform=%ARCHITECTURE%;Configuration=%BUILD_TYPE% /target:Build -maxCpuCount -verbosity:normal -detailedSummary DarkHelp.sln
if %ERRORLEVEL% neq 0 goto END

msbuild.exe /property:Platform=%ARCHITECTURE%;Configuration=%BUILD_TYPE% PACKAGE.vcxproj
if %ERRORLEVEL% neq 0 goto END

:END
cd ..
