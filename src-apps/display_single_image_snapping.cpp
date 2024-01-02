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

		// Load the neural network.  The order of the 3 files does not matter, DarkHelp should figure out which file is which.
		DarkHelp::NN nn(fn1, fn2, fn3);

		/* Turn on snapping.  This will try to grow and/or shrink the bounding boxes in cases where Darknet/YOLO doesn't
		 * provide perfect results.  This is also impacted by the black-and-white threshold values, and may not apply to
		 * all image types.
		 */
		nn.config.snapping_enabled = true;

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
