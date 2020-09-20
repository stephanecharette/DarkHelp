# DarkHelp - C++ helper class for Darknet's C API.
# Copyright 2019-2020 Stephane Charette <stephanecharette@gmail.com>
# MIT license applies.  See "license.txt" for details.
# $Id$


# the library is C++11, but the CLI tool uses std::filesystem from C++17
SET ( CMAKE_CXX_STANDARD				17 )
SET ( CMAKE_CXX_STANDARD_REQUIRED		ON )

ADD_EXECUTABLE			( cli main.cpp )
SET_TARGET_PROPERTIES	( cli PROPERTIES OUTPUT_NAME "DarkHelp" )
TARGET_LINK_LIBRARIES	( cli PRIVATE Threads::Threads dh Darknet::dark ${OpenCV_LIBS} ${Magic} )

INSTALL ( TARGETS cli DESTINATION bin )
