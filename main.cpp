/* DarkHelp - C++ helper class for Darknet's C API.
 * Copyright 2019 Stephane Charette <stephanecharette@gmail.com>
 * MIT license applies.  See "license.txt" for details.
 * $Id$
 */

#include <DarkHelp.hpp>


int main(int argc, char *argv[])
{
	if (argc < 4)
	{
		std::cout << "Usage: " << argv[0] << " <config> <weights> [<names>] <image...>" << std::endl;
		return 1;
	}

	std::string config	= argv[1];
	std::string weights	= argv[2];
	std::string names	= argv[3];
	int image_idx		= -1;

	// the .names file is optional, so we need to see if we have one
	if (names.find("name") == std::string::npos)
	{
		// we probably don't have a names file, so the 3rd index must be the first image
		names = "";
		image_idx = 3;
	}
	else
	{
		// looks like we have a names file, so the image must be the 4th index
		image_idx = 4;
	}

	DarkHelp dark_help(config, weights, names);
	std::cout << "-> loading network took " << dark_help.duration_string() << std::endl;

	while (image_idx < argc)
	{
		const std::string filename = argv[image_idx];
		std::cout << "loading \"" << filename << "\"" << std::endl;
		cv::Mat mat = cv::imread(filename);
		if (mat.empty())
		{
			std::cout << "Failed to read the image named \"" << filename << "\"" << std::endl;
			image_idx ++;
			continue;
		}

		cv::Mat resized;
		cv::resize(mat, resized, cv::Size(512, 512));

		dark_help.predict(resized);
		std::cout << "-> prediction took " << dark_help.duration_string() << std::endl;
		dark_help.annotate();

		cv::imshow("prediction", dark_help.annotated_image);
		cv::waitKey();

		image_idx ++;
	}

	return 0;
}
