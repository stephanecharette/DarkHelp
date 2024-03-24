# DarkHelp - C++ helper class for Darknet's C API.
# Copyright 2019-2024 Stephane Charette <stephanecharette@gmail.com>
# MIT license applies.  See "license.txt" for details.


IF ( WIN32 )
	ADD_COMPILE_DEFINITIONS ( _USE_MATH_DEFINES )
	ADD_COMPILE_OPTIONS ( /wd4244 ) # conversion between types
ENDIF ()


FILE ( GLOB SRC_APPS *.cpp	)
LIST ( SORT SRC_APPS		)


FOREACH ( filename IN LISTS SRC_APPS )
	CMAKE_PATH				( GET filename STEM stem )
	ADD_EXECUTABLE			( ${stem} ${filename} )
	TARGET_LINK_LIBRARIES	( ${stem} PRIVATE Threads::Threads dh ${Darknet} ${OpenCV_LIBS} )
ENDFOREACH ()
