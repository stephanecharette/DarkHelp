/* DarkHelp - C++ helper class for Darknet's C API.
 * Copyright 2019-2024 Stephane Charette <stephanecharette@gmail.com>
 * MIT license applies.  See "license.txt" for details.
 */

#pragma once

// Do not include this header file directly.  Instead, your project should include DarkHelp.hpp
// which will then include all of the necessary secondary headers in the correct order.

#include "DarkHelp.hpp"

/** @file
 * DarkHelp's @p NN class.  Prior to v1.4, this used to be called @p %DarkHelp.
 */


namespace DarkHelp
{
	/** Instantiate one of these objects by giving it the name of the .cfg and .weights file,
	 * then call @ref DarkHelp::NN::predict() as often as necessary to determine what the images contain.
	 * For example:
	 * ~~~~
	 * DarkHelp::NN nn("mynetwork.cfg", "mynetwork.weights", "mynetwork.names");
	 *
	 * const auto image_filenames = {"image_0.jpg", "image_1.jpg", "image_2.jpg"};
	 *
	 * for (const auto & filename : image_filenames)
	 * {
	 *     // these next two lines is where DarkHelp calls into Darknet to do all the hard work
	 *     nn.predict(filename);
	 *     cv::Mat mat = nn.annotate(); // annotates the most recent image seen by predict()
	 *
	 *     // use standard OpenCV calls to show the image results in a window
	 *     cv::imshow("prediction", mat);
	 *     cv::waitKey();
	 * }
	 * ~~~~
	 *
	 * Instead of calling @ref DarkHelp::NN::annotate(), you can get the detection results and iterate through them:
	 *
	 * ~~~~
	 * DarkHelp::NN nn("mynetwork.cfg", "mynetwork.weights", "mynetwork.names");
	 *
	 * const auto results = nn.predict("test_image_01.jpg");
	 *
	 * for (const auto & det : results)
	 * {
	 *     std::cout << det.name << " (" << 100.0 * det.best_probability << "% chance that this is class #" << det.best_class << ")" << std::endl;
	 * }
	 * ~~~~
	 *
	 * Instead of writing your own loop, you can also use the @p std::ostream @p operator<<() like this:
	 *
	 * ~~~~
	 * const auto results = nn.predict("test_image_01.jpg");
	 * std::cout << results << std::endl;
	 * ~~~~
	 */
	class NN final
	{
		public:

			/** We know that Darknet and OpenCV DNN does fancy stuff with the GPU and memory allocated to be used by the GPU.
			 * So delete the copying and moving of @ref DarkHelp::NN objects to prevent problems from happening.  @{
			 */
			NN(const NN &)				= delete;
			NN(NN &&)					= delete;
			NN & operator=(const NN &)	= delete;
			NN & operator=(NN &&)		= delete;
			/// @}

			/// Destructor.  This automatically calls reset() to release memory allocated by the neural network.
			~NN();

			/// Constructor.  When using this constructor, the neural network remains uninitialized until @ref DarkHelp::NN::init() is called.
			NN();

			/** Constructor.  This constructor automatically calls @ref DarkHelp::NN::init() to load the neural network.  The neural
			 * network filenames within the @p cfg object must not be empty otherwise the call to @ref DarkHelp::NN::init() will fail.
			 */
			NN(const Config & cfg);

			/** Constructor.  This constructor automatically calls @ref DarkHelp::NN::init() with the given parameters.
			 *
			 * @note The order in which you pass the various filenames is @em not important if @p verify_files_first is set to
			 * @p true (the default value).  This is because @ref DarkHelp::NN::init() will call @ref DarkHelp::verify_cfg_and_weights()
			 * to correctly determine which is the @p .cfg, @p .weights, and @p .names file, and swap the names around as necessary
			 * so Darknet
			 * is given the correct filenames.
			 */
			NN(const std::string & cfg_filename, const std::string & weights_filename, const std::string & names_filename = "", const bool verify_files_first = true, const EDriver driver = EDriver::kDarknet);

			/// Get a version string for the %DarkHelp library.  E.g., could be `1.5.13-1`.
			static std::string version();

			/** Initialize ("load") the darknet neural network.  If @p verify_files_first has been enabled (the default)
			 * then this method will also call the static method @ref DarkHelp::verify_cfg_and_weights() to perform some
			 * last-minute validation prior to darknet loading the neural network.
			 */
			NN & init(const std::string & cfg_filename, const std::string & weights_filename, const std::string & names_filename = "", const bool verify_files_first = true, const EDriver driver = EDriver::kDarknet);

			/// Initialize ("load") the darknet neural network.  This uses the values within @ref DarkHelp::NN::config.
			NN & init();

			/** The opposite of @ref DarkHelp::NN::init().  This is automatically called by the destructor.
			 * @see @ref clear()
			 */
			NN & reset();

			/** Clear out the images and the predictions stored internally within this object.  This makes it seem as if the
			 * @p NN object has not yet been used to process any images.  Unlike @ref reset(), this does not change any settings,
			 * it only clears the images and the predictions.  If a neural network has been loaded, calling @p clear() does not
			 * unload that neural network, and @ref is_initialized() will continue to return @p true.
			 *
			 * Calling this method between images is not necessary, but is included for completeness.
			 */
			NN & clear();

			/** Determines if a neural network has been loaded.
			 *
			 * For example:
			 * @li When a @p NN object has been created with the default constructor, @p is_initialized() returns @p false.
			 * @li Once @ref init() has been called, @p is_initialized() returns @p true.
			 * @li After @ref clear() has been called, @p is_initialized() would continue to return @p true.
			 * @li After @ref reset() has been called, @p is_initialized() returns @p false.
			 *
			 * @see @ref empty()
			 * @see @ref clear()
			 *
			 * @since 2022-07-12
			 */
			bool is_initialized() const;

			/// Alias for @ref is_initialized().  Returns @p true if the neural network has been loaded.
			bool is_loaded() const { return is_initialized(); }

			/** Only returns @p true if both @ref original_image and @ref prediction_results are both empty.  This will only happen
			 * in the following situations:
			 *
			 * @li when a @ref NN object has been instantiated but @ref init() has not yet been called and no neural network is loaded
			 * @li when a @ref NN object is initialized but no image has been processed
			 * @li after @ref clear() has been called which results in the removal of internal images and predictions
			 *
			 * @see @ref is_initialized()
			 * @see @ref clear()
			 */
			bool empty() const;

			/** Use the neural network to predict what is contained in this image.  This results in a call to either
			 * @ref DarkHelp::NN::predict_internal() or @ref DarkHelp::NN::predict_tile() depending on how
			 * @ref DarkHelp::Config::enable_tiles has been set.
			 *
			 * @param [in] image_filename The name of the image file to load from disk and analyze.  The member
			 * @ref DarkHelp::NN::original_image will be set to this image.  If the image is larger or smaller than the dimensions
			 * of the neural network, then Darknet will stretch the image to match the exact size of the neural network.
			 * Stretching the image does <em>not</em> maintain the the aspect ratio.
			 * @param [in] new_threshold Which threshold to use.  If less than zero, the previous threshold will be applied.
			 * If >= 0, then @ref DarkHelp::Config::threshold will be set to this new value.  The threshold must be either -1,
			 * or a value between 0.0 and 1.0 meaning 0% to 100%.
			 * @returns A vector of @ref DarkHelp::PredictionResult structures, each one representing a different object in the
			 * image. The higher the threshold value, the more "certain" the network is that it has correctly identified the object.
			 *
			 * @see @ref Tiling
			 * @see @ref DarkHelp::NN::predict_tile()
			 * @see @ref DarkHelp::PredictionResult
			 * @see @ref DarkHelp::Config::enable_tiles
			 * @see @ref DarkHelp::Config::sort_predictions
			 * @see @ref DarkHelp::NN::duration
			 */
			PredictionResults predict(const std::string & image_filename, const float new_threshold = -1.0f);

			/** Use the neural network to predict what is contained in this image.  This results in a call to either
			 * @ref DarkHelp::NN::predict_internal() or @ref DarkHelp::NN::predict_tile() depending on how
			 * @ref DarkHelp::Config::enable_tiles has been set.
			 * @param [in] mat A OpenCV2 image which has already been loaded and which needs to be analyzed.  The member
			 * @ref DarkHelp::NN::original_image will be set to this image.  If the image is larger or smaller than the dimensions of
			 * the neural network, then Darknet will stretch the image to match the exact size of the neural network.  Stretching
			 * the image does <em>not</em> maintain the the aspect ratio.
			 * @param [in] new_threshold Which threshold to use.  If less than zero, the previous threshold will be applied.
			 * If >= 0, then @ref DarkHelp::Config::threshold will be set to this new value.  The threshold must be either -1,
			 * or a value between 0.0 and 1.0 meaning 0% to 100%.
			 * @returns A vector of @ref DarkHelp::PredictionResult structures, each one representing a different object in the image.
			 * The higher the threshold value, the more "certain" the network is that it has correctly identified the object.
			 *
			 * @see @ref Tiling
			 * @see @ref DarkHelp::NN::predict_tile()
			 * @see @ref DarkHelp::PredictionResult
			 * @see @ref DarkHelp::Config::enable_tiles
			 * @see @ref DarkHelp::Config::sort_predictions
			 * @see @ref DarkHelp::NN::duration
			 */
			PredictionResults predict(cv::Mat mat, const float new_threshold = -1.0f);

	#ifdef DARKHELP_CAN_INCLUDE_DARKNET
			/** Use the neural network to predict what is contained in this image.    This results in a call to either
			 * @ref DarkHelp::NN::predict_internal() or @ref DarkHelp::NN::predict_tile() depending on how
			 * @ref DarkHelp::Config::enable_tiles has been set.
			 * @param [in] mat A Darknet-style image object which has already been loaded and which needs to be analyzed.
			 * The member @ref DarkHelp::NN::original_image will be set to this image.  If the image is larger or smaller than the
			 * dimensions of the neural network, then Darknet will stretch the image to match the exact size of the neural network.
			 * Stretching the image does <em>not</em> maintain the the aspect ratio.
			 * @param [in] new_threshold Which threshold to use.  If less than zero, the previous threshold will be applied.
			 * If >= 0, then @ref DarkHelp::Config::threshold will be set to this new value.  The threshold must be either -1,
			 * or a value between 0.0 and 1.0 meaning 0% to 100%.
			 * @returns A vector of @ref DarkHelp::NN::PredictionResult structures, each one representing a different object in
			 * the image.  The higher the threshold value, the more "certain" the network is that it has correctly identified the
			 * object.
			 *
			 * @see @ref Tiling
			 * @see @ref DarkHelp::NN::predict_tile()
			 * @see @ref DarkHelp::Config::enable_tiles
			 * @see @ref DarkHelp::PredictionResult
			 * @see @ref DarkHelp::Config::sort_predictions
			 * @see @ref DarkHelp::NN::duration
			 */
			PredictionResults predict(image img, const float new_threshold = -1.0f);
	#endif

			/** Similar to @ref DarkHelp::NN::predict(), but automatically breaks the images down into individual tiles if
			 * it is significantly larger than the network dimensions.  This is explained in details in @ref Tiling.
			 *
			 * @note The method @ref DarkHelp::NN::predict() will @em automatically call @ref DarkHelp::NN::predict_tile() if
			 * necessary <b>when @ref DarkHelp::Config::enable_tiles has been enabled.</b>  If you don't want to use image tiling,
			 * then @ref DarkHelp::Config::enable_tiles must be set to @p false (which is the default).
			 *
			 * Here is a visual representation of a large image broken into 4 tiles for processing by Darknet.  It is important
			 * to understand that neither the individual image tiles nor their results are returned to the caller.  %DarkHelp
			 * only returns the final results once each tile has been processed and the @ref DarkHelp::NN::prediction_results
			 * "vectors" have been merged together.
			 *
			 * @image html mailboxes_2x2_tiles_detection.png
			 *
			 * @see @ref Tiling
			 * @see @ref DarkHelp::NN::predict()
			 */
			PredictionResults predict_tile(cv::Mat mat, const float new_threshold = -1.0f);

			/** Takes the most recent @ref DarkHelp::NN::prediction_results, and applies them to the most recent
			 * @ref DarkHelp::NN::original_image.  The output annotated image is stored in @ref DarkHelp::NN::annotated_image
			 * as well as returned to the caller.
			 *
			 * This is an example of what an annotated image looks like:
			 * @image html barcode_100_percent.png
			 *
			 * @param [in] new_threshold Which threshold to use.  If less than zero, the previous threshold will be applied.
			 * If >= 0, then @ref DarkHelp::Config::threshold will be set to this new value.
			 *
			 * Turning @em down the threshold in @ref DarkHelp::NN::annotate() wont bring back predictions that were excluded due to
			 * a higher threshold originally used with @ref DarkHelp::NN::predict().  Here is an example:
			 *
			 * ~~~~
			 * nn.predict("image.jpg", 0.75);  // note the threshold is set to 75% for prediction
			 * nn.annotate(0.25);              // note the threshold is now modified to be 25%
			 * ~~~~
			 *
			 * In the previous example, when annotate() is called with the lower threshold of 25%, the predictions had already
			 * been capped at 75%.  This means any prediction between >= 25% and < 75% were excluded from the prediction results.
			 * The only way to get those predictions is to re-run predict() with a value of 0.25.
			 *
			 * @note Annotations wont be drawn if @ref DarkHelp::Config::annotation_line_thickness is less than @p 1.
			 *
			 * @see @ref DarkHelp::Config::annotation_colours
			 * @see @ref DarkHelp::Config::annotation_font_scale
			 * @see @ref DarkHelp::Config::annotation_font_thickness
			 * @see @ref DarkHelp::Config::annotation_line_thickness
			 * @see @ref DarkHelp::Config::annotation_include_duration
			 * @see @ref DarkHelp::Config::annotation_include_timestamp
			 * @see @ref DarkHelp::Config::annotation_auto_hide_labels
			 * @see @ref DarkHelp::Config::annotation_shade_predictions
			 * @see @ref DarkHelp::Config::annotation_suppress_classes
			 */
			cv::Mat annotate(const float new_threshold = -1.0f);

			/** Return @ref DarkHelp::NN::duration as a text string which can then be added to the image during annotation.
			 * For example, this might return @p "912 microseconds" or @p "375 milliseconds".
			 * @see @ref DarkHelp::NN::annotate()
			 */
			std::string duration_string();

			/// Determine the size of the network.  For example, 416x416, or 608x480.
			cv::Size network_size();

			/// Return the number of channels defined in the .cfg file.  Usually, this will be @p 3.
			int image_channels();

			/** Snap all the annotations.  This is automatically called from @ref predict() when
			 * @ref DarkHelp::Config::snapping_enabled is set to @p true.  When set to @p false,
			 * you can manually invoke this method to get the annotations to snap, or you can also
			 * manually call @ref DarkHelp::NN::snap_annotation() on specific annotations as needed.
			 *
			 * @note This can be expensive to run depending on the image dimensions, the image threshold
			 * limits, and the amount of "snapping" required for each annotation since the process of "snapping"
			 * is iterative and requires looking for blank spaces within the image.
			 *
			 * @see @ref DarkHelp::Config::snapping_enabled
			 * @see @ref DarkHelp::Config::snapping_horizontal_tolerance
			 *
			 * Image								| Setting
			 * -------------------------------------|--------
			 * @image html snapping_disabled.png ""	| @p snapping_enabled=false
			 * @image html snapping_enabled.png ""	| @p snapping_enabled=true
			 *
			 */
			NN & snap_annotations();

			/** Snap only the given annotation.
			 * @see @ref DarkHelp::NN::snap_annotations()
			 * @see @ref DarkHelp::Config::snapping_enabled
			 * @see @ref DarkHelp::Config::snapping_horizontal_tolerance
			 */
			NN & snap_annotation(PredictionResult & pred);

			/** The Darknet network, but stored as a void* pointer so we don't have to include darknet.h.
			 * This will only be set when the driver is @ref DarkHelp::EDriver::kDarknet in @ref DarkHelp::NN::init().
			 */
			void * darknet_net;

	#ifdef HAVE_OPENCV_DNN_OBJDETECT
			/// The OpenCV network, when the driver has been set to @ref DarkHelp::EDriver::kOpenCV in @ref DarkHelp::NN::init().
			cv::dnn::Net opencv_net;
	#endif

			/** A vector of names corresponding to the identified classes.  This is typically setup in the constructor,
			 * but can be manually set afterwards.
			 */
			VStr names;

			/** The length of time it took to initially load the network and weights (after the %DarkHelp object has been
			 * constructed), or the length of time @ref DarkHelp::NN::predict() took to run on the last image to be processed.
			 * If using @ref DarkHelp::NN::predict_tile(), then this will store the sum of all durations across the entire set
			 * of tiles.  @see @ref DarkHelp::NN::duration_string()
			 */
			std::chrono::high_resolution_clock::duration duration;

			/// A copy of the most recent results after applying the neural network to an image.  This is set by @ref DarkHelp::NN::predict().
			PredictionResults prediction_results;

			/// The most recent image handled by @ref DarkHelp::NN::predict().
			cv::Mat original_image;

			/// The most recent output produced by @ref DarkHelp::NN::annotate().
			cv::Mat annotated_image;

			/// Intended mostly for internal purpose, this is only useful when annotation "snapping" is enabled.
			cv::Mat binary_inverted_image;

			/** The number of horizontal tiles the image was split into by @ref DarkHelp::NN::predict_tile() prior to calling
			 * @ref DarkHelp::NN::predict().  This is set to @p 1 if calling @ref DarkHelp::NN::predict().  It may be &gt; 1
			 * if calling @ref DarkHelp::NN::predict_tile() with an image large enough to require multiple tiles.
			 *
			 * @see @ref DarkHelp::NN::vertical_tiles
			 * @see @ref Tiling
			 * @see @ref DarkHelp::Config::enable_tiles
			 */
			size_t horizontal_tiles;

			/** The number of vertical tiles the image was split into by @ref DarkHelp::NN::predict_tile() prior to calling
			 * @ref DarkHelp::NN::predict().  This is set to @p 1 if calling @ref DarkHelp::NN::predict().  It may be &gt; 1
			 * if calling @ref DarkHelp::NN::predict_tile() with an image large enough to require multiple tiles.
			 *
			 * @see @ref DarkHelp::NN::horizontal_tiles
			 * @see @ref Tiling
			 * @see @ref DarkHelp::Config::enable_tiles
			 */
			size_t vertical_tiles;

			/** The size that was used for each individual tile by @ref DarkHelp::NN::predict_tile().
			 * This will be the size of the network when calling @ref DarkHelp::NN::predict().
			 *
			 * For example, if the network is @p 416x416, and the image used with @ref DarkHelp::NN::predict_tile()
			 * measures @p 1280x960, then:
			 * @li @ref DarkHelp::NN::horizontal_tiles will be set to @p 3
			 * @li @ref DarkHelp::NN::vertical_tiles will be set to @p 2
			 * @li @ref DarkHelp::NN::tile_size will be set to @p "(427, 480)"
			 *
			 * @see @ref Tiling
			 * @see @ref DarkHelp::Config::enable_tiles
			 */
			cv::Size tile_size;

			/** Configuratin for the neural network.  This includes both settings for the neural network itself and everything
			 * needed to annotate images/frames.
			 */
			Config config;

		protected:

			/** Used by all the other @ref DarkHelp::NN::predict() calls to do the actual network prediction.  This uses the
			 * image stored in @ref DarkHelp::NN::original_image.
			 */
			PredictionResults predict_internal(cv::Mat mat, const float new_threshold = -1.0f);

			/// Called from @ref DarkHelp::NN::predict_internal().  @see @ref DarkHelp::NN::predict()
			void predict_internal_darknet();

			/// Called from @ref DarkHelp::NN::predict_internal().  @see @ref DarkHelp::NN::predict()
			void predict_internal_opencv();

			/** Give a consistent name to the given production result.  This gets called by both @ref DarkHelp::NN::predict_internal()
			 * and @ref DarkHelp::NN::predict_tile() and is intended for internal use only.
			 */
			NN & name_prediction(PredictionResult & pred);

			/// Size of the neural network, e.g., @p 416x416 or @p 608x608.  @see @ref DarkHelp::NN::network_size()
			cv::Size network_dimensions;

			/// The number of channels defined in the .cfg file.  This is normally set to @p 3.  @see @ref image_channels()
			int number_of_channels;
	};
}
