# DarkHelp - C++ helper class for Darknet's C API.
# Copyright 2019-2020 Stephane Charette <stephanecharette@gmail.com>
# MIT license applies.  See "license.txt" for details.
# $Id$


EXECUTE_PROCESS (
	COMMAND svnversion
	WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
	OUTPUT_VARIABLE DH_VER_SVN
	OUTPUT_STRIP_TRAILING_WHITESPACE )
SET ( DH_VER_MAJOR 1 )
SET ( DH_VER_MINOR 0 )
SET ( DH_VER_PATCH 0-${DH_VER_SVN} )
SET ( DH_VERSION ${DH_VER_MAJOR}.${DH_VER_MINOR}.${DH_VER_PATCH} )
MESSAGE ( "Building ver: ${DH_VERSION}" )

ADD_DEFINITIONS ( -DDH_VERSION="${DH_VERSION}" )
