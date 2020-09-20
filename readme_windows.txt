# DarkHelp - C++ helper class for Darknet's C API.
# Copyright 2019-2020 Stephane Charette <stephanecharette@gmail.com>
# MIT license applies.  See "license.txt" for details.
# $Id: readme.txt 2901 2020-01-17 17:46:22Z stephane $


Most recent documentation for DarkHelp:  https://www.ccoderun.ca/DarkHelp/


# -------------------------
# SETTING UP PRE-REQUISITES
# -------------------------

Install Visual Studio 2019 from https://visualstudio.microsoft.com/vs/
During installation, select "Desktop development with C++".
Download and install WinGet from https://github.com/microsoft/winget-cli/releases
Run the following commands:

	winget install git.git
	winget install kitware.cmake
	winget install nullsoft.nsis


# ----------------
# CUDA/CUDNN & GPU
# ----------------

If you only want the "CPU" version of OpenCV and Darknet, then skip to the next section called "BUILDING OPENCV & DARKNET".

If you want to use your CUDA-supported GPU with Darknet and DarkHelp:

	- Visit https://developer.nvidia.com/cuda-downloads
	- Click on Windows -> x86_64 -> 10 -> exe (network) -> Download
	- Accept the defaults and finish the CUDA install.
	- At the end of the next section, the installation command you run needs to be the one that includes CUDA/CUDNN support.


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

Run *ONE* of the following two "install" commands:

	- This one is if you have a supported GPU and CUDA:
		vcpkg.exe install --triplet x64-windows-static tclap cuda cudnn opencv-cuda darknet[cuda,cudnn,opencv-cuda]

	- This one is if you only want support for CPU:
		vcpkg.exe install --triplet x64-windows-static tclap opencv darknet[opencv-base]

Note the install command will take some time to run since it has to download and build OpenCV, Darknet, and various dependencies.

Once it has finished, you can free up some disk space by deleting the build directories with this command:

	rmdir /s /q buildtrees downloads


# -----------------
# BUILDING DARKHELP
# -----------------

Download DarkHelp from https://www.ccoderun.ca/
Extract the source into c:\src\DarkHelp\
Run the following commands:

	cd c:\src\DarkHelp\
	mkdir build
	cd build
	cmake -A x64 -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=C:\src\vcpkg\scripts\buildsystems\vcpkg.cmake -DCMAKE_PREFIX_PATH=c:\src\vcpkg\installed\x64-windows-static -DVCPKG_TARGET_TRIPLET=x64-windows-static ..
	msbuild.exe /property:Platform=x64;Configuration=Release /target:Build -maxCpuCount -verbosity:normal -detailedSummary DarkHelp.sln
	msbuild.exe /property:Platform=x64;Configuration=Release PACKAGE.vcxproj

This will build a static library named "DarkHelp\build\src-lib\Release\darkhelp.lib", and the DarkHelp.exe command-line tool.

See build_windows.cmd which contains the steps from "BUILDING DARKHELP".
