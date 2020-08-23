# DarkHelp - C++ helper class for Darknet's C API.
# Copyright 2019-2020 Stephane Charette <stephanecharette@gmail.com>
# MIT license applies.  See "license.txt" for details.
# $Id$


# Get the kernel name.  This should give us a string such as "Linux".
EXECUTE_PROCESS (
	COMMAND uname -s
	OUTPUT_VARIABLE DH_UNAME_S
	OUTPUT_STRIP_TRAILING_WHITESPACE )


# Get the machine hardware name.  This should give us a string such as "x86_64" or "aarch64" (Jetson Nano for example).
EXECUTE_PROCESS (
	COMMAND uname -m
	OUTPUT_VARIABLE DH_UNAME_M
	OUTPUT_STRIP_TRAILING_WHITESPACE )

# Get the name "Ubuntu".
EXECUTE_PROCESS (
	COMMAND lsb_release --id
	COMMAND cut -d\t -f2
	OUTPUT_VARIABLE DH_LSB_ID
	OUTPUT_STRIP_TRAILING_WHITESPACE )

# Get the version number "20.04".
EXECUTE_PROCESS (
	COMMAND lsb_release --release
	COMMAND cut -d\t -f2
	OUTPUT_VARIABLE DH_LSB_REL
	OUTPUT_STRIP_TRAILING_WHITESPACE )


SET ( CPACK_PACKAGE_VENDOR				"Stephane Charette"					)
SET ( CPACK_PACKAGE_CONTACT				"stephanecharette@gmail.com"		)
SET ( CPACK_PACKAGE_VERSION				${DH_VERSION}						)
SET ( CPACK_PACKAGE_VERSION_MAJOR		${DH_VER_MAJOR}						)
SET ( CPACK_PACKAGE_VERSION_MINOR		${DH_VER_MINOR}						)
SET ( CPACK_PACKAGE_VERSION_PATCH		${DH_VER_PATCH}						)
SET ( CPACK_RESOURCE_FILE_LICENSE		${CMAKE_CURRENT_SOURCE_DIR}/license.txt )
SET ( CPACK_PACKAGE_NAME				"darkhelp"							)
SET ( CPACK_PACKAGE_FILE_NAME			"darkhelp-${DH_VERSION}-${DH_UNAME_S}-${DH_UNAME_M}-${DH_LSB_ID}-${DH_LSB_REL}" )
SET ( CPACK_PACKAGE_DESCRIPTION_SUMMARY	"DarkHelp C++ library" )
SET ( CPACK_PACKAGE_DESCRIPTION			"DarkHelp C++ library" )
SET ( CPACK_PACKAGE_HOMEPAGE_URL		"https://www.ccoderun.ca/DarkHelp/"	)
SET ( CPACK_DEBIAN_PACKAGE_SECTION		"other"		)
SET ( CPACK_DEBIAN_PACKAGE_PRIORITY		"optional"	)
SET ( CPACK_DEBIAN_PACKAGE_MAINTAINER	"Stephane Charette <stephanecharette@gmail.com>" )
SET ( CPACK_GENERATOR					"DEB"		)

SET ( CPACK_SOURCE_IGNORE_FILES			".svn" ".kdev4" "build" )
SET ( CPACK_SOURCE_GENERATOR			"TGZ;ZIP" )

INCLUDE( CPack )
