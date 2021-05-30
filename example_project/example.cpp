#include <DarkHelp.hpp>

int main()
{
	DarkHelp dh("example.cfg", "example_best.weights", "example.names");
	dh.enable_tiles					= true;
	dh.combine_tile_predictions		= true;
	dh.annotation_auto_hide_labels	= false;
	dh.annotation_include_duration	= true;
	dh.annotation_include_timestamp	= false;

	const auto results = dh.predict("example.jpg");

	std::cout << results << std::endl;

	cv::Mat img1 = dh.original_image;
	cv::Mat img2 = dh.annotate();

	const cv::Size size(1024, 768);
	cv::imshow("img1", resize_keeping_aspect_ratio(img1, size));
	cv::imshow("img2", resize_keeping_aspect_ratio(img2, size));
	cv::waitKey();

	return 0;
}

