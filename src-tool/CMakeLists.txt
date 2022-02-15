# DarkHelp - C++ helper class for Darknet's C API.
# Copyright 2019-2022 Stephane Charette <stephanecharette@gmail.com>
# MIT license applies.  See "license.txt" for details.


# the library is C++11, but the CLI tool uses std::filesystem from C++17
SET ( CMAKE_CXX_STANDARD				17 )
SET ( CMAKE_CXX_STANDARD_REQUIRED		ON )


ADD_EXECUTABLE			( cli DarkHelpCli.cpp DarkHelp.rc )
SET_TARGET_PROPERTIES	( cli PROPERTIES OUTPUT_NAME "DarkHelp" )
TARGET_LINK_LIBRARIES	( cli PRIVATE Threads::Threads dh ${Darknet} ${OpenCV_LIBS} ${Magic} ${StdCppFS} )
INSTALL					( TARGETS cli DESTINATION bin )


ADD_EXECUTABLE			( server DarkHelpServer.cpp DarkHelp.rc )
SET_TARGET_PROPERTIES	( server PROPERTIES OUTPUT_NAME "DarkHelpServer" )
TARGET_LINK_LIBRARIES	( server PRIVATE Threads::Threads dh ${Darknet} ${OpenCV_LIBS} ${Magic} ${StdCppFS} )
INSTALL					( TARGETS server DESTINATION bin )


IF ( WIN32 )
	INSTALL ( FILES DarkHelp.ico DESTINATION bin )
ENDIF ()
