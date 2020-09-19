# DarkHelp - C++ helper class for Darknet's C API.
# Copyright 2019-2020 Stephane Charette <stephanecharette@gmail.com>
# MIT license applies.  See "license.txt" for details.
# $Id: readme.txt 2901 2020-01-17 17:46:22Z stephane $


Most recent documentation for DarkHelp:  https://www.ccoderun.ca/DarkHelp/


# -------------------------
# SETTING UP PRE-REQUISITES
# -------------------------

Assuming Ubuntu or Debian based Linux distribution, run the following commands:

	sudo apt-get install build-essentials tar curl zip unzip git cmake


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

Run *ONE* of the following two "install" commands:

	- This one is if you have a supported GPU and CUDA:
		./vcpkg install --triplet x64-linux tclap cuda cudnn opencv-cuda darknet[cuda,cudnn,opencv-cuda]

	- This one is if you only want support for CPU:
		./vcpkg install --triplet x64-linux tclap opencv darknet[opencv-base]

Note the install command will take some time to run since it has to download and build OpenCV, Darknet, and various dependencies.

Once it has finished, you can free up some disk space by deleting the build directories with this command:

	rm -rf buildtrees downloads


# ------------------
# SETTING UP DARKNET
# ------------------

cd ~
git clone https://github.com/AlexeyAB/darknet.git
cd darknet

Edit the file "makefile" and at the top make certain that OPENCV=1 and LIBSO=1.

There may be other build settings you want to change.  See this post for details:  https://www.ccoderun.ca/programming/2019-08-18_Installing_and_building_Darknet/

Save the "makefile", then run the following commands:

make
sudo cp include/darknet.h /usr/local/include/
sudo cp libdarknet.so /usr/local/lib/
sudo ldconfig


# -------------------
# SETTING UP DARKHELP
# -------------------

Make sure you follow the instructions for setting up darknet first.

Download DarkHelp from https://www.ccoderun.ca/programming/
Extract the source into ~/src/DarkHelp/

cd ~/src/DarkHelp/
mkdir build
cd build
cmake ..
make
make package
sudo dpkg -i darkhelp*.deb

Example application that uses libdarkhelp.so can be found in src-tool, or at https://www.ccoderun.ca/programming/2019-08-25_Darknet_C_CPP/.
