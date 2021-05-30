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
