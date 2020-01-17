# DarkHelp - C++ helper class for Darknet's C API.
# Copyright 2019-2020 Stephane Charette <stephanecharette@gmail.com>
# MIT license applies.  See "license.txt" for details.
# $Id$


# -------------------------------------------
# First we build the dynamic and static libs.
# -------------------------------------------

SET ( DH_LIB_NAME		darkhelp )
SET ( DH_LIB_STATIC		${DH_LIB_NAME} )
SET ( DH_LIB_SHARED		${DH_LIB_NAME}_shared )

# this creates the static library (.a)
ADD_LIBRARY				( ${DH_LIB_STATIC} STATIC DarkHelp.cpp )

# this creates the shared library (.so)
ADD_LIBRARY				( ${DH_LIB_SHARED} SHARED DarkHelp.cpp )
SET_TARGET_PROPERTIES	( ${DH_LIB_SHARED} PROPERTIES OUTPUT_NAME "${DH_LIB_NAME}" )
SET_TARGET_PROPERTIES	( ${DH_LIB_SHARED} PROPERTIES PREFIX "lib" )


# ---------------------------------------------------
# Next we install the files where we need them to be.
# ---------------------------------------------------

INSTALL ( FILES DarkHelp.hpp				DESTINATION include )
INSTALL ( TARGETS ${DH_LIB_STATIC}			DESTINATION lib )
INSTALL ( TARGETS ${DH_LIB_SHARED} LIBRARY	DESTINATION lib )
