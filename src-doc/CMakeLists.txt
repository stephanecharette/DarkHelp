# DarkHelp - C++ helper class for Darknet's C API.
# Copyright 2019 Stephane Charette <stephanecharette@gmail.com>
# MIT license applies.  See "license.txt" for details.
# $Id$


FIND_PACKAGE ( Doxygen QUIET )

IF ( Doxygen_FOUND )

	CONFIGURE_FILE ( ${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile )

	ADD_CUSTOM_TARGET (
		doc
		${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
		WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
		COMMENT "Generating documentation with Doxygen" VERBATIM )

ENDIF ( Doxygen_FOUND )
