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
		if (argc != 4)
		{
			std::cout
			<< "Usage:" << std::endl
			<< argv[0] << " <filename.cfg> <filename.names> <filename.weights>" << std::endl;
			throw std::invalid_argument("wrong number of arguments");
		}

		// Load the neural network.  The order of the 3 files does not matter, DarkHelp should figure out which file is which.
		DarkHelp::NN nn(argv[1], argv[2], argv[3]);

		// Use OpenCV to open the webcam.  Index zero is the first webcam.  Attemp to set a few camera properties.
		cv::VideoCapture cap(0);
		if (not cap.isOpened())
		{
			throw std::runtime_error("failed to open the webcam");
		}
		cap.set(cv::CAP_PROP_FRAME_WIDTH, 640.0);
		cap.set(cv::CAP_PROP_FRAME_HEIGHT, 480.0);
		cap.set(cv::CAP_PROP_FPS, 30.0);

		while (cap.isOpened())
		{
			cv::Mat frame;
			cap >> frame;
			if (frame.empty())
			{
				break;
			}

			nn.predict(frame);
			frame = nn.annotate();

			/* This is an over-simplistic example, where the waitKey() timeout is hard-coded.  You want the timeout to be short
			 * enough for the OpenCV HighGUI event loop to have time to process events, but not too long that the code doesn't
			 * have time to keep up with new incoming video frames.
			 *
			 * An assumption is made that most webcams might default to 30 FPS, meaning 33.33333 milliseconds per frame.  Sleep
			 * just half of that time to process HighGUI events, and instead spend the extra remaining time waiting for the next
			 * frame to be extracted.
			 */
			cv::imshow("video", frame);
			const auto key = cv::waitKey(15);
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
