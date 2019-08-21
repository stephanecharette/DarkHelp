/* DarkHelp - C++ helper class for Darknet's C API.
 * Copyright 2019 Stephane Charette <stephanecharette@gmail.com>
 * MIT license applies.  See "license.txt" for details.
 * $Id$
 */

#pragma once

#include <chrono>
#include <string>
#include <vector>
#include <opencv2/opencv.hpp>


/** @file
 * DarkHelp is a C++ helper layer for accessing Darknet.  It was developed and tested with AlexeyAB's fork of the
 * popular Darknet project:  https://github.com/AlexeyAB/darknet
 *
 * @note The original darknet.h header file defines structures in the global namespace with names such as "image" and
 * "network" which are likely to cause problems in large existing projects.  For this reason, the DarkHelp class uses
 * a void* for the network and will only include darknet.h if explicitly told it can.
 *
 * Unless you are using darknet's "image" class directly in your application, it is probably best to NOT define this
 * macro, and not include darknet.h.  (The header is included by DarkHelp.cpp, so you definitely still need to have it,
 * but the scope of where it is needed is confined to that one .cpp file.)
 */
#ifdef DARKHELP_CAN_INCLUDE_DARKNET
#include <darknet.h>
#endif


/** Instantiate one of these objects by giving it the name of the .cfg and .weights file,
 * then call @ref predict() as often as necessary to determine what the images contain.
 * For example:
 * ~~~~
 * DarkHelp darkhelp("my_neural_network.cfg", "my_neural_network.weights", "my_neural_network.names");
 *
 * for (const auto & filename : {"image_0.jpg", "image_1.jpg", "image_2.jpg"})
 * {
 *     darkhelp.predict(filename);
 *     cv::Mat mat = darkhelp.annotate();
 *     cv::imshow("prediction", mat);
 *     cv::waitKey();
 * }
 * ~~~~
 *
 * Instead of calling @ref annotate(), you can get the detection results and iterate through them:
 * ~~~~
 * DarkHelp darkhelp("my_neural_network.cfg", "my_neural_network.weights", "my_neural_network.names");
 * for (const auto & det : darkhelp.predict("test_image_01.jpg"))
 * {
 *     std::cout << det.name << ": " << 100.0 * det.probability << "%" << std::endl;
 * }
 * ~~~~
 */
class DarkHelp
{
	public:

		/// Vector of text strings.  Typically used to store the class names.
		typedef std::vector<std::string> VStr;

		struct PredictionResult
		{
			cv::Rect rect;
			int class_id;
			float probability;
			std::string name;
		};
		typedef std::vector<PredictionResult> PredictionResults;

		/// Destructor.
		virtual ~DarkHelp();

		/// Constructor.
		DarkHelp(const std::string & cfg_filename, const std::string & weights_filename, const std::string & names_filename = "");

		/** Use the neural network to predict what is contained in this image.
		 * @param [in] image_filename The name of the image file to load from disk and analyze.  The member
		 * @ref original_image will be set to this image.
		 * @param [in] new_threshold Which threshold to use.  If less than zero, the previous threshold will be applied.
		 * If >= 0, then @ref threshold will be set to this new value.
		 * @see @ref duration
		 */
		virtual PredictionResults predict(const std::string & image_filename, const float new_threshold = -1.0f);

		/** Use the neural network to predict what is contained in this image.
		 * @param [in] mat A OpenCV2 image which has already been loaded and which needs to be analyzed.  The member
		 * @ref original_image will be set to this image.
		 * @param [in] new_threshold Which threshold to use.  If less than zero, the previous threshold will be applied.
		 * If >= 0, then @ref threshold will be set to this new value.
		 * @see @ref duration
		 */
		virtual PredictionResults predict(cv::Mat mat, const float new_threshold = -1.0f);

#ifdef DARKHELP_CAN_INCLUDE_DARKNET
		/** Use the neural network to predict what is contained in this image.
		 * @param [in] mat A Darknet-style image object which has already been loaded and which needs to be analyzed.
		 * The member @ref original_image will be set to this image.
		 * @param [in] new_threshold Which threshold to use.  If less than zero, the previous threshold will be applied.
		 * If >= 0, then @ref threshold will be set to this new value.
		 * @see @ref duration
		 */
		virtual PredictionResults predict(image img, const float new_threshold = -1.0f);
#endif

		/** Takes the most recent @ref prediction_results, and applies them to the most recent @ref original_image.
		 * the output annotated image is stored in @ref annotated_image as well as returned to the caller.
		 * @param [in] new_threshold Which threshold to use.  If less than zero, the previous threshold will be applied.
		 * If >= 0, then @ref threshold will be set to this new value.
		 * @see @ref annotation_colour
		 * @see @ref annotation_font_scale
		 * @see @ref annotation_font_thickness
		 * @see @ref annotation_include_duration
		 * @see @ref annotation_include_timestamp
		 */
		virtual cv::Mat annotate(const float new_threshold = -1.0f);

		/** Return the @ref duration as a text string which can then be added to the image during annotation.
		 * @see @ref annotate()
		 */
		std::string duration_string();

#ifdef DARKHELP_CAN_INCLUDE_DARKNET
		/** Static function to convert the OpenCV @p cv::Mat objects to Darknet's internal @p image format.
		 * Provided for convenience in case you need to call into one of Darknet's functions.
		 * @see @ref convert_darknet_image_to_opencv_mat()
		 */
		static image convert_opencv_mat_to_darknet_image(cv::Mat mat);

		/** Static function to convert Darknet's internal @p image format to OpenCV's @p cv::Mat format.
		 * Provided for convenience in case you need to manipulate a Darknet image.
		 * @see @ref convert_opencv_mat_to_darknet_image()
		 */
		static cv::Mat convert_darknet_image_to_opencv_mat(const image img);

		/** The Darknet network.  This is setup in the constructor.
		 * @note Unfortunately, the Darknet C API does not allow this to be de-allocated.
		 */
		network * net;
#else
		/** The Darknet network, but stored as a void* pointer so we don't have to include darknet.h.
		 * @note Unfortunately, the Darknet C API does not allow this to be de-allocated.
		 */
		void * net;
#endif

		/** A vector of names corresponding to the identified classes.  This is typically setup in the constructor,
		 * but can be manually set afterwards.
		 */
		VStr names;

		/** The length of time it took to initially load the network and weights (after the DarkHelp object has been
		 * constructed), or the length of time @ref predict() took to run on the last image to be processed.
		 * @see @ref duration_string()
		 */
		std::chrono::high_resolution_clock::duration duration;

		/// Image prediction threshold.  Defaults to 0.5.  @see @ref predict()  @see @ref annotate()
		float threshold;

		/// Used during prediction.  Defaults to 0.5.  @see @ref predict()  @todo need more details on this one.
		float hierchy_threshold;

		/** Non-Maximal Suppression (NMS) threshold suppresses overlapping bounding boxes and only retains the bounding
		 * box that has the maximum probability of object detection associated with it.  Defaults to 0.45.
		 * @see @ref predict()
		 */
		float non_maximal_suppression_threshold;

		/// The most recent results after applying the neural network to an image.  This is set by @ref predict().
		PredictionResults prediction_results;

		/// The colour to use in @ref annotate().  Defaults to purple @p "(255, 0, 255)".
		cv::Scalar annotation_colour;

		/// Scaling factor used for the font in @ref annotate().  Defaults to 0.5.
		double annotation_font_scale;

		/// Thickness of the font in @ref annotate().  Defaults to 1.
		int annotation_font_thickness;

		/** If set to @p true then @ref annotate() will call @ref duration_string() and display on the image the length
		 * of time @ref predict() took to process the image.  Defaults to true.
		 */
		bool annotation_include_duration;

		/// If set to @p true then @ref annotate() will display a timestamp on the image.  Defaults to false.
		bool annotation_include_timestamp;

		/// The most recent image handled by @ref predict().
		cv::Mat original_image;

		/// The most recent output produced by @ref annotate().
		cv::Mat annotated_image;


	protected:

		/** @internal Used by all the other @ref predict() calls to do the actual network prediction.  This uses the
		 * image stored in @ref original_image.
		 */
		PredictionResults predict(const float new_threshold = -1.0f);
};
