# DarkHelp - C++ helper class for Darknet's C API.
# Copyright 2019-2022 Stephane Charette <stephanecharette@gmail.com>
# MIT license applies.  See "license.txt" for details.


IF (WIN32)
	ADD_COMPILE_OPTIONS ( /W4 )				# warning level (high)
	ADD_COMPILE_OPTIONS ( /WX )				# treat warnings as errors
	ADD_COMPILE_OPTIONS ( /permissive- )	# stick to C++ standards (turn off Microsoft-specific extensions)
	ADD_COMPILE_OPTIONS ( /wd4100 )			# disable "unreferenced formal parameter"
	ADD_COMPILE_DEFINITIONS ( _CRT_SECURE_NO_WARNINGS )	# don't complain about localtime()
	SET ( CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>" )
ELSE ()
	ADD_COMPILE_OPTIONS ( -Wall -Wextra -Werror -Wno-unused-parameter )
ENDIF ()
