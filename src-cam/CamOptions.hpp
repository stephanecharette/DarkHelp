/* DarkHelp - C++ helper class for Darknet's C API.
 * Copyright 2019-2024 Stephane Charette <stephanecharette@gmail.com>
 * MIT license applies.  See "license.txt" for details.
 */


#include "DarkHelp.hpp"


namespace DarkHelp
{
	class CamOptions final
	{
		public:

			bool		show_gui;			///< Set to @p true if OpenCV HighGUI can be used, set to @p false to run in a console window.
			std::string	device_filename;	///< A specific filename to use such as @p /dev/video0 when @ref device_index is set to @p -1.
			int			device_index;		///< The OpenCV device index to use, unless it is set to @p -1 in which case @ref device_filename is used instead.
			int			device_backend;		///< Which backend OpenCV should be using (CAP_ANY, CAP_V4L2, CAP_FFMPEG, ...).
			double		fps_request;		///< Frames per second requested by the user.
			double		fps_actual;			///< Frames per second as determined by OpenCV, which is not necessarily the same as what was requested.
			cv::Size	size_request;		///< The video dimensions the user want to be using.
			cv::Size	size_actual;		///< The video dimensions as determined by OpenCV, which is not necessarily the same as what was requested.
			cv::Size	resize_before;		///< If set, images will be resized to this dimension prior to calling Darknet.
			cv::Size	resize_after;		///< If set, images will be resized to this dimension after annotating by DarkHelp.
			int			capture_seconds;	///< The length of time (in seconds) to run before exiting.

			/// Constructor.
			CamOptions();

			/// Destructor
			~CamOptions();

			/// Reset all of the options to their default values.  This is automatically called by the constructor.
			CamOptions & reset();
	};

	void parse(CamOptions & cam_options, DarkHelp::Config & config, int argc, char * argv[]);
}
