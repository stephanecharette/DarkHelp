# DarkHelp - C++ helper class for Darknet's C API.
# Copyright 2019-2022 Stephane Charette <stephanecharette@gmail.com>
# MIT license applies.  See "license.txt" for details.


IF(WIN32)
	set(CMAKE_THREAD_LIBS_INIT "-lpthread")
	set(CMAKE_HAVE_THREADS_LIBRARY 1)
	set(CMAKE_USE_WIN32_THREADS_INIT 1)
	set(CMAKE_USE_PTHREADS_INIT 0)
	set(THREADS_PREFER_PTHREAD_FLAG ON)
ENDIF ()

FIND_PACKAGE ( Threads			REQUIRED	)
FIND_PACKAGE ( OpenCV	CONFIG	REQUIRED	)
INCLUDE_DIRECTORIES (${OpenCV_INCLUDE_DIRS}	)

IF (WIN32)
	# Assume that vcpkg was used on Windows
	FIND_PACKAGE (Darknet REQUIRED)
	INCLUDE_DIRECTORIES (${Darknet_INCLUDE_DIR})
	SET (Darknet Darknet::dark)
ELSE ()
	FIND_LIBRARY (Darknet darknet)
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
