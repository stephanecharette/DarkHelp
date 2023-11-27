# DarkHelp - C++ helper class for Darknet's C API.
# Copyright 2019-2023 Stephane Charette <stephanecharette@gmail.com>
# MIT license applies.  See "license.txt" for details.


# the library is C++11, but the sample apps uses std::filesystem from C++17
SET ( CMAKE_CXX_STANDARD			17 )
SET ( CMAKE_CXX_STANDARD_REQUIRED	ON )


FILE ( GLOB SRC_APPS *.cpp	)
LIST ( SORT SRC_APPS		)


FOREACH ( filename IN LISTS SRC_APPS )
	CMAKE_PATH				( GET filename STEM stem )
	ADD_EXECUTABLE			( ${stem} ${filename} )
	TARGET_LINK_LIBRARIES	( ${stem} PRIVATE Threads::Threads dh ${Darknet} ${OpenCV_LIBS} )
ENDFOREACH ()
