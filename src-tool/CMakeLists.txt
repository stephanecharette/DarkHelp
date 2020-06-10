# DarkHelp - C++ helper class for Darknet's C API.
# Copyright 2019-2020 Stephane Charette <stephanecharette@gmail.com>
# MIT license applies.  See "license.txt" for details.
# $Id$


ADD_EXECUTABLE ( DarkHelp main.cpp)
TARGET_LINK_LIBRARIES ( DarkHelp Threads::Threads darkhelp ${Darknet} ${OpenCV_LIBS} ${Magic} )
INSTALL ( TARGETS DarkHelp DESTINATION bin )
