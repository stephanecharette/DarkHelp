# DarkHelp - C++ helper class for Darknet's C API.
# Copyright 2019-2022 Stephane Charette <stephanecharette@gmail.com>
# MIT license applies.  See "license.txt" for details.


Most recent documentation for DarkHelp:  https://github.com/stephanecharette/DarkHelp/


# -------------------------
# SETTING UP PRE-REQUISITES
# -------------------------

Install Visual Studio 2019 from https://visualstudio.microsoft.com/vs/
During installation, select "Desktop development with C++".

Download and install WinGet from https://github.com/microsoft/winget-cli/releases
Run the following commands:

	winget install git.git
	winget install kitware.cmake
	winget install nsis


# -------------------------
# BUILDING OPENCV & DARKNET
# -------------------------

Run the following commands:

	mkdir c:\src
	cd c:\src
	git clone https://github.com/microsoft/vcpkg
	cd vcpkg
	bootstrap-vcpkg.bat
	vcpkg.exe integrate install
	vcpkg.exe integrate powershell
	vcpkg.exe install opencv[contrib,core,dnn,ffmpeg,jpeg,png,quirc,tiff,webp]:x64-windows darknet[opencv-base]:x64-windows


# -----------------
# BUILDING DARKHELP
# -----------------

Assuming you already followed the steps above which included installing vcpkg, run the following commands:

	cd c:\src\vcpkg
	vcpkg.exe install tclap:x64-windows
	cd c:\src
	git clone https://github.com/stephanecharette/DarkHelp.git
	cd darkhelp
	mkdir build
	cd build
	cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=C:/src/vcpkg/scripts/buildsystems/vcpkg.cmake ..

You should now have a Visual Studio project file.  You can build using Visual Studio, or from the command line:

	msbuild.exe /property:Platform=x64;Configuration=Release /target:Build -maxCpuCount -verbosity:normal -detailedSummary DarkHelp.sln

This will build a static library named "darkhelp.lib", and the "DarkHelp.exe" example command-line tool.

Create the installation package:

	msbuild.exe /property:Platform=x64;Configuration=Release PACKAGE.vcxproj

Also see the script build_windows.cmd.
