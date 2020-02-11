# DarkHelp - C++ helper class for Darknet's C API.
# Copyright 2019-2020 Stephane Charette <stephanecharette@gmail.com>
# MIT license applies.  See "license.txt" for details.
# $Id$


SET ( CMAKE_CXX_STANDARD 11 )
SET ( CMAKE_CXX_STANDARD_REQUIRED ON )
SET ( CMAKE_POSITION_INDEPENDENT_CODE ON )

ADD_COMPILE_OPTIONS ( -Wall -Wextra -Werror -Wno-unused-parameter )


# We *MUST* know how darknet was built.  CPU only?  GPU?  GPU+CUDNN?
# DarkHelp must define the same macros as darknet prior to including darknet.h.
IF ( NOT DARKNET_FLAG )
	MESSAGE ( FATAL_ERROR "You must specify DARKNET_FLAG when building DarkHelp.  The flag must indicate if libdarknet.so was built for CPU, GPU, or CUDNN." )
ELSEIF ( DARKNET_FLAG STREQUAL "CPU" )
	MESSAGE ( "IMPORTANT: flag is set indicating that darknet was built for CPU-only" )
ELSEIF ( DARKNET_FLAG STREQUAL "GPU" )
	MESSAGE ( "IMPORTANT: flag is set indicating that darknet was built with GPU enabled" )
	ADD_DEFINITIONS ( -DGPU=1 )
	ENABLE_LANGUAGE ( CUDA )
ELSEIF ( DARKNET_FLAG STREQUAL "CUDNN" )
	MESSAGE ( "IMPORTANT: flag is set indicating that darknet was built with both GPU and CUDNN enabled" )
	ADD_DEFINITIONS ( -DGPU=1 )
	ADD_DEFINITIONS ( -DCUDNN=1 )
	ENABLE_LANGUAGE ( CUDA )
ELSE ()
	MESSAGE ( FATAL_ERROR "DARKNET_FLAG must be set to CPU, GPU, or CUDNN.  Current value of '${DARKNET_FLAG}' is invalid." )
ENDIF ()
