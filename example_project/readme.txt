This is an example of how to create a CMake-based C++ project that uses the
DarkHelp library.

The CMakeLists.txt file will pull in darknet, OpenCV, and DarkHelp.

The example.cpp file loads a neural network, sets several example options,
annotates a single image file, and displays the results.

To build on Linux:

	mkdir build
	cd build
	cmake ..
	make

To build on Windows (you may need to adjust, similar commands as the ones you used to build Darknet and DarkHelp):

	mkdir build
	cd build
	cmake -DCMAKE_TOOLCHAIN_FILE=C:/src/vcpkg/scripts/buildsystems/vcpkg.cmake ..
	msbuild.exe ...etc... or use Visual Studio

IMPORTANT!  See the other source code examples in ../src-apps/.

