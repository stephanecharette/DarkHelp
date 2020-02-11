# DarkHelp - C++ helper class for Darknet's C API.
# Copyright 2019-2020 Stephane Charette <stephanecharette@gmail.com>
# MIT license applies.  See "license.txt" for details.
# $Id$


FIND_PACKAGE ( Threads REQUIRED )
FIND_PACKAGE ( OpenCV REQUIRED )
FIND_LIBRARY ( Darknet darknet )

IF (NOT DARKNET_FLAG STREQUAL "CPU" )
	# if we're not building for CPU, then we must need CUDA
	FIND_PACKAGE ( CUDA REQUIRED )
	INCLUDE_DIRECTORIES ( ${CUDA_INCLUDE_DIRS} )
ENDIF ()


INCLUDE_DIRECTORIES ( ${OpenCV_INCLUDE_DIRS} )
