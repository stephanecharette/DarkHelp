/* DarkHelp - C++ helper class for Darknet's C API.
 * Copyright 2019-2024 Stephane Charette <stephanecharette@gmail.com>
 * MIT license applies.  See "license.txt" for details.
 */


#include "DarkHelp.hpp"
#include "CamOptions.hpp"


int main(int argc, char * argv[])
{
	int rc = 1;

	try
	{
		#ifndef HAVE_OPENCV_HIGHGUI
		...do something here
		#endif

		DarkHelp::Config config;
		DarkHelp::CamOptions options;

		DarkHelp::parse(options, config, argc, argv);
		DarkHelp::NN nn(config);

		cv::VideoCapture cap;
		cap.setExceptionMode(true);
		if (options.device_index >= 0)
		{
			std::cout << "-> opening camera device index #" << options.device_index << std::endl;
#if 0 // not available until newer versions of OpenCV
			DarkHelp::VInt v =
			{
				cv::CAP_PROP_BUFFERSIZE,		5,
//				cv::CAP_PROP_HW_ACCELERATION,	cv::VIDEO_ACCELERATION_ANY,
				cv::CAP_PROP_FRAME_WIDTH,		options.size_request.width,
				cv::CAP_PROP_FRAME_HEIGHT,		options.size_request.height,
				cv::CAP_PROP_FPS,				static_cast<int>(std::round(options.fps_request)),
			};
			cap.open(options.device_index, options.device_backend, v);
#else
			cap.open(options.device_index, options.device_backend);
#endif
			if (not cap.isOpened())
			{
				throw std::runtime_error("failed to open camera index #" + std::to_string(options.device_index));
			}
		}
		else
		{
			std::cout << "-> opening filename \"" << options.device_filename << "\"" << std::endl;
			cap.open(options.device_filename, options.device_backend);
			if (not cap.isOpened())
			{
				throw std::runtime_error("failed to open \"" + options.device_filename + "\"");
			}
		}

		std::cout << "-> video backend API: " << cap.getBackendName() << std::endl;

		if (options.fps_request > 0.0)
		{
			std::cout << "-> attempting to set the video device to " << options.fps_request << " FPS" << std::endl;
			cap.set(cv::CAP_PROP_FPS, options.fps_request);
		}

		if (options.size_request.width > 0 and options.size_request.height > 0)
		{
			std::cout << "-> attempting to set the video dimensions to " << options.size_request.width << "x" << options.size_request.height << std::endl;
			cap.set(cv::CAP_PROP_FRAME_WIDTH, options.size_request.width);
			cap.set(cv::CAP_PROP_FRAME_HEIGHT, options.size_request.height);
		}

		int milliseconds_to_wait = 10;
		cv::Mat first_frame;
		cap >> first_frame;
		if (first_frame.empty())
		{
			std::cout << "-> failed to read video frame" << std::endl;

			// looks like things are about to fail, but still make an attempt
			options.fps_actual	= options.fps_request;
			options.size_actual	= options.size_request;
		}
		else
		{
			options.fps_actual			= cap.get(cv::CAP_PROP_FPS);
			options.size_actual.width	= first_frame.cols;
			options.size_actual.height	= first_frame.rows;

			std::cout << "-> input video claims to be " << first_frame.cols << "x" << first_frame.rows << " @ " << options.fps_actual << " FPS" << std::endl;

			// see if we can confirm the FPS since either V4L2 or OpenCV seems to get that wrong most of the time
			bool ok = true;
			int read_this_many_frames = std::max(10.0, std::ceil(options.fps_actual));
			const auto timestamp_start = std::chrono::high_resolution_clock::now();
			for (int idx = 0; idx < read_this_many_frames; idx ++)
			{
				cv::Mat mat;
				cap >> mat;
				if (mat.size() != first_frame.size())
				{
					ok = false;
					break;
				}
			}
			const auto timestamp_end = std::chrono::high_resolution_clock::now();
			if (not ok)
			{
				std::cout << "-> failed to read initial video frames" << std::endl;
			}
			else
			{
				// 1 second = 1,000,000,000 nanoseconds
				const auto duration = timestamp_end - timestamp_start;
				const double length_in_nanoseconds = std::chrono::duration_cast<std::chrono::nanoseconds>(duration).count();
				const double real_fps = 1000000000.0 / length_in_nanoseconds * read_this_many_frames;

				std::cout << "-> took " << DarkHelp::duration_string(duration) << " to read " << read_this_many_frames << " frames, giving us " << real_fps << " FPS" << std::endl;
				const auto ratio = options.fps_actual / real_fps;
				if (ratio < 0.9 or ratio > 1.1)
				{
					std::cout << "-> modfying input video from " << options.fps_actual << " FPS to " << real_fps << " FPS" << std::endl;
					options.fps_actual = real_fps;
				}
			}
			milliseconds_to_wait = std::max(5.0, std::min(10.0, std::round(1000.0 / 2.0 / options.fps_actual)));
			std::cout << "-> HighGUI event timeout is set to " << milliseconds_to_wait << " milliseconds which is good up to " << std::floor(1000.0 / milliseconds_to_wait) << " FPS" << std::endl;
		}

		if (options.fps_actual <= 0)
		{
			std::cout << "-> " << options.fps_actual << " FPS seems to be invalid" << std::endl;
			options.fps_actual = 10.0;
		}

		if (options.size_actual.width < 10 or options.size_actual.height < 10)
		{
			std::cout << "-> video dimensions of " << options.size_actual.width << "x" << options.size_actual.height << " seems to be invalid" << std::endl;
			options.size_actual = cv::Size(640, 480);
		}

		cv::Size final_size(options.size_actual);
		bool resize_before	= false;
		bool resize_after	= false;
		size_t errors		= 0;

		if (options.resize_before.width > 0 and options.resize_before.height > 0)
		{
			resize_before = true;
			if (not first_frame.empty())
			{
				// the size requested may need to be adjusted to keep the aspect ratio the same
				auto tmp = DarkHelp::resize_keeping_aspect_ratio(first_frame, options.resize_before);
				options.resize_before = tmp.size();
			}
			std::cout << "-> resizing video frames before inference to " << options.resize_before.width << "x" << options.resize_before.height << std::endl;
			final_size = options.resize_before;
		}
		if (options.resize_after.width > 0 and options.resize_after.height > 0)
		{
			resize_after = true;
			if (not first_frame.empty())
			{
				// the size requested may need to be adjusted to keep the aspect ratio the same
				auto tmp = DarkHelp::resize_keeping_aspect_ratio(first_frame, options.resize_after);
				options.resize_after = tmp.size();
			}
			std::cout << "-> resizing video frames after annotation to " << options.resize_after.width << "x" << options.resize_after.height << std::endl;
			final_size = options.resize_after;
		}

		cv::VideoWriter output("output.mp4", cv::VideoWriter::fourcc('m', 'p', '4', 'v'), options.fps_actual, final_size);
		if (not output.isOpened())
		{
			std::cout << "-> failed to open output.mp4 (video will not be saved!)" << std::endl;
		}
		else
		{
			std::cout << "-> output video will be " << final_size.width << "x" << final_size.height << " @ " << options.fps_actual << " FPS" << std::endl;
		}

		size_t max_frame_counter = 0;
		if (options.capture_seconds > 0)
		{
			max_frame_counter = options.fps_actual * options.capture_seconds;
			std::cout << "-> max frame counter is set to " << max_frame_counter << " (" << (max_frame_counter / options.fps_actual) << " seconds)" << std::endl;
		}

		// if we got here, then assume that everything is good
		rc = 0;
		const int fps_rounded = std::round(options.fps_actual);
		if (options.show_gui)
		{
			std::cout << "-> press ESC to stop" << std::endl;
		}

		// Create a map where we'll track which objects were seen when.  The key is the object name, the value is the frame index.
		std::map<std::string, size_t> m;
		std::string previously_seen_objects;

		size_t frame_counter = 0;
		while (cap.isOpened() and errors < 5)
		{
			cv::Mat frame;
			cap >> frame;
			if (frame.empty())
			{
				// Was the camera disconnected?  Or a bad frame?  End of the video?
				errors ++;
				continue;
			}
			errors = 0;

			if ((frame_counter % fps_rounded) == 0)
			{
				std::cout << "\rframe #" << frame_counter << " " << std::flush;
			}
			frame_counter ++;

			if (resize_before)
			{
				frame = DarkHelp::resize_keeping_aspect_ratio(frame, options.resize_before);
			}

			const auto results = nn.predict(frame);
			frame = nn.annotate();

			// update the map that tracks when an object was last seen
			for (const auto & pred : results)
			{
				const auto & key = nn.names[pred.best_class];
#if 0
				if (m.count(key) == 0 or						// object was previously undetected...
					m[key] + fps_rounded * 2 < frame_counter)	// ...or was last seen more than 2 seconds ago
				{
					new_object_found = true;
				}
#endif
				m[key] = frame_counter;
			}

			// come up with a new string of recently seen objects
			std::string str;
			for (const auto & [key, val] : m)
			{
				if (val + fps_rounded * 4 >= frame_counter)
				{
					// we're recently seen this object
					if (not str.empty())
					{
						str += ", ";
					}
					str += key;
				}
			}
			if (str != previously_seen_objects)
			{
				std::cout << "\rframe #" << frame_counter << ": " << str << std::endl;
				previously_seen_objects = str;
			}

			if (resize_after)
			{
				frame = DarkHelp::resize_keeping_aspect_ratio(frame, options.resize_after);
			}

			if (output.isOpened())
			{
				output.write(frame);
			}

			if (max_frame_counter > 0 and frame_counter > max_frame_counter)
			{
				std::cout << std::endl << "Exiting!" << std::endl;
				break;
			}

			if (options.show_gui)
			{
				cv::imshow("DarkHelp Camera Output", frame);
				const auto key = cv::waitKey(milliseconds_to_wait);
				if (key == 27)
				{
					std::cout << std::endl << "ESC detected -- exiting!" << std::endl;
					break;
				}
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
