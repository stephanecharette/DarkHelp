# DarkHelp - C++ helper class for Darknet's C API.
# Copyright 2019-2024 Stephane Charette <stephanecharette@gmail.com>
# MIT license applies.  See "license.txt" for details.


ADD_EXECUTABLE			( cli DarkHelpCli.cpp DarkHelp.rc )
SET_TARGET_PROPERTIES	( cli PROPERTIES OUTPUT_NAME "DarkHelp" )
TARGET_LINK_LIBRARIES	( cli PRIVATE Threads::Threads dh ${Darknet} ${OpenCV_LIBS} ${Magic} ${StdCppFS} )


ADD_EXECUTABLE			( server DarkHelpServer.cpp DarkHelp.rc )
SET_TARGET_PROPERTIES	( server PROPERTIES OUTPUT_NAME "DarkHelpServer" )
TARGET_LINK_LIBRARIES	( server PRIVATE Threads::Threads dh ${Darknet} ${OpenCV_LIBS} ${Magic} ${StdCppFS} )


ADD_EXECUTABLE			( combine DarkHelpCombine.cpp DarkHelp.rc )
SET_TARGET_PROPERTIES	( combine PROPERTIES OUTPUT_NAME "DarkHelpCombine" )
TARGET_LINK_LIBRARIES	( combine PRIVATE Threads::Threads dh ${Darknet} ${OpenCV_LIBS} ${StdCppFS} )


IF (UNIX)
	# non-Windows installation is very simple
	INSTALL (TARGETS cli server combine DESTINATION bin)
ELSE ()
	INSTALL ( FILES DarkHelp.ico DESTINATION bin )
	# more complicated install for Windows so we also get the .DLL files copied over from vcpkg
	# https://stackoverflow.com/a/72088102/13022
	# note you may also need to manually copy over darknet.dll!
	INSTALL (TARGETS cli server combine DESTINATION bin
		RUNTIME_DEPENDENCIES
			PRE_EXCLUDE_REGEXES "api-ms-" "ext-ms-" "wpaxholder" "HvsiFileTrust" "PdmUtilities"
			POST_EXCLUDE_REGEXES ".*system32/.*\\.dll"
			DIRECTORIES ${CMAKE_BINARY_DIR}/bin ${CMAKE_BINARY_DIR}/src-lib/Release/ ${CMAKE_BINARY_DIR}/src-lib/Debug/
		RUNTIME DESTINATION bin
		)
ENDIF ()
