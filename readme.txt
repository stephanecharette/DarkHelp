# DarkHelp - C++ helper class for Darknet's C API.
# Copyright 2019-2020 Stephane Charette <stephanecharette@gmail.com>
# MIT license applies.  See "license.txt" for details.
# $Id$


Most recent documentation for DarkHelp:  https://www.ccoderun.ca/DarkHelp/

Also see this post:  https://www.ccoderun.ca/programming/2019-08-25_Darknet_C_CPP/


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
