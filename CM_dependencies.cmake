# DarkHelp - C++ helper class for Darknet's C API.
# Copyright 2019-2024 Stephane Charette <stephanecharette@gmail.com>
# MIT license applies.  See "license.txt" for details.


IF (WIN32)
	SET (CMAKE_HAVE_THREADS_LIBRARY 1)
	SET (CMAKE_USE_WIN32_THREADS_INIT 1)
	SET (CMAKE_USE_PTHREADS_INIT 0)
	SET (THREADS_PREFER_PTHREAD_FLAG ON)
ENDIF ()

FIND_PACKAGE (Threads			REQUIRED	)
FIND_PACKAGE (OpenCV	CONFIG	REQUIRED	)
INCLUDE_DIRECTORIES (${OpenCV_INCLUDE_DIRS}	)

# On Linux, Darknet should be installed as /usr/lib/libdarknet.so.
# On Windows, it is user-defined but probably is C:/Program Files/Darknet/lib/darknet.lib.
# If that is not the case, be prepared to manually edit the CMake cache file.
FIND_LIBRARY (Darknet darknet)
IF (WIN32)
	GET_FILENAME_COMPONENT(DARKNET_PARENT_DIR "${Darknet}" DIRECTORY)
	INCLUDE_DIRECTORIES (${DARKNET_PARENT_DIR}/../include/)
ENDIF ()

SET (StdCppFS "")

IF (NOT WIN32)
	FIND_LIBRARY (Magic magic) # sudo apt-get install libmagic-dev

	# On older 18.04, we need to use "experimental/filesystem" instead of "filesystem"
	# and we need to pass in the -lstdc++fs flag when linking.  This seems to have no
	# impact even when using newer versions of g++ which technically doesn't need this
	# to link.  (Does this need to be fixed in a different manner?)
	SET ( StdCppFS stdc++fs	)
ENDIF ()

FIND_PATH (TCLAP_INCLUDE_DIRS "tclap/Arg.h") # sudo apt-get install libtclap-dev
INCLUDE_DIRECTORIES (${TCLAP_INCLUDE_DIRS})
