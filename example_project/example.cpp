#include <DarkHelp.hpp>

int main()
{
	DarkHelp::Config cfg("example.cfg", "example_best.weights", "example.names");
	cfg.enable_tiles					= false;
	cfg.annotation_auto_hide_labels		= false;
	cfg.annotation_include_duration		= true;
	cfg.annotation_include_timestamp	= false;
	// lots of other options, scroll down this page to see what can be done:  https://www.ccoderun.ca/darkhelp/api/classDarkHelp_1_1Config.html#details

	DarkHelp::NN nn(cfg);

	// you can further modify the configuration even after the neural network has been created
	nn.config.annotation_line_thickness	= 1;
	nn.config.annotation_shade_predictions = 0.36;

	// apply the neural network to an image on disk
	const auto results = nn.predict("example.jpg");

	// print the neural network results on the console
	std::cout << results << std::endl;

	// display both the original image and the annotated image using OpenCV HighGUI
	cv::Mat img1 = nn.original_image;
	cv::Mat img2 = nn.annotate();

	const cv::Size size(1024, 768);
	cv::imshow("img1", DarkHelp::resize_keeping_aspect_ratio(img1, size));
	cv::imshow("img2", DarkHelp::resize_keeping_aspect_ratio(img2, size));
	cv::waitKey();

	// or, if you want to handle video files and webcams instead of static images, you'd do something like this
	cv::VideoCapture cap(0); // first webcam is index zero

	// possibly set the webcam resolution and FPS here (see OpenCV for details)
	// cap.set(cv::CAP_PROP_FPS, 25), etc...
	
	while (cap.isOpened())
	{
		cv::Mat frame;
		cap >> frame;
		if (frame.empty())
		{
			// video has ended (or webcam has been unplugged?)
			break;
		}

		const auto result = nn.predict(frame);

		// maybe loop through the results looking for something specific?
		for (const auto & prediction : results)
		{
			// the "PredictionResult" class is documented here:  https://www.ccoderun.ca/darkhelp/api/structDarkHelp_1_1PredictionResult.html#details
			
			if (prediction.best_class == 3) // classes are zero-based, so 3 would be the 4th object type in our .names file
			{
				// do something with the "rect" which has the coordinates of the object, or any of the other fields documented in the link above
				
				if (rect.width > 50 and rect.height > 50)
				{
					// do something here with objects of class 3 that are greater than this size
				}
			}
		}
	}

	return 0;
}

