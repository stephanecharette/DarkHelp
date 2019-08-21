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
	int first_image_idx	= -1;

	// the .names file is optional, so we need to see if we have one
	if (names.find("name") == std::string::npos)
	{
		// we probably don't have a names file, so the 3rd index must be the first image
		names = "";
		first_image_idx = 3;
	}
	else
	{
		// looks like we have a names file, so the image must be the 4th index
		first_image_idx = 4;
	}

	DarkHelp dark_help(config, weights, names);
	std::cout << "-> loading network took " << dark_help.duration_string() << std::endl;

	dark_help.names_include_percentage		= true;
	dark_help.annotation_include_duration	= true;
	dark_help.annotation_include_timestamp	= false;
	dark_help.annotation_font_scale			= 1.0;

	for (int idx = first_image_idx; idx < argc; idx ++)
	{
		const std::string filename = argv[idx];
		std::cout << "loading \"" << filename << "\"" << std::endl;
		cv::Mat mat;

		try
		{
			mat = cv::imread(filename);
		}
		catch (const std::exception & e)
		{
			std::cout << "Caught a C++ exception: " << e.what() << std::endl;
		}

		if (mat.empty())
		{
			std::cout << "Failed to read the image named \"" << filename << "\"" << std::endl;
			continue;
		}

		dark_help.predict(mat);
		std::cout << "-> prediction took " << dark_help.duration_string() << std::endl;
		dark_help.annotate();

		cv::Mat image_to_show = dark_help.annotated_image;
		if (image_to_show.cols > 768 || image_to_show.rows > 768)
		{
			cv::Mat resized;
			cv::resize(image_to_show, resized, cv::Size(768, 768));
			image_to_show = resized;
		}

		cv::imshow("prediction", image_to_show);
		cv::waitKey();
	}

	return 0;
}
