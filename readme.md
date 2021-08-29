# What is DarkHelp?

DarkHelp is a C++ wrapper to make it easier to use the Darknet neural network framework within a C++ application.  DarkHelp performs the following:

- load a [Darknet](https://github.com/AlexeyAB/darknet)-style neural network (.cfg, .names, .weights)
- run inference on images -- either filenames or OpenCV `cv::Mat` images and video frames -- and return [a vector of results](https://www.ccoderun.ca/darkhelp/api/classDarkHelp.html#abcd5ee07c8d804ec37963cdb606511a9)
- optionally annotate images/frames with the inference results

Example annotated image after calling [`DarkHelp::predict()`](https://www.ccoderun.ca/darkhelp/api/classDarkHelp.html#a5c3e83dcf445d7c2590ae6f1672789b5) and [`DarkHelp::annotate()`](https://www.ccoderun.ca/darkhelp/api/classDarkHelp.html#a0777388373987711f6f955f95f7b413a):

![annotated image example](src-doc/shade_25pcnt.png)

DarkHelp also has [a very simple command-line tool](https://www.ccoderun.ca/darkhelp/api/Tool.html) that uses the DarkHelp API so some of the functionality can be accessed directly from the command-line.  This can be useful to run tests or for shell scripting.

# License

DarkHelp is open source and published using the MIT license.  Meaning you can use it in your commercial application.  See license.txt for details.

# How to Build DarkHelp (Linux)

Extremely simple easy-to-follow tutorial on how to build Darknet, DarkHelp, and [DarkMark](https://github.com/stephanecharette/DarkMark).

[![DarkHelp build tutorial](src-doc/darkmark_build_thumbnail.png)](https://www.youtube.com/watch?v=pJ2iyf_E9PM)

DarkHelp requires that [Darknet](https://github.com/AlexeyAB/darknet) has already been built and installed, since DarkHelp is a *wrapper* for the C functionality available in `libdarknet.so`.

## Building Darknet (Linux)

You must build Darknet with the `LIBSO=1` variable set to have it build `libdarknet.so`.  On Ubuntu:

	sudo apt-get install build-essential git libopencv-dev
	cd ~/src
	git clone https://github.com/AlexeyAB/darknet.git
	cd darknet
	# edit Makefile to set LIBSO=1, and possibly other flags
	make
	sudo cp libdarknet.so /usr/local/lib/
	sudo cp include/darknet.h /usr/local/include/
	sudo ldconfig

## Building DarkHelp (Linux)

Now that Darknet is built and installed, you can go ahead and build DarkHelp.  On Ubuntu:

	sudo apt-get install cmake build-essential libtclap-dev libmagic-dev libopencv-dev
	cd ~/src
	git clone https://github.com/stephanecharette/DarkHelp.git
	cd DarkHelp
	mkdir build
	cd build
	cmake -DCMAKE_BUILD_TYPE=Release ..
	make
	make package
	sudo dpkg -i darkhelp*.deb

## Building Darknet (Windows)

The Windows build uses `vcpkg` to install the necessary 3rd-party libraries such as OpenCV.  See the files `readme_windows.txt` and `build_windows.cmd` for details.

Start the "Developer Command Prompt for Visual Studio" (not Power Shell!) and run the following commands to build Darknet and OpenCV:

	cd c:\src
	git clone https://github.com/microsoft/vcpkg
	cd vcpkg
	bootstrap-vcpkg.bat
	vcpkg.exe integrate install
	vcpkg.exe integrate powershell
	vcpkg.exe install opencv[contrib,core,dnn,ffmpeg,jpeg,png,quirc,tiff,webp]:x64-windows darknet[opencv-base]:x64-windows

## Building DarkHelp (Windows)

Once you finish building Darknet and OpenCV, run the following commands in the "Developer Command Prompt for VS" to build DarkHelp:

	cd c:\src\vcpkg
	vcpkg.exe install tclap:x64-windows
	cd c:\src
	git clone https://github.com/stephanecharette/DarkHelp.git
	cd darkhelp
	mkdir build
	cd build
	cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=C:/src/vcpkg/scripts/buildsystems/vcpkg.cmake ..
	msbuild.exe /property:Platform=x64;Configuration=Release /target:Build -maxCpuCount -verbosity:normal -detailedSummary DarkHelp.sln

Make sure you update the path to the toolchain file if you used a different directory.

If you have [NSIS](https://nsis.sourceforge.io/) installed, then you can create an installation package with this command:

	msbuild.exe /property:Platform=x64;Configuration=Release PACKAGE.vcxproj

# Example Code

DarkHelp has many optional settings that impact the output, especially [`DarkHelp::annotate()`](https://www.ccoderun.ca/darkhelp/api/classDarkHelp.html#a0777388373987711f6f955f95f7b413a).

To keep it simple this example code doesn't change any settings.  It uses the default values as it runs inference on several images and saves the output:

    // include DarkHelp.hpp and link against libdarkhelp.so, libdarknet.so, and OpenCV
    //
    const auto samples_images = {"dog.jpg", "cat.jpg", "horse.jpg"};
    //
    // Only do this once.  You don't want to keep reloading the network inside
    // the loop because loading the network is actually a long process that takes
    // several seconds to run to finish.
    DarkHelp darkhelp("animals.cfg", "animals_best.weights", "animals.names");
    //
    for (const auto & filename : samples_images)
    {
        // get the predictions; on a decent GPU this should take milliseconds,
        // while on a CPU this might take a full second or more
        const auto results = darkhelp.predict(filename);
        //
        // display the results on the console
        // (meaning coordinates and confidence levels, not displaying the image)
        std::cout << results << std::endl;
        //
        // annotate the image and save the results
        cv::Mat output = darkhelp.annotate();
        cv::imwrite("output_" + filename, output, {CV_IMWRITE_PNG_COMPRESSION, 9});
    }

# C++ API Doxygen Output

The official DarkHelp documentation and web site is at <https://www.ccoderun.ca/darkhelp/>.

Some links to specific useful pages:

- [`DarkHelp` class details](https://www.ccoderun.ca/darkhelp/api/classDarkHelp.html#details)
- [`DarkHelp::DarkHelp()` constructor](https://www.ccoderun.ca/darkhelp/api/classDarkHelp.html#a38176b90d636340be98b1ce16c1d0c81)
- [`DarkHelp::predict(filename)`](https://www.ccoderun.ca/darkhelp/api/classDarkHelp.html#a422ca1f4279521b68cf7e59d182d7709)
- [`DarkHelp::predict(OpenCV Mat)`](https://www.ccoderun.ca/darkhelp/api/classDarkHelp.html#a422ca1f4279521b68cf7e59d182d7709)
- [`DarkHelp::annotate()`](https://www.ccoderun.ca/darkhelp/api/classDarkHelp.html#a0777388373987711f6f955f95f7b413a)
- [Image tiling](https://www.ccoderun.ca/darkhelp/api/Tiling.html)
- [`DarkHelp::predict_tile()`](https://www.ccoderun.ca/darkhelp/api/classDarkHelp.html#ae2afffea2f7c2fdd54ce47e22bc4e042)

![tiled image example](src-doc/mailboxes_2x2_tiles_detection.png)
