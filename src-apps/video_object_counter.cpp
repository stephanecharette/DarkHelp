/* DarkHelp - C++ helper class for Darknet's C API.
 * Copyright 2019-2024 Stephane Charette <stephanecharette@gmail.com>
 * MIT license applies.  See "license.txt" for details.
 */

#include <DarkHelp.hpp>

/* A possible input video to use for this sample app is this one:
 *
 *		https://youtu.be/F191OV3TqFw
 *
 * The output, given a network trained to detect pigs, should look like this:
 *
 *		https://youtu.be/2biQpVRFhbk
 */


// Set this to "1" if you want to also create an output video.
// Otherwise, the output is only shown on screen.
#define SAVE_OUTPUT_VIDEO 0


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
		DarkHelp::NN nn(argv[1], argv[2], argv[3], true, DarkHelp::EDriver::kOpenCV);

		DarkHelp::PositionTracker tracker;
		tracker.maximum_number_of_frames_per_object = 10;

		const cv::Scalar black	(0x00, 0x00, 0x00);
		const cv::Scalar red	(0x00, 0x00, 0xff);
		const cv::Scalar green	(0x00, 0xff, 0x00);
		const cv::Scalar blue	(0xff, 0x00, 0x00);

		int key = -1;

		/* Use OpenCV to read the video frames.  Note the technique used below is not necessarily how I'd process webcams
		 * since a long pause is inserted at the time the frame is displayed.  If you're using a webcam, see the other
		 * example apps where the filenames contain the word "webcam".
		 */
		const std::string video_filename = argv[4];
		cv::VideoCapture cap(video_filename);
		if (not cap.isOpened())
		{
			throw std::runtime_error("failed to open the video file " + video_filename);
		}

		// get the first frame so we know exactly what size we're dealing with
		cv::Mat frame;
		cap >> frame;
		cap.set(cv::CAP_PROP_POS_FRAMES, 0.0);
		const double fps = cap.get(cv::CAP_PROP_FPS);
		const int width = frame.cols;
		const int height = frame.rows;
		const int vertical_boundary_line = width / 2;
		int64_t object_counter = 0;

		#if SAVE_OUTPUT_VIDEO > 0
		cv::VideoWriter output("output.mp4", cv::VideoWriter::fourcc('m', 'p', '4', 'v'), fps, frame.size());
		#endif

		// determine exactly how much time we should be displaying each frame (so we know how long to wait)
		const size_t duration_in_nanoseconds = std::round(1000000000.0 / fps);
		std::cout << video_filename << ": " << fps << " FPS, meaning we must display each frame for " << duration_in_nanoseconds << " nanoseconds" << std::endl;
		const std::chrono::high_resolution_clock::duration duration_of_each_frame = std::chrono::nanoseconds(duration_in_nanoseconds);
		std::chrono::high_resolution_clock::time_point next_frame_timestamp = std::chrono::high_resolution_clock::now() + duration_of_each_frame;

		while (cap.isOpened())
		{
			cap >> frame;
			if (frame.empty())
			{
				// video has ended, no more frames available
				break;
			}

			auto results = nn.predict(frame);
			tracker.add(results);

			for (const auto & pred : results)
			{
				// say we only want to track objects with a class id #0, and ignore everything else
				if (pred.best_class != 0)
				{
					continue;
				}

				const auto & obj = tracker.get(pred.object_id);

				int current_x = -1;
				int previous_x = -1;

				// determine the average "X" positon for this object over the last few frames
				// so we can determine the direction in which it is moving
				float frame_counter = 0.0f;
				float average_x = 0.0f;
				for (auto iter = obj.fids_and_rects.crbegin(); frame_counter < 5.0f and iter != obj.fids_and_rects.crend(); iter ++)
				{
					frame_counter ++;
					const int x = iter->second.x;
					average_x += x;

					if (current_x == -1)
					{
						current_x = x;
					}
					else if (previous_x == -1)
					{
						previous_x = x;

						// now that we know both the current and previous X coordinate,
						// see if the object has crossed the line so we can adjust the object counter
						if (previous_x < vertical_boundary_line and current_x >= vertical_boundary_line)
						{
							object_counter ++;
						}
						else if (previous_x >= vertical_boundary_line and current_x < vertical_boundary_line)
						{
							object_counter --;
						}
					}
				}
				average_x /= frame_counter;

				// compare the most recent tracker object rectangle to the average to see if we're moving left->right, right->left, or sitting still
				auto colour = black;
				if (obj.fids_and_rects.crbegin()->second.x - average_x >= 3)
				{
					colour = green; // left-to-right
				}
				else if (obj.fids_and_rects.crbegin()->second.x - average_x <= -3)
				{
					colour = red; // right-to-left
				}

				// draw some circles over the past few locations where this object has been tracked
				frame_counter = 0.0f;
				for (auto iter = obj.fids_and_rects.rbegin(); frame_counter < 5.0f and iter != obj.fids_and_rects.rend(); iter ++)
				{
					frame_counter ++;
					const auto r = iter->second;
					cv::Point p(r.x + r.width / 2, r.y + r.height / 2);
					cv::circle(frame, p, 10, colour, 3, cv::LINE_AA);
				}
				cv::putText(frame, std::to_string(obj.oid), obj.center(), cv::FONT_HERSHEY_SIMPLEX, 1.25, black, 2, cv::LINE_AA);
			}

			// draw the vertical line and display the total count at the top of the frame
			cv::line(frame, cv::Point(vertical_boundary_line, 0), cv::Point(vertical_boundary_line, height), blue, 1);
			cv::putText(frame, std::to_string(object_counter), cv::Point(vertical_boundary_line, 35), cv::FONT_HERSHEY_SIMPLEX, 1.25, blue, 2, cv::LINE_AA);

			#if SAVE_OUTPUT_VIDEO > 0
			output.write(frame);
			#endif

			// show the frame and sleep for exactly the right amount of time so the video is displayed "realtime"
			const auto now = std::chrono::high_resolution_clock::now();
			auto milliseconds_to_wait = std::chrono::duration_cast<std::chrono::milliseconds>(next_frame_timestamp - now).count();

			#if 0
			// if you're running this on a slow computer, you may need to force
			// the timeout to be non-negative, otherwise you'll never see anything
			if (milliseconds_to_wait < 5)
			{
				milliseconds_to_wait = 5;
			}
			#endif

			if (milliseconds_to_wait > 0) // beware, length of time remaining can be negative if we're falling behind!
			{
				cv::imshow("counting objects", frame);

				while (true)
				{
					// press spacebar to pause or unpause, ESC to exit, any other key to continue
					key = cv::waitKey(milliseconds_to_wait);

					if (key == -1)
					{
						// regular timeout happened (video is not paused, otherwise we wouldn't timeout)
						break;
					}
					else if (key == 27)
					{
						// we've been asked to exit completely
						cap.release();
						break;
					}
					else if (key == 32 and milliseconds_to_wait < 0)
					{
						// we've been asked to unpause the video

						// since we've been paused for a while, reset the timestamp when the next frame is expected to be shown
						next_frame_timestamp = std::chrono::high_resolution_clock::now();
						break;
					}
					else if (key == 32 and milliseconds_to_wait >= 0)
					{
						// we've been asked to pause the video
						milliseconds_to_wait = -1;
						continue;
					}
				}
			}
			next_frame_timestamp += duration_of_each_frame;
		}

		#if SAVE_OUTPUT_VIDEO > 0
		output.release();
		#endif

		if (key != 27)
		{
			// pause to leave the last frame up on the screen
			cv::waitKey();
		}
	}
	catch (const std::exception & e)
	{
		std::cout << e.what() << std::endl;
		rc = 1;
	}

	return rc;
}
