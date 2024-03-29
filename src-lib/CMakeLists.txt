# DarkHelp - C++ helper class for Darknet's C API.
# Copyright 2019-2024 Stephane Charette <stephanecharette@gmail.com>
# MIT license applies.  See "license.txt" for details.


SET ( CMAKE_POSITION_INDEPENDENT_CODE	ON )
SET ( CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS	ON )
SET ( BUILD_SHARED_LIBS					TRUE )

FILE ( GLOB SRC_LIB *.cpp	)
LIST ( SORT SRC_LIB			)

FILE ( GLOB HEADERS *.hpp *.h)
LIST ( SORT HEADERS			)

ADD_LIBRARY ( dh SHARED ${SRC_LIB} )
SET_TARGET_PROPERTIES ( dh PROPERTIES OUTPUT_NAME "darkhelp" )
TARGET_LINK_LIBRARIES ( dh PRIVATE Threads::Threads ${Darknet} ${OpenCV_LIBS} )

INSTALL ( FILES ${HEADERS}	DESTINATION include	)
INSTALL ( TARGETS dh		DESTINATION lib		)

IF (WIN32)
	# Windows assumes the DLLs are installed with the executables,
	# so put a copy of it here as well
	INSTALL (TARGETS dh DESTINATION bin)
ENDIF ()
