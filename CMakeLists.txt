# DarkHelp - C++ helper class for Darknet's C API.
# Copyright 2019-2023 Stephane Charette <stephanecharette@gmail.com>
# MIT license applies.  See "license.txt" for details.


# for anyone trying to build this in Ubuntu 18.04, if the minimum version is causing you problems,
# use the cmake that vcpkg downloads into vcpkg/downloads/tools/cmake-xxx/cmake-xxx/bin/cmake
CMAKE_MINIMUM_REQUIRED ( VERSION 3.10 )

PROJECT ( DarkHelp C CXX )

IF ( NOT CMAKE_BUILD_TYPE )
	SET ( CMAKE_BUILD_TYPE Release )
ENDIF ()

INCLUDE ( CM_version.cmake		)	# get the version number
INCLUDE ( CM_definitions.cmake	)	# compiler definitions
INCLUDE ( CM_dependencies.cmake	)	# find all required dependencies
INCLUDE ( CM_source.cmake		)	# source code that needs to be built
INCLUDE ( CM_package.cmake		)	# create .exe or .deb packages
