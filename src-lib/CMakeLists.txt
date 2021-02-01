# DarkHelp - C++ helper class for Darknet's C API.
# Copyright 2019-2021 Stephane Charette <stephanecharette@gmail.com>
# MIT license applies.  See "license.txt" for details.


# the library is C++11, but the CLI tool uses std::filesystem from C++17
SET ( CMAKE_CXX_STANDARD				11 )
SET ( CMAKE_CXX_STANDARD_REQUIRED		ON )
SET ( CMAKE_POSITION_INDEPENDENT_CODE	ON )
SET ( BUILD_SHARED_LIBS					FALSE )

ADD_LIBRARY ( dh STATIC DarkHelp.cpp )
SET_TARGET_PROPERTIES ( dh PROPERTIES OUTPUT_NAME "darkhelp" )

INSTALL ( FILES DarkHelp.hpp	DESTINATION include	)
INSTALL ( TARGETS dh			DESTINATION lib		)
