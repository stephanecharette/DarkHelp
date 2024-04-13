/* DarkHelp - C++ helper class for Darknet's C API.
 * Copyright 2019-2024 Stephane Charette <stephanecharette@gmail.com>
 * MIT license applies.  See "license.txt" for details.
 */

#include "DarkHelp.hpp"

int main(int argc, char * argv[])
{
	int rc = 0;

	try
	{
		if (argc != 4)
		{
			std::cout
				<< "Usage:" << std::endl
				<< argv[0] << " <filename.dh> <key> <image.jpg>" << std::endl;
			throw std::invalid_argument("wrong number of arguments");
		}

		std::string dh = argv[1];
		std::string key = argv[2];
		std::string image = argv[3];

		// Load the neural network.  The .dh filename must have been created with the DarkHelpCombine CLI tool.
		DarkHelp::NN nn(false, dh, key);

		// Use OpenCV to load the image.
		cv::Mat original_image = cv::imread(image);

		// Call on the neural network to process the given filename
		nn.predict(original_image);

		// Annotate the image using the results.
		cv::Mat annotated_image = nn.annotate();

		// Display the results using OpenCV.
		cv::imshow("original", original_image);
		cv::imshow("annotated", annotated_image);
		cv::waitKey();
	}
	catch (const std::exception & e)
	{
		std::cout << e.what() << std::endl;
		rc = 1;
	}

	return rc;
}
