# DarkHelp - C++ helper class for Darknet's C API.
# Copyright 2019-2024 Stephane Charette <stephanecharette@gmail.com>
# MIT license applies.  See "license.txt" for details.


# for anyone trying to build this in Ubuntu 18.04, if the minimum version
# is causing you problems, please use the cmake from vcpkg or snap
CMAKE_MINIMUM_REQUIRED ( VERSION 3.20 )

PROJECT ( DarkHelp C CXX )

IF ( NOT CMAKE_BUILD_TYPE )
	SET ( CMAKE_BUILD_TYPE Release )
ENDIF ()

SET ( CMAKE_CXX_STANDARD			17 )
SET ( CMAKE_CXX_STANDARD_REQUIRED	ON )


INCLUDE ( CM_version.cmake		)	# get the version number
INCLUDE ( CM_definitions.cmake	)	# compiler definitions
INCLUDE ( CM_dependencies.cmake	)	# find all required dependencies
INCLUDE ( CM_source.cmake		)	# source code that needs to be built
INCLUDE ( CM_package.cmake		)	# create .exe or .deb packages
