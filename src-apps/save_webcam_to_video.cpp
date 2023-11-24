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
		cap.set(cv::CAP_PROP_FPS, 30.0);
		cap.set(cv::CAP_PROP_FRAME_WIDTH, 640.0);
		cap.set(cv::CAP_PROP_FRAME_HEIGHT, 480.0);

		/* Be careful here.  Just because we requested a certain size and FPS doesn't mean the camera supports that mode,
		 * or that OpenCV was successful in setting those values.  You should check to see if the values we think we set are
		 * the values that OpenCV and the webcam are using.
		 */
		cv::Mat frame;
		cap >> frame;
		const double fps = cap.get(cv::CAP_PROP_FPS);
		cv::VideoWriter out("out.mp4", cv::VideoWriter::fourcc('m', 'p', '4', 'v'), fps, frame.size());

		/* The length of time we wait should be 1000 milliseconds divided by the FPS.  So for 30 FPS video, we would normally
		 * wait 33.333 milliseconds between each frame.  But this example is very simple, and we're not getting the current
		 * time to calibrate against to ensure each frame is shown for the right duration.  Plus we make no attempt in the
		 * code below to take into account the time it takes to run nn.predict() and nn.annotate().
		 *
		 * So as a simple solution for a simple app, let's divide the wait time in half.  This should give the HighGUI event
		 * loop time to run, but shouldn't be too much where we introduce lag.
		 */
		const int milliseconds_to_wait = std::round(1000.0 / 2.0 / fps);
		while (cap.isOpened())
		{
			cap >> frame;
			if (frame.empty())
			{
				// something went wrong -- for example, webcam may have been disconnected
				break;
			}

			nn.predict(frame);
			auto annotated_frame = nn.annotate();
			out.write(annotated_frame);

			cv::imshow("video", annotated_frame);
			const auto key = cv::waitKey(milliseconds_to_wait);
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
