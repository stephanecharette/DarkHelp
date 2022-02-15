# DarkHelp - C++ helper class for Darknet's C API.
# Copyright 2019-2022 Stephane Charette <stephanecharette@gmail.com>
# MIT license applies.  See "license.txt" for details.


SET ( CPACK_PACKAGE_VENDOR				"Stephane Charette"					)
SET ( CPACK_PACKAGE_CONTACT				"stephanecharette@gmail.com"		)
SET ( CPACK_PACKAGE_VERSION				${DH_VERSION}						)
SET ( CPACK_PACKAGE_VERSION_MAJOR		${DH_VER_MAJOR}						)
SET ( CPACK_PACKAGE_VERSION_MINOR		${DH_VER_MINOR}						)
SET ( CPACK_PACKAGE_VERSION_PATCH		${DH_VER_PATCH}						)
SET ( CPACK_PACKAGE_INSTALL_DIRECTORY	"DarkHelp"							)
SET ( CPACK_RESOURCE_FILE_LICENSE		${CMAKE_CURRENT_SOURCE_DIR}/license.txt )
SET ( CPACK_PACKAGE_NAME				"darkhelp"							)
SET ( CPACK_PACKAGE_DESCRIPTION_SUMMARY	"DarkHelp C++ library" )
SET ( CPACK_PACKAGE_DESCRIPTION			"DarkHelp C++ library" )
SET ( CPACK_PACKAGE_HOMEPAGE_URL		"https://www.ccoderun.ca/DarkHelp/"	)

IF ( WIN32 )
	SET ( CPACK_PACKAGE_FILE_NAME						"darkhelp-${DH_VERSION}-Windows-64" )
	SET ( CPACK_NSIS_PACKAGE_NAME						"DarkHelp"						)
	SET ( CPACK_NSIS_DISPLAY_NAME						"DarkHelp v${DH_VERSION}"		)
	SET ( CPACK_NSIS_MUI_ICON							"${CMAKE_CURRENT_SOURCE_DIR}\\\\src-tool\\\\DarkHelp.ico" )
	SET ( CPACK_NSIS_MUI_UNIICON						"${CMAKE_CURRENT_SOURCE_DIR}\\\\src-tool\\\\DarkHelp.ico" )
	SET ( CPACK_PACKAGE_ICON							"${CMAKE_CURRENT_SOURCE_DIR}\\\\src-tool\\\\DarkHelp.ico" )
	SET ( CPACK_NSIS_CONTACT							"stephanecharette@gmail.com"	)
	SET ( CPACK_NSIS_URL_INFO_ABOUT						"https://www.ccoderun.ca/DarkHelp/" )
	SET ( CPACK_NSIS_HELP_LINK							"https://www.ccoderun.ca/DarkHelp/" )
	SET ( CPACK_NSIS_MODIFY_PATH						"ON"		)
	SET ( CPACK_NSIS_ENABLE_UNINSTALL_BEFORE_INSTALL	"ON"		)
	SET ( CPACK_NSIS_INSTALLED_ICON_NAME				"bin\\\\DarkHelp.ico" )
	SET ( CPACK_GENERATOR								"NSIS"		)
ELSE ()
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

	SET ( CPACK_PACKAGE_FILE_NAME			"darkhelp-${DH_VERSION}-${DH_UNAME_S}-${DH_UNAME_M}-${DH_LSB_ID}-${DH_LSB_REL}" )
	SET ( CPACK_DEBIAN_PACKAGE_SECTION		"other"		)
	SET ( CPACK_DEBIAN_PACKAGE_PRIORITY		"optional"	)
	SET ( CPACK_DEBIAN_PACKAGE_MAINTAINER	"Stephane Charette <stephanecharette@gmail.com>" )
	SET ( CPACK_GENERATOR					"DEB"		)
	SET ( CPACK_SOURCE_IGNORE_FILES			".svn" ".kdev4" "build/" "build_and_upload" )
	SET ( CPACK_SOURCE_GENERATOR			"TGZ;ZIP"	)
ENDIF ()

INCLUDE( CPack )
