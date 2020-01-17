# DarkHelp - C++ helper class for Darknet's C API.
# Copyright 2019-2020 Stephane Charette <stephanecharette@gmail.com>
# MIT license applies.  See "license.txt" for details.
# $Id$


SET ( CPACK_PACKAGE_VENDOR				"Stephane Charette"					)
SET ( CPACK_PACKAGE_CONTACT				"stephanecharette@gmail.com"		)
SET ( CPACK_PACKAGE_VERSION				${DH_VERSION}						)
SET ( CPACK_PACKAGE_VERSION_MAJOR		${DH_VER_MAJOR}					)
SET ( CPACK_PACKAGE_VERSION_MINOR		${DH_VER_MINOR}					)
SET ( CPACK_PACKAGE_VERSION_PATCH		${DH_VER_PATCH}					)
SET ( CPACK_RESOURCE_FILE_LICENSE		${CMAKE_CURRENT_SOURCE_DIR}/license.txt )
SET ( CPACK_PACKAGE_NAME				"darkhelp" )
SET ( CPACK_PACKAGE_DESCRIPTION_SUMMARY	"DarkHelp C++ library" )
SET ( CPACK_PACKAGE_DESCRIPTION			"DarkHelp C++ library" )
SET ( CPACK_DEBIAN_PACKAGE_SECTION		"other"		)
SET ( CPACK_DEBIAN_PACKAGE_PRIORITY		"optional"	)
SET ( CPACK_DEBIAN_PACKAGE_MAINTAINER	"Stephane Charette <stephanecharette@gmail.com>" )
SET ( CPACK_GENERATOR					"DEB"		)

#SET ( CPACK_DEBIAN_PACKAGE_DEPENDS			"" )
#SET ( CPACK_DEBIAN_PACKAGE_CONTROL_EXTRA	"" )

SET ( CPACK_SOURCE_IGNORE_FILES			".svn" ".kdev4" "build" )
SET ( CPACK_SOURCE_GENERATOR			"TGZ;ZIP" )

INCLUDE( CPack )
