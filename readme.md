# What is the DarkHelp C++ API?

The DarkHelp C++ API is a wrapper to make it easier to use the Darknet neural network framework within a C++ application.  DarkHelp performs the following:

- load a [Darknet](https://github.com/hank-ai/darknet)-style neural network (.cfg, .names, .weights)
- run inference on images -- either filenames or OpenCV `cv::Mat` images and video frames -- and return [a vector of results](https://www.ccoderun.ca/DarkHelp/api/structDarkHelp_1_1PredictionResult.html#details)
- optionally annotate images/frames with the inference results

Example annotated image after calling [`DarkHelp::NN::predict()`](https://www.ccoderun.ca/DarkHelp/api/classDarkHelp_1_1NN.html#a827eaa61af42451f0796a4f0adb43013)
and [`DarkHelp::NN::annotate()`](https://www.ccoderun.ca/DarkHelp/api/classDarkHelp_1_1NN.html#a718c604a24ffb20efca54bbd73d79de5):

![annotated image example](src-doc/shade_25pcnt.png)

# What is the DarkHelp CLI?

DarkHelp also has [a very simple command-line tool](https://www.ccoderun.ca/darkhelp/api/Tool.html) that uses the DarkHelp C++ API so some of the functionality can be accessed directly from the command-line.  This can be useful to run tests or for shell scripting.

# What is the DarkHelp Server?

DarkHelp Server is a command-line tool that loads a neural network once, and then keeps running in the background.  It repeatedly applies the network to images or video frames and saves the results.

Unlike Darknet and the DarkHelp CLI which have to re-load the neural network every time they're called, DarkHelp Server only does this once.  DarkHelp Server [can be configured](https://www.ccoderun.ca/darkhelp/api/Server.html) to save the results in `.txt` format, `.json` format, annotate images, and can also crop the objects and create individual image files from each of the objects detected by the neural network.

# License

DarkHelp is open source and published using the MIT license.  Meaning you can use it in your commercial application.  See license.txt for details.

# How to Build DarkHelp (Linux)

Extremely simple easy-to-follow tutorial on how to build [Darknet](https://github.com/hank-ai/darknet#table-of-contents), DarkHelp, and [DarkMark](https://github.com/stephanecharette/DarkMark).

[![DarkHelp build tutorial](https://github.com/hank-ai/darknet/raw/master/doc/linux_build_thumbnail.jpg)](https://www.youtube.com/watch?v=WTT1s8JjLFk)

DarkHelp requires that [Darknet](https://github.com/hank-ai/darknet) has already been built and installed, since DarkHelp is a *wrapper* for the C functionality available in `libdarknet.so`.

## Building Darknet (Linux)

You must build Darknet first.  See the [Darknet repo](https://github.com/hank-ai/darknet#linux-cmake-method) for details.

## Building DarkHelp (Linux)

Now that Darknet is built and installed, you can go ahead and build DarkHelp.  On Ubuntu:

	sudo apt-get install build-essential libtclap-dev libmagic-dev libopencv-dev
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

You must build Darknet first.  See the [Darknet repo](https://github.com/hank-ai/darknet#windows-cmake-method) for details.

## Building DarkHelp (Windows)

Once you finish building Darknet, run the following commands in the "Developer Command Prompt for VS" to build DarkHelp:

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

DarkHelp has many optional settings that impact the output, especially [`DarkHelp::NN::annotate()`](https://www.ccoderun.ca/darkhelp/api/classDarkHelp_1_1NN.html#a718c604a24ffb20efca54bbd73d79de5).

To keep it simple this example code doesn't change any settings.  It uses the default values as it runs inference on several images and saves the output:

    // include DarkHelp.hpp and link against libdarkhelp, libdarknet, and OpenCV
    //
    const auto samples_images = {"dog.jpg", "cat.jpg", "horse.jpg"};
    //
    // Only do this once.  You don't want to keep reloading the network inside
    // the loop because loading the network is actually a long process that takes
    // several seconds to run to finish.
    DarkHelp::NN nn("animals.cfg", "animals_best.weights", "animals.names");
    //
    for (const auto & filename : samples_images)
    {
        // get the predictions; on a decent GPU this should take milliseconds,
        // while on a CPU this might take a full second or more
        const auto results = nn.predict(filename);
        //
        // display the results on the console
        // (meaning coordinates and confidence levels, not displaying the image)
        std::cout << results << std::endl;
        //
        // annotate the image and save the results
        cv::Mat output = nn.annotate();
        cv::imwrite("output_" + filename, output, {CV_IMWRITE_PNG_COMPRESSION, 9});
    }

# C++ API Doxygen Output

The official DarkHelp documentation and web site is at <https://www.ccoderun.ca/darkhelp/>.

Some links to specific useful pages:

- [`DarkHelp` namespace](https://www.ccoderun.ca/darkhelp/api/namespaceDarkHelp.html)
- [`DarkHelp::NN` class for "neural network"](https://www.ccoderun.ca/darkhelp/api/classDarkHelp_1_1NN.html#details)
- [`DarkHelp::Config` class for configuration items](https://www.ccoderun.ca/darkhelp/api/classDarkHelp_1_1Config.html#details)
- [Image tiling](https://www.ccoderun.ca/darkhelp/api/Tiling.html)
- [DarkHelp Server](https://www.ccoderun.ca/darkhelp/api/Server.html)

![tiled image example](src-doc/mailboxes_2x2_tiles_detection.png)
