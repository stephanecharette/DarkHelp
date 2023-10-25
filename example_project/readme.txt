This is an example of how to create a CMake-based C++ project that uses the
DarkHelp library.

The CMakeLists.txt file will pull in darknet, OpenCV, and DarkHelp.

The example.cpp file loads a neural network, sets several example options,
annotates a single image file, and displays the results.

To build:

	mkdir build
	cd build
	cmake ..
	make


To build using Visual Studio, below flag should be added in CmakeLists.txt;

ADD_COMPILE_OPTIONS ( /permissive- )

and below line should be removed, since it requires GNU GCC compiler;

ADD_DEFINITIONS ("-Wall -Wextra -Werror -Wno-unused-parameter")
