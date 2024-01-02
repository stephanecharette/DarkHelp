/* DarkHelp - C++ helper class for Darknet's C API.
 * Copyright 2019-2024 Stephane Charette <stephanecharette@gmail.com>
 * MIT license applies.  See "license.txt" for details.
 */

#pragma once

#include <chrono>
#include <map>
#include <string>
#include <vector>
#include <list>
#include <opencv2/opencv.hpp>
// More headers are included at the bottom of this file.


/** @file
 * DarkHelp is a C++ helper layer for accessing Darknet.  It was developed and tested with AlexeyAB's fork of the
 * popular Darknet project, which is now maintained by Hank.ai:  https://github.com/hank-ai/darknet
 *
 * @note The original darknet.h header file defines structures in the global namespace with names such as "image" and
 * "network" which are likely to cause problems in large existing projects.  For this reason, the DarkHelp class uses
 * a @p void* for the network and will only include darknet.h if explicitly told it can.
 *
 * Unless you are using darknet's "image" class directly in your application, it is probably best to NOT define the
 * @p DARKHELP_CAN_INCLUDE_DARKNET macro, and not include darknet.h.  (The header is included by DarkHelpNN.cpp, so
 * you definitely still need to have it, but the scope of where it is needed is confined to that one .cpp file.)
 */
#ifdef DARKHELP_CAN_INCLUDE_DARKNET
#include <darknet.h>
#endif


/** The %DarkHelp namespace contains (almost) everything in the %DarkHelp library.  Prior to version 1.4, @p %DarkHelp
 * was the name of a class.  But in October/November 2021, a large code re-organization took place, and the previous
 * class definition was split into multiple classes across several files.  This makes things easier to manage and was
 * needed to support other projects like DarkHelpFPS.
 *
 * Appologies to everyone who has code that relied on the previous @p %DarkHelp API.  The old @p %Darkhelp class has
 * been renamed to @ref DarkHelp::NN, and the settings that used to be in @p %DarkHelp have been moved to
 * @ref DarkHelp::NN::config.
 *
 * @since November 2021
 *
 * @see @ref DarkHelp::Config
 * @see @ref DarkHelp::PredictionResult
 * @see @ref DarkHelp::NN
 */
namespace DarkHelp
{
	/* OpenCV4 has renamed some common defines and placed them in the cv namespace.
	 * Need to deal with this until older versions of OpenCV are no longer in use.
	 */
	#if CV_VERSION_MAJOR >= 4
	constexpr auto CV_INTER_CUBIC	= cv::INTER_CUBIC;
	constexpr auto CV_INTER_AREA	= cv::INTER_AREA;
	constexpr auto CV_AA			= cv::LINE_AA;
	constexpr auto CV_FILLED		= cv::FILLED;
	#endif

	/// Map of strings where both the key and the value are @p std::string.
	using  MStr = std::map<std::string, std::string>;

	/// Vector of text strings.  Typically used to store the class names.
	using VStr = std::vector<std::string>;

	/// Vector of colours to use by @ref DarkHelp::NN::annotate().  @see @ref DarkHelp::Config::annotation_colours  @see @ref DarkHelp::get_default_annotation_colours()
	using VColours = std::vector<cv::Scalar>;

	/// Vector of @p int used with OpenCV.
	using VInt = std::vector<int>;

	/// Vector of @p float used with OpenCV.
	using VFloat = std::vector<float>;

	/// Vector of OpenCV rectangles used with OpenCV.
	using VRect = std::vector<cv::Rect>;

	/// Similar to @ref DarkHelp::VRect, but the rectangle uses @p double instead of @p int.
	using VRect2d = std::vector<cv::Rect2d>;

	/** %DarkHelp can utilise either @p libdarknet.so or OpenCV's DNN module to load the neural network and run inference.
	 * OpenCV is much faster, but support for it is relatively new in %DarkHelp and support for newer models like YOLOv4
	 * requires @em very recent versions of OpenCV.  The default is @p kDarknet.
	 *
	 * @see @ref DarkHelp::NN::init()
	 *
	 * @note Setting the driver to any value other than @p kDarknet will result in the execution of experimental code.
	 *
	 * If using @p kOpenCV or @p kOpenCVCPU you can customize the backend and target after DarkHelp::init() is called.  For example:
	 * ~~~~
	 * DarkHelp::NN nn;
	 * nn.init("rocks.cfg", "rocks.weights", "rocks.names", true, DarkHelp::EDriver::kOpenCV);
	 * nn.opencv_net.setPreferableBackend(cv::dnn::DNN_BACKEND_CUDA);
	 * nn.opencv_net.setPreferableTarget(cv::dnn::DNN_TARGET_CUDA_FP16);
	 * ~~~~
	 *
	 * @since October 2021
	 */
	enum class EDriver
	{
		kInvalid	= 0,
		kMin		= 1,
		kDarknet	= kMin,	///< Use @p libdarknet.so.
		kOpenCV		,		///< Use OpenCV's @p dnn module.  Attempts to use CUDA, and will automatically revert to CPU if CUDA is not available.
		kOpenCVCPU	,		///< Use OpenCV's @p dnn module, but skip CUDA and only use the CPU
		kMax		= kOpenCVCPU
	};

	/// @see @ref DarkHelp::Config::sort_predictions
	enum class ESort
	{
		kUnsorted	= 0,	///< Do not sort predictions.
		kAscending	,		///< Sort predictions using @ref DarkHelp::PredictionResult::best_probability in ascending order (low values first, high values last).
		kDescending	,		///< Sort predictions using @ref DarkHelp::PredictionResult::best_probability in descending order (high values first, low values last).
		kPageOrder			///< Sort predictions based @em loosely on where they appear within the image.  From top-to-bottom, and left-to-right.
	};
}

#include "DarkHelpPredictionResult.hpp"
#include "DarkHelpConfig.hpp"
#include "DarkHelpNN.hpp"
#include "DarkHelpUtils.hpp"
#include "DarkHelpPositionTracker.hpp"

/* The C API should not be required or necessary when using the C++ API,
 * but may as well make everything as easy to use as possible.
 */
#include "DarkHelp_C_API.h"
