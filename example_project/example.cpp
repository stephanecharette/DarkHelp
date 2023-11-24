#include <DarkHelp.hpp>

// -----------------------------------------------------------------------------
// Please also see the other source code examples in the ../src-apps/ directory.
// -----------------------------------------------------------------------------

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

	return 0;
}
