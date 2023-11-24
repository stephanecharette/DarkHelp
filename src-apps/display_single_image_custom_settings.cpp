/* DarkHelp - C++ helper class for Darknet's C API.
 * Copyright 2019-2023 Stephane Charette <stephanecharette@gmail.com>
 * MIT license applies.  See "license.txt" for details.
 */

#include <DarkHelp.hpp>

int main(int argc, char * argv[])
{
	int rc = 0;

	try
	{
		if (argc != 5)
		{
			std::cout
				<< "Usage:" << std::endl
				<< argv[0] << " <filename.cfg> <filename.names> <filename.weights> <filename.jpg>" << std::endl;
			throw std::invalid_argument("wrong number of arguments");
		}

		std::string fn1 = argv[1];
		std::string fn2 = argv[2];
		std::string fn3 = argv[3];
		std::string image = argv[4];

		// You can customize settings before you load the network, or after you load the network.  Or both, like we do in this example.
		DarkHelp::Config config(fn1, fn2, fn3);
		config.annotation_auto_hide_labels = false;
		config.annotation_include_duration = true;
		config.annotation_include_timestamp = false;
		config.threshold = 0.25;

		// Load the neural network using the configuration object.
		DarkHelp::NN nn(config);

		/* At this point the "config" object above is no longer used.  The DarkHelp::NN object made a copy of it.  When you
		 * want to alter additional config settings after this point, you'll need to do so via that copy in nn.config.
		 */
		nn.config.enable_tiles = false;
		nn.config.annotation_line_thickness = 1;
		nn.config.annotation_font_scale = 0.75;

		// Call on the neural network to process the given filename
		nn.predict(image);

		// Display the annotated image.
		cv::imshow("annotated", nn.annotate());
		cv::waitKey();
	}
	catch (const std::exception & e)
	{
		std::cout << e.what() << std::endl;
		rc = 1;
	}

	return rc;
}
