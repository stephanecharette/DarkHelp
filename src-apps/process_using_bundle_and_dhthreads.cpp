/* DarkHelp - C++ helper class for Darknet's C API.
 * Copyright 2019-2024 Stephane Charette <stephanecharette@gmail.com>
 * MIT license applies.  See "license.txt" for details.
 */

#include "DarkHelp.hpp"
#include "DarkHelpThreads.hpp"


int main(int argc, char * argv[])
{
	int rc = 0;

	try
	{
		if (argc < 4)
		{
			std::cout
				<< "Usage:" << std::endl
				<< argv[0] << " <filename.dh> <key> <image.jpg> [<image2.jpg> ...]" << std::endl;
			throw std::invalid_argument("wrong number of arguments");
		}

		std::string dh							= argv[1];
		std::string key							= argv[2];
		const size_t number_of_threads_to_start	= 10;

		DarkHelp::DHThreads dht(dh, key, number_of_threads_to_start);

		// if you want to change some of the configuration settings, you'll need to do it *after* the network has been loaded
		for (size_t i = 0; i < number_of_threads_to_start; i++)
		{
			DarkHelp::NN * nn = dht.get_nn(i);
			if (nn)
			{
				nn->config.threshold		= 0.2f;
				nn->config.enable_tiles		= false;
				nn->config.snapping_enabled	= false;
			}
		}

		// add the images and immediately start processing them on different threads
		for (int i = 3; i < argc; i++)
		{
			dht.add_images(argv[i]);
		}

		const auto results = dht.wait_for_results();

		// display all of the filenames and the results for each one
		for (const auto & [key, val] : results)
		{
			std::cout << key << ": " << val << std::endl;
		}

	}
	catch (const std::exception & e)
	{
		std::cout << e.what() << std::endl;
		rc = 1;
	}

	return rc;
}
