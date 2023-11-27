/* DarkHelp - C++ helper class for Darknet's C API.
 * Copyright 2019-2023 Stephane Charette <stephanecharette@gmail.com>
 * MIT license applies.  See "license.txt" for details.
 */

#include <DarkHelp.hpp>

// see this blog post for details on this example code:  https://www.ccoderun.ca/programming/2023-11-26_YOLO_and_image_rotation/

int main(int argc, char * argv[])
{
	int rc = 0;

	try
	{
		if (argc < 5)
		{
			std::cout
				<< "Usage:" << std::endl
				<< argv[0] << " <filename.cfg> <filename.names> <filename.weights> <filename.jpg> [<more images...>]" << std::endl;
			throw std::invalid_argument("wrong number of arguments");
		}

		const cv::Scalar light_blue	(255	, 128	, 64	);
		const cv::Scalar red		(0		, 0		, 255	);
		const cv::Scalar yellow		(0		, 255	, 255	);
		const cv::Scalar white		(255	, 255	, 255	);
		const cv::Scalar black		(0		, 0		, 0		);

		// Load the neural network.  The order of the 3 files does not matter, DarkHelp should figure out which file is which.
		DarkHelp::NN nn(argv[1], argv[2], argv[3]);
		nn.config.annotation_auto_hide_labels	= false;
		nn.config.annotation_include_duration	= false;
		nn.config.annotation_colours[0]			= light_blue;
		nn.config.annotation_colours[1]			= yellow;

		/* If the difference between the two landmarks is less than this value then we don't bother torotate the image.
		 * We'll consider it "level".  Use a value that makes sense for your project.  This will likely depend on the
		 * size of the images as well as the type of objects you are detecting.
		 */
		const int tolerance_difference_in_pixels = 5;

		// Loop through all the images provided as argv[4]...argv[n].
		for (int idx = 4; idx < argc; idx ++)
		{
			std::string image = argv[idx];
			std::cout << image << ": processing..." << std::endl;

			cv::Mat original_image = cv::imread(image);

			// Start by displaying some blank images.  This is purely cosmetic and can be deleted since it has nothing to do with the image rotation.
			if (true)
			{
				cv::Mat blank(original_image.size(), CV_8UC3, white);
				cv::imshow("markup"						, blank);
				cv::imshow("annotated (pre-rotation)"	, blank);
				cv::imshow("annotated (post-rotation)"	, blank);
			}

			// Call on the neural network to process the image.
			auto results = nn.predict(original_image);

			// this block is for display/debug purpose only and can be deleted
			if (true)
			{
				cv::Mat annotated_pre = nn.annotate();
				cv::imshow("annotated (pre-rotation)", annotated_pre);
			}

			cv::Point left_landmark;
			cv::Point right_landmark;
			for (const auto & pred : results)
			{
				// With the film perforation network, class #0 is the perforations and class #1 are the frames.
				// Remember to set this appropriately for you own network!
				if (pred.best_class == 0)
				{
					// remember the landmarks -- doesn't matter which is left or right, we'll swap them if necessary once we have two to compare
					if (left_landmark == cv::Point(0, 0))
					{
						left_landmark = pred.rect.tl();
					}
					else
					{
						right_landmark = pred.rect.tl();
						if (left_landmark.x > right_landmark.x)
						{
							std::swap(left_landmark, right_landmark);
						}
						break;
					}
				}
			}

			if (left_landmark	== cv::Point(0, 0) or	// failed to find any landmark
				right_landmark	== cv::Point(0, 0) or	// failed to find a 2nd landmark
				std::abs(left_landmark.y - right_landmark.y) < tolerance_difference_in_pixels)
			{
				std::cout << image << ": no rotation to apply: left=" << left_landmark << " right=" << right_landmark << std::endl;
			}
			else
			{
				// figure out the angle between the left and right landmarks
				const float delta_x = right_landmark.x - left_landmark.x;
				const float delta_y = left_landmark.y - right_landmark.y;	// reverse left and right since Y axis grows down
				const float radians = std::atan2(delta_y, delta_x);
				const float degrees = radians * 180.0f / M_PI;

				// this next block of code is purely cosmetic for debug/display purposes and can be deleted
				if (true)
				{
					cv::Mat markup = nn.annotated_image.clone();
					cv::circle(markup, left_landmark	, 5, red, cv::FILLED);
					cv::circle(markup, right_landmark	, 5, red, cv::FILLED);
					cv::line(markup, left_landmark, right_landmark, red, 2, cv::LINE_AA);

					cv::Point p(right_landmark.x, left_landmark.y);
					cv::line(markup, left_landmark, p	, light_blue, 2, cv::LINE_AA);
					cv::line(markup, p, right_landmark	, light_blue, 2, cv::LINE_AA);

					const std::string text = "angle = " + std::to_string(degrees) + " degrees";
					std::cout << image << ": " << text << std::endl;

					p.x = (left_landmark.x + right_landmark.x) / 2 - 100; // (too lazy to calculate the exact length of the string!)
					p.y = (left_landmark.y + right_landmark.y) / 2;
					cv::putText(markup, text, p, cv::HersheyFonts::FONT_HERSHEY_PLAIN, 1.0, white	, 4, cv::LineTypes::LINE_AA);
					cv::putText(markup, text, p, cv::HersheyFonts::FONT_HERSHEY_PLAIN, 1.0, red		, 1, cv::LineTypes::LINE_AA);

					cv::imshow("markup", markup);
				}

				// apply a counter-rotation for the angle we figured out above
				const auto angle = 0.0f - degrees;
				const float x = original_image.cols / 2.0f;
				const float y = original_image.rows / 2.0f;
				const cv::Point2f center(x, y);

				// this is where the original image is actually rotated
				cv::Mat rotation_matrix = cv::getRotationMatrix2D(center, angle, 1.0);
				cv::Rect2f box = cv::RotatedRect(center, original_image.size(), angle).boundingRect2f();
				rotation_matrix.at<double>(0,2) += box.width  / 2.0 - center.x;
				rotation_matrix.at<double>(1,2) += box.height / 2.0 - center.y;
				cv::Mat rotated_image;
				cv::warpAffine(original_image, rotated_image, rotation_matrix, box.size(), cv::INTER_LINEAR, cv::BORDER_CONSTANT, white);

				// now that the image has been rotated back to level, apply the neural network again to get more accurate results
				results = nn.predict(rotated_image);

				if (true)
				{
					// display the predictions so we can compare pre-rotation and post-rotation results
					cv::imshow("annotated (post-rotation)", nn.annotate());
				}
			}

			std::cout << image << ": " << results << std::endl;

			const auto key = cv::waitKey();
			if (key == 27)
			{
				break;
			}
		}
	}
	catch (const std::exception & e)
	{
		std::cout << e.what() << std::endl;
		rc = 1;
	}

	return rc;
}
