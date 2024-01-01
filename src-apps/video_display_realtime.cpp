/* DarkHelp - C++ helper class for Darknet's C API.
 * Copyright 2019-2024 Stephane Charette <stephanecharette@gmail.com>
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
				<< argv[0] << " <filename.cfg> <filename.names> <filename.weights> <video>" << std::endl;
			throw std::invalid_argument("wrong number of arguments");
		}

		// Load the neural network.  The order of the 3 files does not matter, DarkHelp should figure out which file is which.
		DarkHelp::NN nn(argv[1], argv[2], argv[3]);

		/* Use OpenCV to read the video frames.  Note the technique used below is not necessarily how I'd process webcams
		 * since a long pause is inserted at the time the frame is displayed.  If you're using a webcam, see the other
		 * examples where the filenames contain the word "webcam".
		 */
		const std::string video_filename = argv[4];
		cv::VideoCapture cap(video_filename);
		if (not cap.isOpened())
		{
			throw std::runtime_error("failed to open the video file " + video_filename);
		}

		// determine exactly how much time we should be displaying each frame (so we know how long to wait)
		const double fps = cap.get(cv::CAP_PROP_FPS);
		const size_t duration_in_nanoseconds = std::round(1000000000.0 / fps);
		std::cout << video_filename << ": " << fps << " FPS, meaning we must display each frame for " << duration_in_nanoseconds << " nanoseconds" << std::endl;
		const std::chrono::high_resolution_clock::duration duration_of_each_frame = std::chrono::nanoseconds(duration_in_nanoseconds);
		std::chrono::high_resolution_clock::time_point next_frame_timestamp = std::chrono::high_resolution_clock::now() + duration_of_each_frame;

		while (cap.isOpened())
		{
			cv::Mat frame;
			cap >> frame;
			if (frame.empty())
			{
				// video has ended, no more frames available
				break;
			}

			nn.predict(frame);
			auto annotated_frame = nn.annotate();

			/* If you're doing off-line processing of video files and saving them to disk or writing the results to a file, then
			 * you don't want to pause between each frame.  Instead, you want the processing to go as fast as possible, in which
			 * case you'd skip the following block of lines.
			 *
			 * Otherwise, since we want to show the video as close to "realtime" as we can, then we need to pause at each frame.
			 * The length of time we pause is determined by the FPS of the video, and how long it takes us to process every frame.
			 */
			const auto now = std::chrono::high_resolution_clock::now();
			const auto milliseconds_to_wait = std::chrono::duration_cast<std::chrono::milliseconds>(next_frame_timestamp - now).count();
			if (milliseconds_to_wait > 0) // beware, length of time remaining can be negative if we're falling behind!
			{
				cv::imshow("original video", frame);
				cv::imshow("annotated video", annotated_frame);
				const auto key = cv::waitKey(milliseconds_to_wait);
				if (key == 27)
				{
					break;
				}
			}
			next_frame_timestamp += duration_of_each_frame;
		}
	}
	catch (const std::exception & e)
	{
		std::cout << e.what() << std::endl;
		rc = 1;
	}

	return rc;
}
