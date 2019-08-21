# DarkHelp - C++ helper class for Darknet's C API.
# Copyright 2019 Stephane Charette <stephanecharette@gmail.com>
# MIT license applies.  See "license.txt" for details.
# $Id$

PROJECT ( DarkHelp C CXX )

CMAKE_MINIMUM_REQUIRED ( VERSION 3.1 )

SET ( CMAKE_BUILD_TYPE Debug )
SET ( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wno-unused-parameter -Werror" )

FIND_PACKAGE ( Threads REQUIRED )
FIND_PACKAGE ( OpenCV REQUIRED )
FIND_LIBRARY ( Darknet darknet )

INCLUDE_DIRECTORIES ( ${OpenCV_INCLUDE_DIRS} )
INCLUDE_DIRECTORIES ( ${CMAKE_CURRENT_SOURCE_DIR} )

ADD_EXECUTABLE ( DarkHelp DarkHelp.cpp )
TARGET_LINK_LIBRARIES ( DarkHelp ${CMAKE_THREAD_LIBS_INIT} ${OpenCV_LIBS} ${Darknet} Threads::Threads )

