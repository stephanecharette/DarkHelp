/* DarkHelp - C++ helper class for Darknet's C API.
 * Copyright 2019-2021 Stephane Charette <stephanecharette@gmail.com>
 * MIT license applies.  See "license.txt" for details.
 */

#pragma once

#include <chrono>
#include <string>
#include <map>
#include <set>
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
 * DarkHelp darkhelp("mynetwork.cfg", "mynetwork.weights", "mynetwork.names");
 *
 * const auto image_filenames = {"image_0.jpg", "image_1.jpg", "image_2.jpg"};
 *
 * for (const auto & filename : image_filenames)
 * {
 *     // these next two lines is where DarkHelp calls into Darknet to do all the hard work
 *     darkhelp.predict(filename);
 *     cv::Mat mat = darkhelp.annotate(); // annotates the most recent image seen by predict()
 *
 *     // use standard OpenCV calls to show the image results in a window
 *     cv::imshow("prediction", mat);
 *     cv::waitKey();
 * }
 * ~~~~
 *
 * Instead of calling @ref annotate(), you can get the detection results and iterate through them:
 *
 * ~~~~
 * DarkHelp darkhelp("mynetwork.cfg", "mynetwork.weights", "mynetwork.names");
 *
 * const auto results = darkhelp.predict("test_image_01.jpg");
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
 * const auto results = darkhelp.predict("test_image_01.jpg");
 * std::cout << results << std::endl;
 * ~~~~
 */
class DarkHelp
{
	public:

		/// Map of strings where both the key and the value are @p std::string.
		typedef std::map<std::string, std::string> MStr;

		/// Vector of text strings.  Typically used to store the class names.
		typedef std::vector<std::string> VStr;

		/// Vector of colours to use by @ref annotate().  @see @ref annotation_colours  @see @ref get_default_annotation_colours()
		typedef std::vector<cv::Scalar> VColours;

		/** Map of a class ID to a probability that this object belongs to that class.
		 * The key is the zero-based index of the class, while the value is the probability
		 * that the object belongs to that class.  @see @ref PredictionResult::all_probabilities
		 */
		typedef std::map<int, float> MClassProbabilities;

		/** Structure used to store interesting information on predictions.  A vector of these is created and returned to the
		 * caller every time @ref predict() is called.  The most recent predictions are also stored in @ref prediction_results.
		 */
		struct PredictionResult
		{
			/** OpenCV rectangle which describes where the object is located in the original image.
			 *
			 * Given this example annotated 230x134 image:
			 * @image html xkcd_bike.png
			 * The red rectangle returned would be:
			 * @li @p rect.x = 96 (top left)
			 * @li @p rect.y = 38 (top left)
			 * @li @p rect.width = 108
			 * @li @p rect.height = 87
			 *
			 * @see @ref original_point @see @ref original_size
			 */
			cv::Rect rect;

			/** The original normalized X and Y coordinate returned by darknet.  This is the normalized mid-point, not the corner.
			 * If in doubt, you probably want to use @p rect.x and @p rect.y instead of this value.
			 *
			 * Given this example annotated 230x134 image:
			 * @image html xkcd_bike.png
			 * The @p original_point returned would be:
			 * @li @p original_point.x = 0.652174 (mid x / image width, or 150 / 230)
			 * @li @p original_point.y = 0.608209 (mid y / image height, or 81.5 / 134)
			 *
			 * @see @ref rect @see @ref original_size
			 */
			cv::Point2f original_point;

			/** The original normalized width and height returned by darknet.  If in doubt, you probably want to use
			 * @p rect.width and @p rect.height instead of this value.
			 *
			 * Given this example annotated 230x134 image:
			 * @image html xkcd_bike.png
			 * The @p original_size returned would be:
			 * @li @p original_size.width  = 0.469565 (rect width / image width, or 108 / 230)
			 * @li @p original_size.height = 0.649254 (rect height / image height, or 87 / 134)
			 *
			 * @see @ref rect @see @ref original_point
			 */
			cv::Size2f original_size;

			/** This is only useful if you have multiple classes, and an object may be one of several possible classes.
			 *
			 * @note This will contain all @em non-zero class/probability pairs.
			 *
			 * For example, if your classes in your @p names file are defined like this:
			 * ~~~~{.txt}
			 * car
			 * person
			 * truck
			 * bus
			 * ~~~~
			 *
			 * Then an image of a truck may be 10.5% car, 0% person, 95.8% truck, and 60.3% bus.  Only the non-zero
			 * values are ever stored in this map, which for this example would be the following:
			 *
			 * @li 0 -> 0.105 // car
			 * @li 2 -> 0.958 // truck
			 * @li 3 -> 0.603 // bus
			 *
			 * The C++ map would contains the following values:
			 *
			 * ~~~~
			 * all_probabilities = { {0, 0.105}, {2, 0.958}, {3, 0.603} };
			 * ~~~~
			 *
			 * (Note how @p person is not stored in the map, since the probability for that class is 0%.)
			 *
			 * In addition to @p %all_probabilities, the best results will @em also be duplicated in @ref best_class
			 * and @ref best_probability, which in this example would contain the values representing the truck:
			 *
			 * @li @ref best_class == 2
			 * @li @ref best_probability == 0.958
			 */
			MClassProbabilities all_probabilities;

			/** The class that obtained the highest probability.  For example, if an object is predicted to be 80% car
			 * or 60% truck, then the class id of the car would be stored in this variable.
			 * @see @ref best_probability
			 * @see @ref all_probabilities
			 */
			int best_class;

			/** The probability of the class that obtained the highest value.  For example, if an object is predicted to
			 * be 80% car or 60% truck, then the value of 0.80 would be stored in this variable.
			 * @see @ref best_class
			 * @see @ref all_probabilities
			 */
			float best_probability;

			/** A name to use for the object.  If an object has multiple probabilities, then the one with the highest
			 * probability will be listed first.  For example, a name could be @p "car 80%, truck 60%".  The @p name
			 * is used as a label when calling @ref annotate().  @see @ref names_include_percentage
			 */
			std::string name;

			/** The tile number on which this object was found.  This is mostly for debug purposes and only tiling has
			 * been enabled (see @ref DarkHelp::enable_tiles), otherwise the value will always be zero.
			 */
			int tile;
		};

		/** A vector of predictions for the image analyzed by @ref predict().  Each @ref PredictionResult entry in the
		 * vector represents a different object in the image.
		 * @see @ref PredictionResult
		 * @see @ref prediction_results
		 * @see @ref sort_predictions
		 */
		typedef std::vector<PredictionResult> PredictionResults;

		/// Destructor.  This automatically calls reset() to release memory allocated by the neural network.
		virtual ~DarkHelp();

		/// Constructor.  When using this constructor, the neural network remains uninitialized until @ref init() is called.
		DarkHelp();

		/** Constructor.  This constructor automatically calls @ref init().
		 *
		 * @note The order in which you pass the various filenames is @em not important if @p verify_files_first is set to
		 * @p true (the default value).  This is because @ref init() will call @ref verify_cfg_and_weights() to correctly
		 * determine which is the @p .cfg, @p .weights, and @p .names file, and swap the names around as necessary so Darknet
		 * is given the correct filenames.
		 */
		DarkHelp(const std::string & cfg_filename, const std::string & weights_filename, const std::string & names_filename = "", const bool verify_files_first = true);

		/// Get a version string for the DarkHelp library.  E.g., could be `1.0.0-123`.
		virtual std::string version() const;

		/** Initialize ("load") the darknet neural network.  If @p verify_files_first has been enabled (the default)
		 * then this method will also call the static method @ref verify_cfg_and_weights() to perform some last-minute
		 * validation prior to darknet loading the neural network.
		 */
		virtual DarkHelp & init(const std::string & cfg_filename, const std::string & weights_filename, const std::string & names_filename = "", const bool verify_files_first = true);

		/// The opposite of @ref init().  This is automatically called by the destructor.
		virtual void reset();

		/** Use the neural network to predict what is contained in this image.  This results in a call to either
		 * @ref predict_internal() or @ref predict_tile() depending on how @ref enable_tiles has been set.
		 * @param [in] image_filename The name of the image file to load from disk and analyze.  The member
		 * @ref original_image will be set to this image.  If the image is larger or smaller than the dimensions of the neural
		 * network, then Darknet will stretch the image to match the exact size of the neural network.  Stretching the image
		 * does <em>not</em> maintain the the aspect ratio.
		 * @param [in] new_threshold Which threshold to use.  If less than zero, the previous threshold will be applied.
		 * If >= 0, then @ref threshold will be set to this new value.  The threshold must be either -1, or a value
		 * between 0.0 and 1.0 meaning 0% to 100%.
		 * @returns A vector of @ref PredictionResult structures, each one representing a different object in the image.
		 * The higher the threshold value, the more "certain" the network is that it has correctly identified the object.
		 *
		 * @see @ref Tiling
		 * @see @ref predict_tile()
		 * @see @ref enable_tiles
		 * @see @ref PredictionResult
		 * @see @ref sort_predictions
		 * @see @ref duration
		 */
		virtual PredictionResults predict(const std::string & image_filename, const float new_threshold = -1.0f);

		/** Use the neural network to predict what is contained in this image.  This results in a call to either
		 * @ref predict_internal() or @ref predict_tile() depending on how @ref enable_tiles has been set.
		 * @param [in] mat A OpenCV2 image which has already been loaded and which needs to be analyzed.  The member
		 * @ref original_image will be set to this image.  If the image is larger or smaller than the dimensions of the neural
		 * network, then Darknet will stretch the image to match the exact size of the neural network.  Stretching the image
		 * does <em>not</em> maintain the the aspect ratio.
		 * @param [in] new_threshold Which threshold to use.  If less than zero, the previous threshold will be applied.
		 * If >= 0, then @ref threshold will be set to this new value.  The threshold must be either -1, or a value
		 * between 0.0 and 1.0 meaning 0% to 100%.
		 * @returns A vector of @ref PredictionResult structures, each one representing a different object in the image.
		 * The higher the threshold value, the more "certain" the network is that it has correctly identified the object.
		 *
		 * @see @ref Tiling
		 * @see @ref predict_tile()
		 * @see @ref enable_tiles
		 * @see @ref PredictionResult
		 * @see @ref sort_predictions
		 * @see @ref duration
		 */
		virtual PredictionResults predict(cv::Mat mat, const float new_threshold = -1.0f);

#ifdef DARKHELP_CAN_INCLUDE_DARKNET
		/** Use the neural network to predict what is contained in this image.    This results in a call to either
		 * @ref predict_internal() or @ref predict_tile() depending on how @ref enable_tiles has been set.
		 * @param [in] mat A Darknet-style image object which has already been loaded and which needs to be analyzed.
		 * The member @ref original_image will be set to this image.  If the image is larger or smaller than the dimensions
		 * of the neural network, then Darknet will stretch the image to match the exact size of the neural network.
		 * Stretching the image does <em>not</em> maintain the the aspect ratio.
		 * @param [in] new_threshold Which threshold to use.  If less than zero, the previous threshold will be applied.
		 * If >= 0, then @ref threshold will be set to this new value.  The threshold must be either -1, or a value
		 * between 0.0 and 1.0 meaning 0% to 100%.
		 * @returns A vector of @ref PredictionResult structures, each one representing a different object in the image.
		 * The higher the threshold value, the more "certain" the network is that it has correctly identified the object.
		 *
		 * @see @ref Tiling
		 * @see @ref predict_tile()
		 * @see @ref enable_tiles
		 * @see @ref PredictionResult
		 * @see @ref sort_predictions
		 * @see @ref duration
		 */
		virtual PredictionResults predict(image img, const float new_threshold = -1.0f);
#endif

		/** Similar to @ref predict(), but automatically breaks the images down into individual tiles if it is significantly
		 * larger than the network dimensions.  This is explained in details in @ref Tiling.
		 *
		 * @note The method @ref predict() will @em automatically call @ref predict_tile() if necessary <b>when
		 * @ref enable_tiles has been enabled.</b>  If you don't want to use image tiling, then @ref enable_tiles
		 * must be set to @p false (which is the default).
		 *
		 * Here is a visual representation of a large image broken into 4 tiles for processing by Darknet.  It is important
		 * to understand that neither the individual image tiles nor their results are returned to the caller.  %DarkHelp
		 * only returns the final results once each tile has been processed and the @ref prediction_results "vectors"
		 * have been merged together.
		 *
		 * @image html mailboxes_2x2_tiles_detection.png
		 *
		 * @see @ref Tiling
		 * @see @ref predict()
		 */
		virtual PredictionResults predict_tile(cv::Mat mat, const float new_threshold = -1.0f);

		/** Takes the most recent @ref prediction_results, and applies them to the most recent @ref original_image.
		 * The output annotated image is stored in @ref annotated_image as well as returned to the caller.
		 *
		 * This is an example of what an annotated image looks like:
		 * @image html barcode_100_percent.png
		 *
		 * @param [in] new_threshold Which threshold to use.  If less than zero, the previous threshold will be applied.
		 * If >= 0, then @ref threshold will be set to this new value.
		 *
		 * Turning @em down the threshold in @ref annotate() wont bring back predictions that were excluded due to a
		 * higher threshold originally used with @ref predict().  Here is an example:
		 *
		 * ~~~~
		 * darkhelp.predict("image.jpg", 0.75);  // note the threshold is set to 75% for prediction
		 * darkhelp.annotate(0.25);              // note the threshold is now modified to be 25%
		 * ~~~~
		 *
		 * In the previous example, when annotate() is called with the lower threshold of 25%, the predictions had already
		 * been capped at 75%.  This means any prediction between >= 25% and < 75% were excluded from the prediction results.
		 * The only way to get those predictions is to re-run predict() with a value of 0.25.
		 *
		 * @note Annotations wont be drawn if @ref annotation_line_thickness is less than @p 1.
		 *
		 * @see @ref annotation_colours
		 * @see @ref annotation_font_scale
		 * @see @ref annotation_font_thickness
		 * @see @ref annotation_line_thickness
		 * @see @ref annotation_include_duration
		 * @see @ref annotation_include_timestamp
		 * @see @ref annotation_auto_hide_labels
		 * @see @ref annotation_shade_predictions
		 * @see @ref annotation_suppress_classes
		 */
		virtual cv::Mat annotate(const float new_threshold = -1.0f);

		/** Return the @ref duration as a text string which can then be added to the image during annotation.
		 * For example, this might return @p "912 microseconds" or @p "375 milliseconds".
		 * @see @ref annotate()
		 */
		virtual std::string duration_string();

		/// Determine the size of the network.  For example, 416x416, or 608x480.
		virtual cv::Size network_size();

		/** Obtain a vector of at least 25 different bright colours that may be used to annotate images.  OpenCV uses BGR, not RGB.
		 * For example:
		 *
		 * @li @p "{0, 0, 255}" is pure red
		 * @li @p "{255, 0, 0}" is pure blue
		 *
		 * The colours returned by this function are intended to be used by OpenCV, and thus are in BGR format.
		 * @see @ref annotation_colours
		 *
		 * Default colours returned by this method are:
		 *
		 * Index	| RGB Hex	| Name
		 * ---------|-----------|-----
		 * 0		| @p FF355E	| <span style="background-color: #FF355E">Radical Red</span>
		 * 1		| @p 299617	| <span style="background-color: #299617">Slimy Green</span>
		 * 2		| @p FFCC33	| <span style="background-color: #FFCC33">Sunglow</span>
		 * 3		| @p AF6E4D	| <span style="background-color: #AF6E4D">Brown Sugar</span>
		 * 4		| @p FF00FF	| <span style="background-color: #FF00FF">Pure magenta</span>
		 * 5		| @p 50BFE6	| <span style="background-color: #50BFE6">Blizzard Blue</span>
		 * 6		| @p CCFF00	| <span style="background-color: #CCFF00">Electric Lime</span>
		 * 7		| @p 00FFFF	| <span style="background-color: #00FFFF">Pure cyan</span>
		 * 8		| @p 8D4E85	| <span style="background-color: #8D4E85">Razzmic Berry</span>
		 * 9		| @p FF48CC	| <span style="background-color: #FF48CC">Purple Pizzazz</span>
		 * 10		| @p 00FF00	| <span style="background-color: #00FF00">Pure green</span>
		 * 11		| @p FFFF00	| <span style="background-color: #FFFF00">Pure yellow</span>
		 * 12		| @p 5DADEC	| <span style="background-color: #5DADEC">Blue Jeans</span>
		 * 13		| @p FF6EFF	| <span style="background-color: #FF6EFF">Shocking Pink</span>
		 * 14		| @p AAF0D1	| <span style="background-color: #AAF0D1">Magic Mint</span>
		 * 15		| @p FFC000	| <span style="background-color: #FFC000">Orange</span>
		 * 16		| @p 9C51B6	| <span style="background-color: #9C51B6">Purple Plum</span>
		 * 17		| @p FF9933	| <span style="background-color: #FF9933">Neon Carrot</span>
		 * 18		| @p 66FF66	| <span style="background-color: #66FF66">Screamin' Green</span>
		 * 19		| @p FF0000	| <span style="background-color: #FF0000">Pure red</span>
		 * 20		| @p 4B0082	| <span style="background-color: #4B0082">Indigo</span>
		 * 21		| @p FF6037	| <span style="background-color: #FF6037">Outrageous Orange</span>
		 * 22		| @p FFFF66	| <span style="background-color: #FFFF66">Laser Lemon</span>
		 * 23		| @p FD5B78	| <span style="background-color: #FD5B78">Wild Watermelon</span>
		 * 24		| @p 0000FF	| <span style="background-color: #0000FF">Pure blue</span>
		 */
		static VColours get_default_annotation_colours();

		/** Look at the names and/or the contents of all 3 files and swap the filenames around if necessary so the @p .cfg,
		 * @p .weights, and @p .names are assigned where they should be.  This is necessary because darknet tends to segfault
		 * if it is given the wrong filename.  (For example, if it mistakenly tries to parse the @p .weights file as a @p .cfg
		 * file.)  This function does a bit of sanity checking, determines which file is which, and also returns a map of debug
		 * information related to each file.
		 *
		 * On @em input, it doesn't matter which file goes into which parameter.  Simply pass in the filenames in any order.
		 *
		 * On @em output, the @p .cfg, @p .weights, and @p .names will be set correctly.  If needed for display purposes, some
		 * additional information is also passed back using the @p MStr string map, but most callers should ignore this.
		 *
		 * @see @ref init()
		 */
		static MStr verify_cfg_and_weights(std::string & cfg_filename, std::string & weights_filename, std::string & names_filename);

		/** This is used to insert lines into the @p [net] section of the configuration file.  Pass in a map of key-value pairs,
		 * and if the key exists it will be modified.  If the key does not exist, then it will be added to the bottom of the
		 * @p [net] section.
		 *
		 * For example, this is used by @ref init() when @ref modify_batch_and_subdivisions is enabled.
		 *
		 * @returns The number of lines that were modified or had to be inserted into the configuration file.
		 */
		static size_t edit_cfg_file(const std::string & cfg_filename, MStr m);

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

		/// The Darknet network.  This is setup in the constructor.
		network * net;
#else
		/// The Darknet network, but stored as a void* pointer so we don't have to include darknet.h.
		void * net;
#endif

		/** A vector of names corresponding to the identified classes.  This is typically setup in the constructor,
		 * but can be manually set afterwards.
		 */
		VStr names;

		/** The length of time it took to initially load the network and weights (after the %DarkHelp object has been
		 * constructed), or the length of time @ref predict() took to run on the last image to be processed.  If using
		 * @ref predict_tile(), then this will store the sum of all durations across the entire set of tiles.
		 * @see @ref duration_string()
		 */
		std::chrono::high_resolution_clock::duration duration;

		/** Image prediction threshold.  Defaults to @p 0.5.  @see @ref predict()  @see @ref annotate()
		 *
		 * Quote: <blockquote> [...] threshold is what is used to determine whether or not there is an object in the predicted
		 * bounding box. The network predicts an explicit 'objectness' score separate from the class predictions that if above
		 * the threshold indicates that a bounding box will be returned.
		 * [<a href="https://github.com/philipperemy/yolo-9000/issues/3#issuecomment-304208297">source</a>]
		 * </blockquote>
		 */
		float threshold;

		/** Used during prediction.  Defaults to @p 0.5.  @see @ref predict()
		 *
		 * Quote: <blockquote> [...] the network traverses the tree of candidate detections and multiples through the conditional
		 * probabilities for each item, e.g. object * animal * feline * house cat. The hierarchical threshold is used in this
		 * second step, completely after and separate from whether there is an item or not, to decide whether following the
		 * tree further to a more specific class is the right action to take. When this threshold is 0, the tree will basically
		 * follow the highest probability branch all the way to a leaf node.
		 * [<a href="https://github.com/philipperemy/yolo-9000/issues/3#issuecomment-304208297">source</a>]
		 * </blockquote>
		 *
		 * @note This variable used to be named @p hierchy_threshold.  The typo in the name was fixed in December 2019.
		 */
		float hierarchy_threshold;

		/** Non-Maximal Suppression (NMS) threshold suppresses overlapping bounding boxes and only retains the bounding
		 * box that has the maximum probability of object detection associated with it.  Defaults to @p 0.45.
		 * @see @ref predict()
		 *
		 * Quote: <blockquote> [...] nms works by looking at all bounding boxes that made it past the 'objectness' threshold
		 * and removes the least confident ​of the boxes that overlap with each other above a certain IOU threshold
		 * [<a href="https://github.com/philipperemy/yolo-9000/issues/3#issuecomment-304208297">source</a>]
		 * </blockquote>
		 *
		 * (IOU -- "intersection over union" -- is a ratio that describes how much two areas overlap, where 0.0 means two
		 * areas don't overlap at all, and 1.0 means two areas perfectly overlap.)
		 */
		float non_maximal_suppression_threshold;

		/// A copy of the most recent results after applying the neural network to an image.  This is set by @ref predict().
		PredictionResults prediction_results;

		/** Determines if the name given to each prediction includes the percentage.
		 *
		 * For example, the name for a prediction might be @p "dog" when this flag is set to @p false, or it might be
		 * @p "dog 98%" when set to @p true.  Defaults to @p true.
		 *
		 * Examples:
		 *
		 * Setting								| Image
		 * -------------------------------------|------
		 * @p names_include_percentage=true		| @image html include_percentage_true.png
		 * @p names_include_percentage=false	| @image html include_percentage_false.png
		 */
		bool names_include_percentage;

		/** Hide the label if the size of the text exceeds the size of the prediction.  This can help "clean up" some
		 * images which contain many small objects.  Set to @p false to always display every label.  Set to @p true if
		 * %DarkHelp should decide whether a label must be shown or hidden.  Defaults to @p true.
		 * @since 2020-07-03
		 *
		 * Examples:
		 *
		 * Setting						| Image
		 * -----------------------------|------
		 * @p auto_hide_labels=true		| @image html auto_hide_labels_true.png
		 * @p auto_hide_labels=false	| @image html auto_hide_labels_false.png
		 */
		bool annotation_auto_hide_labels;

		/** Determines the amount of "shade" used when drawing the prediction rectangles.  When set to zero, the rectangles
		 * are not shaded.  When set to 1.0, prediction recangles are completely filled.  Values in between are semi-transparent.
		 * For example, the default value of 0.25 means the rectangles are filled at 25% opacity.
		 * @since 2020-07-03
		 *
		 * Examples:
		 *
		 * Setting						| Image
		 * -----------------------------|------
		 * @p shade_predictions=0.0		| @image html shade_0pcnt.png
		 * @p shade_predictions=0.25	| @image html shade_25pcnt.png
		 * @p shade_predictions=0.50	| @image html shade_50pcnt.png
		 * @p shade_predictions=0.75	| @image html shade_75pcnt.png
		 * @p shade_predictions=1.0		| @image html shade_100pcnt.png
		 */
		float annotation_shade_predictions;

		/** Determine if multiple class names are included when labelling an item.
		 *
		 * For example, if an object is 95% car or 80% truck, then the label could say @p "car, truck"
		 * when this is set to @p true, and simply @p "car" when set to @p false.  Defaults to @p true.
		 */
		bool include_all_names;

		/** The colours to use in @ref annotate().  Defaults to @ref get_default_annotation_colours().
		 *
		 * Remember that OpenCV uses BGR, not RGB.  So pure red is @p "(0, 0, 255)".
		 */
		VColours annotation_colours;

		/// Font face to use in @ref annotate().  Defaults to @p cv::HersheyFonts::FONT_HERSHEY_SIMPLEX.
		cv::HersheyFonts annotation_font_face;

		/// Scaling factor used for the font in @ref annotate().  Defaults to @p 0.5.
		double annotation_font_scale;

		/// Thickness of the font in @ref annotate().  Defaults to @p 1.
		int annotation_font_thickness;

		/// Thickness of the lines to draw in @ref annotate().  Defaults to @p 2.
		int annotation_line_thickness;

		/** If set to @p true then @ref annotate() will call @ref duration_string() and display on the top-left of the
		 * image the length of time @ref predict() took to process the image.  Defaults to @p true.
		 *
		 * When enabed, the duration may look similar to this:
		 * @image html barcode_100_percent.png
		 */
		bool annotation_include_duration;

		/** If set to @p true then @ref annotate() will display a timestamp on the bottom-left corner of the image.
		 * Defaults to @p false.
		 *
		 * When enabled, the timestamp may look similar to this:
		 * @image html barcode_with_timestamp.png
		 */
		bool annotation_include_timestamp;

		/** Darknet sometimes will return values that are out-of-bound, especially when working with low thresholds.
		 * For example, the @p X or @p Y coordinates might be less than zero, or the @p width and @p height might extend
		 * beyond the edges of the image.  When @p %fix_out_of_bound_values is set to @p true (the default) then the
		 * results (@ref prediction_results) after calling @ref predict() will be capped so all values are positive and
		 * do not extend beyond the edges of the image.  When set to @p false, the exact values as returned by darknet
		 * will be used.  Defaults to @p true.
		 */
		bool fix_out_of_bound_values;

		/// The most recent image handled by @ref predict().
		cv::Mat original_image;

		/// The most recent output produced by @ref annotate().
		cv::Mat annotated_image;

		/// @see @ref sort_predictions
		enum class ESort
		{
			kUnsorted,		///< Do not sort predictions.
			kAscending,		///< Sort predictions using @ref PredictionResult::best_probability in ascending order (low values first, high values last).
			kDescending		///< Sort predictions using @ref PredictionResult::best_probability in descending order (high values first, low values last).
		};

		/** Determines if the predictions will be sorted the next time @ref predict() is called.  When set to
		 * @ref ESort::kUnsorted, the predictions are in the exact same order as they were returned by Darknet.  When
		 * set to @ref ESort::kAscending or @ref ESort::kDescending, the predictions will be sorted according to
		 * @ref PredictionResult::best_probability.
		 *
		 * If annotations will be drawn on the image for visual consumption, then it is often preferable to have the higher
		 * probability predictions drawn last so they appear "on top".  Otherwise, lower probability predictions may overwrite
		 * or obscure the more important ones.  This means using @ref ESort::kAscending (the default).
		 *
		 * If you want to process only the first few predictions instead of drawing annotations, then you may want to sort
		 * using @ref ESort::kDescending to ensure you handle the most likely predictions first.
		 *
		 * Defaults to @ref ESort::kAscending.
		 */
		ESort sort_predictions;

		/** This enables some non-specific debug functionality within the DarkHelp library.  The exact results of enabling
		 * this is undocumented, and will change or may be completely removed without prior notice.  It is not meant for the
		 * end-user, but instead is used for developers debugging DarkHelp and Darknet.  Default value is @p false.
		 */
		bool enable_debug;

		/** Determines if calls to @ref predict() are sent directly to Darknet, or processed first by @ref predict_tile()
		 * to break the image file into smaller sections.
		 *
		 * This flag is only checked when @ref predict() is called.  If you call @ref predict_tile() directly, then it
		 * bypasses the check for @p enable_tiles and %DarkHelp will assume that the image is a candidate for tiling.
		 *
		 * @note Only images which are much larger than the network dimensions will be considered for tiles.  If an image is
		 * <b>less than</b> approximately 1.5 times the size of the network, then a 1x1 tile (meaning no tiling) will be used.
		 *
		 * Both @ref predict() and @ref predict_tile() will set the values @ref tile_size, @ref vertical_tiles, and
		 * @ref horizontal_tiles once they have finished running.  The caller can then reference these to determine what
		 * kind of tiling was used.  Even when an image is not tiled, these variables will be set; for example, @ref tile_size
		 * may be set to 1x1, and the horizontal and vertical sizes will match the neural network dimensions.
		 *
		 * The default value for @p enable_tiles is @p false, meaning that calling @ref predict() wont automatically result
		 * in image tiling.
		 *
		 * @see @ref Tiling
		 * @see @ref combine_tile_predictions
		 * @see @ref horizontal_tiles
		 * @see @ref vertical_tiles
		 * @see @ref tile_size
		 */
		bool enable_tiles;

		/** The number of horizontal tiles the image was split into by @ref predict_tile() prior to calling @ref predict().
		 * This is set to @p 1 if calling @ref predict().  It may be &gt; 1 if calling @ref predict_tile() with an image
		 * large enough to require multiple tiles.  @see @ref vertical_tiles
		 *
		 * @see @ref Tiling
		 * @see @ref enable_tiles
		 */
		size_t horizontal_tiles;

		/** The number of vertical tiles the image was split into by @ref predict_tile() prior to calling @ref predict().
		 * This is set to @p 1 if calling @ref predict().  It may be &gt; 1 if calling @ref predict_tile() with an image
		 * large enough to require multiple tiles.  @see @ref horizontal_tiles
		 *
		 * @see @ref Tiling
		 * @see @ref enable_tiles
		 */
		size_t vertical_tiles;

		/** The size that was used for each individual tile by @ref predict_tile().  This will be the size of the network
		 * when calling @ref predict().
		 *
		 * For example, if the network is @p 416x416, and the image used with @ref predict_tile() measures @p 1280x960, then:
		 * @li @ref horizontal_tiles will be set to @p 3
		 * @li @ref vertical_tiles will be set to @p 2
		 * @li @ref tile_size will be set to @p "(427, 480)"
		 *
		 * @see @ref Tiling
		 * @see @ref enable_tiles
		 */
		cv::Size tile_size;

		/** When training, the @p "batch=..." and @p "subdivisions=..." values in the .cfg file are typically set to a large
		 * value.  But when loading a neural network for inference as %DarkHelp is designed to help with, @em both of those
		 * values in the .cfg should be set to @p "1".  When @p modify_batch_and_subdivisions is enabled, %DarkHelp will edit
		 * the configuration file once @ref DarkHelp::init() is called.  This ensures the values are set as needed prior to
		 * Darknet loading the .cfg file.
		 *
		 * The default value for @p modify_batch_and_subdivisions is @p true, meaning the .cfg file will be modified.  If set
		 * to @p false, %DarkHelp will not modify the configuration file.
		 *
		 * Example use:
		 *
		 * ~~~~
		 * DarkHelp dh;
		 * dh.modify_batch_and_subdivisions = true;
		 * dh.init("cars.cfg", "cars_best.weights", "cars.names");
		 * ~~~~
		 *
		 * @since Starting with version 1.1.9 on 2021-03-02, this was modified to also toggle the new
		 * @p use_cuda_graph=1 flag in the @p [net] section.
		 * See <a target="_blank" href="https://github.com/AlexeyAB/darknet/issues/7444">issue #7444</a> for details.
		 */
		bool modify_batch_and_subdivisions;

		/** Determines which classes to suppress during the call to @ref annotate().  Any prediction returned by Darknet for
		 * a class listed in this @p std::set will be ignored:  no bounding box will be drawn, and no label will be shown.
		 * The set may be modified at any point and will take effect the next time @ref annotate() is called.
		 *
		 * It is initialized by @ref init() to contain any classes where the label name begins with the text @p "dont_show"
		 * as described in https://github.com/AlexeyAB/darknet/issues/2122.
		 *
		 * For example, when considering this annotated image:
		 *
		 * @image html mailboxes.png
		 *
		 * If the @p .names file is modified in this manner:
		 *
		 * ~~~~{.txt}
		 * lock
		 * 1
		 * dont_show 2
		 * 3
		 * 4
		 * ...
		 * ~~~~
		 *
		 * Then the annotated image will look like this:
		 *
		 * @image html mailboxes_suppress.png
		 *
		 * @note This does not suppress the @em detection of classes.  The vector returned when calling @ref predict() will
		 * contain all of the objects found by Darknet, regardless of what classes are listed in @p annotation_suppress_classes.
		 */
		std::set<int> annotation_suppress_classes;

		/** When tiling is enabled, objects may span multiple tiles.  When this flag is set to @p true, %DarkHelp will attempt
		 * to combine predictions that cross two or more tiles into a single prediction.  This has no impact when tiling is off,
		 * or when the image processed fits within a single tile.  Default is @p true.
		 *
		 * @see @ref enable_tiles
		 * @see @ref tile_edge_factor
		 * @see @ref tile_rect_factor
		 *
		 * Image								| Description
		 * -------------------------------------|------------
		 * @image html tile_combine_1.png ""	| For example, the image on the left is a portion of a much larger image.
		 * @image html tile_combine_2.png ""	| The blue horizontal line in this image shows the location of the boundary between two image tiles.
		 * @image html tile_combine_3.png ""	| When @p combine_tile_predictions=false, the predictions which are split by a tile boundary look like this.
		 * @image html tile_combine_4.png ""	| When @p combine_tile_predictions=true, the predictions split by a tile boundary are re-combined into a single object.
		 */
		bool combine_tile_predictions;

		/** This value controls how close to the edge of a tile an object must be to be considered for re-combining when both
		 * tiling and recombining have been enabled.  The smaller the value, the closer the object must be to the edge of a
		 * tile.  The factor is multiplied by the width and height of the detected object.
		 *
		 * Possible range to consider would be @p 0.01 to @p 0.5.  If set to zero, then the detected object must be right on the
		 * tile boundary to be considered.  The default is @p 0.25.
		 *
		 * @see @ref enable_tiles
		 * @see @ref combine_tile_predictions
		 */
		float tile_edge_factor;

		/** This value controls how close the rectangles needs to line up on two tiles before the predictions are combined.
		 * This is only used when both tiling and recombining have been enabled.  As the value approaches @p 1.0, the closer
		 * the rectangles have to be "perfect" to be combined.  Values below @p 1.0 are impossible, and predictions will
		 * never be combined.  Possible range to consider might be @p 1.10 to @p 1.50 or even higher; this also depends on
		 * the nature/shape of the objects detected and how the tiles are split up.  For example, if the objects are pear-shaped,
		 * where the smaller end is on one tile and the larger end on another tile, you may need to increase this value as the
		 * object on the different tiles will be of different sizes.  The default is @p 1.20.
		 *
		 * @see @ref enable_tiles
		 * @see @ref combine_tile_predictions
		 */
		float tile_rect_factor;

	protected:

		/** Used by all the other @ref predict() calls to do the actual network prediction.  This uses the
		 * image stored in @ref original_image.
		 */
		PredictionResults predict_internal(cv::Mat mat, const float new_threshold = -1.0f);

		/** Give a consistent name to the given production result.  This gets called by both @ref predict_internal() and
		 * @ref predict_tile() and is intended for internal use only.
		 */
		DarkHelp & name_prediction(PredictionResult & pred);
};


/** Convenience function to stream a single result as a "readable" line of text.
 * Mostly intended for debug or logging purposes.
 */
std::ostream & operator<<(std::ostream & os, const DarkHelp::PredictionResult & pred);


/** Convenience function to stream an entire vector of results as readable text.
 * Mostly intended for debug or logging purposes.
 *
 * For example:
 *
 * ~~~~
 * DarkHelp darkhelp("mynetwork.cfg", "mynetwork.weights", "mynetwork.names");
 * const auto results = darkhelp.predict("test_image_01.jpg");
 * std::cout << results << std::endl;
 * ~~~~
 *
 * This would generate text similar to this:
 *
 * ~~~~{.txt}
 * prediction results: 12
 * -> 1/12: "Barcode 94%" #43 prob=0.939646 x=430 y=646 w=173 h=17 entries=1
 * -> 2/12: "Tag 100%" #40 prob=0.999954 x=366 y=320 w=281 h=375 entries=1
 * -> 3/12: "G 85%, 2 12%" #19 prob=0.846418 x=509 y=600 w=28 h=37 entries=2 [ 2=0.122151 19=0.846418 ]
 * ...
 * ~~~~
 *
 * Where:
 *
 * @li @p "1/12" is the number of predictions found.
 * @li @p "Barcode 94%" is the class name and the probability if @ref DarkHelp::names_include_percentage is enabled.
 * @li @p "#43" is the zero-based class index.
 * @li @p "prob=0.939646" is the probabilty that it is class #43.  (Multiply by 100 to get percentage.)
 * @li @p "x=..." are the X, Y, width, and height of the rectangle that was identified.
 * @li @p "entries=1" means that only 1 class was matched.  If there is more than 1 possible class,
 * then the class index and probability for each class will be shown.
 */
std::ostream & operator<<(std::ostream & os, const DarkHelp::PredictionResults & results);


/** Convenience function to resize an image yet retain the exact original aspect ratio.  Performs no resizing if the
 * image is already the desired size.  Depending on the size of the original image and the desired size, a "best"
 * size will be chosen that <b>does not exceed</b> the specified size.  No letterboxing will be performed.
 *
 * For example, if the image is 640x480, and the specified size is 400x400, the image returned will be 400x300
 * which maintains the original 1.333 aspect ratio.
 */
cv::Mat resize_keeping_aspect_ratio(cv::Mat mat, const cv::Size & desired_size);
