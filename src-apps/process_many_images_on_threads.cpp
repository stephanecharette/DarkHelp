/* DarkHelp - C++ helper class for Darknet's C API.
 * Copyright 2019-2024 Stephane Charette <stephanecharette@gmail.com>
 * MIT license applies.  See "license.txt" for details.
 */

#include "DarkHelpThreads.hpp"


int main(int argc, char * argv[])
{
	int rc = 1;

	try
	{
		if (argc < 5)
		{
			std::cout
				<< "Usage:" << std::endl
				<< argv[0] << " <filename.cfg> <filename.names> <filename.weights> <image or subdirectory> [<more images or subdirectories...>]" << std::endl;
			throw std::invalid_argument("wrong number of arguments");
		}

		// these are just a few of the DarkHelp settings that can be set
		// for additional examples, see https://www.ccoderun.ca/darkhelp/api/classDarkHelp_1_1Config.html#details
		DarkHelp::Config cfg(argv[1], argv[2], argv[3]);
		cfg.threshold						= 0.2f;
		cfg.enable_tiles					= false;
		cfg.snapping_enabled				= false;
		cfg.annotation_auto_hide_labels		= false;
		cfg.annotation_include_duration		= false;
		cfg.annotation_include_timestamp	= false;
		cfg.annotation_pixelate_enabled		= false;
		cfg.annotation_line_thickness		= 1;

		const size_t number_of_threads_to_start = 10;

		DarkHelp::DHThreads dht(cfg, number_of_threads_to_start, "/tmp/output/");

#if 0
		// test adding a bunch of image filenames
		for (int i = 4; i < argc; i ++)
		{
			// call this as many times as necessary, with either a subdirectory name or a specific image filename
			dht.add_images(argv[i]);
		}
#else
		// test adding a bunch of *images* (not filenames)
		for (int i = 4 ; i < argc; i ++)
		{
			// hopefully for this example all of the these are image files and not directory names
			cv::Mat mat = cv::imread(argv[i]);
			dht.add_image(mat);
		}
#endif

		const auto results = dht.wait_for_results();

		// display all of the filenames and the results for each one
		for (const auto & [key, val] : results)
		{
			std::cout << key << ": " << val << std::endl;
		}

		rc = 0;
	}
	catch (const std::exception & e)
	{
		std::cout << "Exception: " << e.what() << std::endl;
	}

	return rc;
}
