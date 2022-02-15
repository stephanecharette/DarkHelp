# DarkHelp - C++ helper class for Darknet's C API.
# Copyright 2019-2022 Stephane Charette <stephanecharette@gmail.com>
# MIT license applies.  See "license.txt" for details.


Most recent documentation for DarkHelp:  https://github.com/stephanecharette/DarkHelp/

@warning These instructions are no longer up-to-date.  Using vcpkg was causing too many problems since it doesn't build
OpenCV correctly.  (For example, vcpkg doesn't build highgui or the video plugins required to import video files.)  For
this reason, building with vcpkg is no longer supported.  See the build instructions on the DarkHelp project page instead.


# -------------------------
# SETTING UP PRE-REQUISITES
# -------------------------

Assuming Ubuntu or similar Debian-based Linux distribution, run the following command:

	sudo apt-get install build-essential tar curl zip unzip git cmake libmagic-dev libgtk2.0-dev pkg-config yasm


# ----------------
# CUDA/CUDNN & GPU
# ----------------

If you only want the "CPU" version of OpenCV and Darknet, then skip to the next section called "BUILDING OPENCV & DARKNET".

If you want to use your CUDA-supported GPU with Darknet and DarkHelp:

	- (These notes are from a while back.  The process may have changed.)
	- Visit https://developer.nvidia.com/cuda-downloads or https://developer.nvidia.com/cuda-toolkit
	- Click on Linux -> x86_64 -> Ubuntu -> 18.04, deb (network)
	- The instructions should be similar to this:
		wget https://developer.download.nvidia.com/compute/cuda/repos/ubuntu1804/x86_64/cuda-ubuntu1804.pin
		sudo mv cuda-ubuntu1804.pin /etc/apt/preferences.d/cuda-repository-pin-600
		sudo apt-key adv --fetch-keys https://developer.download.nvidia.com/compute/cuda/repos/ubuntu1804/x86_64/7fa2af80.pub
		sudo add-apt-repository "deb http://developer.download.nvidia.com/compute/cuda/repos/ubuntu1804/x86_64/ /"
		sudo apt-get -y install cuda
	- You'll need to add nsight-compute and cuda to the path.  For example, since I'm using fish as my shell, I run this:
		set --universal fish_user_paths /opt/nvidia/nsight-compute/2019.5.0 $fish_user_paths
		set --universal fish_user_paths /usr/local/cuda/bin $fish_user_paths
	- At the end of the next section, the installation command you run needs to be the one that includes CUDA/CUDNN support.


# -------------------------
# BUILDING OPENCV & DARKNET
# -------------------------

Run the following commands:

	mkdir ~/src
	cd ~/src
	git clone https://github.com/microsoft/vcpkg
	cd vcpkg
	./bootstrap-vcpkg.sh
	./vcpkg integrate install
	./vcpkg integrate bash

Edit the file ~/src/vcpkg/ports/opencv4/portfile.cmake and look for the this line:
	-DWITH_GTK=OFF

Change that line to this:
	-DWITH_GTK=ON

Save the portfile.cmake file, exit from the editor, and run *ONE* of the following two "install" commands:

	- This one is if you have a supported GPU and CUDA:
		./vcpkg install --triplet x64-linux tclap cuda cudnn opencv-cuda darknet[cuda,cudnn,opencv-cuda]

	- This one is if you only want support for CPU:
		./vcpkg install --triplet x64-linux tclap opencv darknet[opencv-base]

Note the install command will take some time to run since it has to download and build OpenCV, Darknet, and various dependencies.

Once it has finished, you can free up some disk space by deleting these two subdirectories:

	rm -rf buildtrees downloads


# -----------------
# BUILDING DARKHELP
# -----------------

Download DarkHelp from https://www.ccoderun.ca/
Extract the source into ~/src/DarkHelp/
Run the following commands:

	cd ~/src/DarkHelp/
	mkdir build
	cd build
	cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=~/src/vcpkg/scripts/buildsystems/vcpkg.cmake -DCMAKE_PREFIX_PATH=~/src/vcpkg/installed/x64-linux -DVCPKG_TARGET_TRIPLET=x64-linux ..
	make -j $(nproc)
	make package

This will build a static library named "libdarkhelp.a", and the "DarkHelp" command-line tool.

See build_linux.cmd which contains the steps from "BUILDING DARKHELP".
