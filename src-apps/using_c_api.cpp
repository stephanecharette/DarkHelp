/* DarkHelp - C++ helper class for Darknet's C API.
 * Copyright 2019-2024 Stephane Charette <stephanecharette@gmail.com>
 * MIT license applies.  See "license.txt" for details.
 */


// This source file is written in C++ so we can use OpenCV easily,
// but the calls demonstrated are from the "C" DarkHelp API.
#include "DarkHelp_C_API.h"
#include <iostream>
#include <opencv2/opencv.hpp>


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

		const std::string fn1 = argv[1];
		const std::string fn2 = argv[2];
		const std::string fn3 = argv[3];
		const std::string image = argv[4];

		std::cout
			<< "Darknet v"	<< DarknetVersion()		<< std::endl
			<< "DarkHelp v"	<< DarkHelpVersion()	<< std::endl;

		DarkHelpPtr ptr = CreateDarkHelpNN(fn1.c_str(), fn2.c_str(), fn3.c_str());

		/* All of the DarkHelp settings have default values described in the documentation.  Calling these here
		 * just for demonstration.  You don't have to call all of these if the default value works for you.
		 */
		SetThreshold(ptr, 0.25);
		EnableNamesIncludePercentage(ptr, true);
		EnableAnnotationAutoHideLabels(ptr, false);
		SetAnnotationShadePredictions(ptr, 0.15);
		SetAnnotationFontScale(ptr, 0.5);
		SetAnnotationFontThickness(ptr, 1);
		SetAnnotationLineThickness(ptr, 1);
		EnableAnnotationIncludeDuration(ptr, false);
		EnableAnnotationIncludeTimestamp(ptr, false);
		EnableTiles(ptr, false);
		EnableSnapping(ptr, true);

		PredictFN(ptr, image.c_str());

		const char * json = GetPredictionResults(ptr);
		std::cout << "results=" << json << std::endl;

		Annotate(ptr, "testing.jpg");

		/* Once the DarkHelp object has been destroyed, make sure you don't make any further
		 * DarkHelp "C" calls other than CreateDarkHelpNN() to create a new instance.
		 */
		DestroyDarkHelpNN(ptr);

		// see what the output image looks like
		cv::Mat mat = cv::imread("testing.jpg");
		cv::imshow("annotated", mat);
		cv::waitKey();
	}
	catch (const std::exception & e)
	{
		std::cout << e.what() << std::endl;
		rc = 1;
	}

	return rc;
}
