/* DarkHelp - C++ helper class for Darknet's C API.
 * Copyright 2019-2020 Stephane Charette <stephanecharette@gmail.com>
 * MIT license applies.  See "license.txt" for details.
 * $Id$
 */

#pragma once

#include <chrono>
#include <string>
#include <map>
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

		/// Constructor.  This constructor automatically calls @ref init().
		DarkHelp(const std::string & cfg_filename, const std::string & weights_filename, const std::string & names_filename = "", const bool verify_files_first = true);

		/** Initialize ("load") the darknet neural network.  If @p verify_files_first has been enabled (the default)
		 * then this method will also call the static method @ref verify_cfg_and_weights() to perform some last-minute
		 * validation prior to darknet loading the neural network.
		 */
		DarkHelp & init(const std::string & cfg_filename, const std::string & weights_filename, const std::string & names_filename = "", const bool verify_files_first = true);

		/// The opposite of @ref init().  This is automatically called by the destructor.
		void reset();

		/** Use the neural network to predict what is contained in this image.
		 * @param [in] image_filename The name of the image file to load from disk and analyze.  The member
		 * @ref original_image will be set to this image.
		 * @param [in] new_threshold Which threshold to use.  If less than zero, the previous threshold will be applied.
		 * If >= 0, then @ref threshold will be set to this new value.  The threshold must be either -1, or a value
		 * between 0.0 and 1.0 meaning 0% to 100%.
		 * @returns A vector of @ref PredictionResult structures, each one representing a different object in the image.
		 * The higher the threshold value, the more "certain" the network is that it has correctly identified the object.
		 * @see @ref PredictionResult
		 * @see @ref sort_predictions
		 * @see @ref duration
		 */
		virtual PredictionResults predict(const std::string & image_filename, const float new_threshold = -1.0f);

		/** Use the neural network to predict what is contained in this image.
		 * @param [in] mat A OpenCV2 image which has already been loaded and which needs to be analyzed.  The member
		 * @ref original_image will be set to this image.
		 * @param [in] new_threshold Which threshold to use.  If less than zero, the previous threshold will be applied.
		 * If >= 0, then @ref threshold will be set to this new value.  The threshold must be either -1, or a value
		 * between 0.0 and 1.0 meaning 0% to 100%.
		 * @returns A vector of @ref PredictionResult structures, each one representing a different object in the image.
		 * The higher the threshold value, the more "certain" the network is that it has correctly identified the object.
		 * @see @ref PredictionResult
		 * @see @ref sort_predictions
		 * @see @ref duration
		 */
		virtual PredictionResults predict(cv::Mat mat, const float new_threshold = -1.0f);

#ifdef DARKHELP_CAN_INCLUDE_DARKNET
		/** Use the neural network to predict what is contained in this image.
		 * @param [in] mat A Darknet-style image object which has already been loaded and which needs to be analyzed.
		 * The member @ref original_image will be set to this image.
		 * @param [in] new_threshold Which threshold to use.  If less than zero, the previous threshold will be applied.
		 * If >= 0, then @ref threshold will be set to this new value.  The threshold must be either -1, or a value
		 * between 0.0 and 1.0 meaning 0% to 100%.
		 * @returns A vector of @ref PredictionResult structures, each one representing a different object in the image.
		 * The higher the threshold value, the more "certain" the network is that it has correctly identified the object.
		 * @see @ref PredictionResult
		 * @see @ref sort_predictions
		 * @see @ref duration
		 */
		virtual PredictionResults predict(image img, const float new_threshold = -1.0f);
#endif

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
		 */
		virtual cv::Mat annotate(const float new_threshold = -1.0f);

		/** Return the @ref duration as a text string which can then be added to the image during annotation.
		 * For example, this might return @p "912 microseconds" or @p "375 milliseconds".
		 * @see @ref annotate()
		 */
		std::string duration_string();

		/** Obtain a vector of several bright colours that may be used to annotate images.
		 * Remember that OpenCV uses BGR, not RGB.  So pure red is @p "{0, 0, 255}".  The
		 * vector returned by this function are intended to be used by OpenCV, and thus are
		 * in BGR format.
		 * @see @ref annotation_colours
		 */
		static VColours get_default_annotation_colours();

		/** Look at the names and/or the contents of all 3 files and swap the filenames around if necessary so the @p .cfg,
		 * @p .weights, and @p .names are assigned where they should be.  This is necessary because darknet tends to segfault
		 * if it is given the wrong filename.  (For example, if it mistakenly tries to parse the @p .weights file as a @p .cfg
		 * file.)  This function does a bit of sanity checking, determines which file is which, and also returns a map of debug
		 * information related to each file.
		 *
		 * On @em input, it doesn't matter which file goes into which parameter.  Simply pass in the filenames you have in any
		 * order.
		 *
		 * On @em output, the @p .cfg, @p .weights, and @p .names will be set correctly.  If needed for display purposes, some
		 * additional information is also passed back using the @p MStr string map, but most callers can ignore this.
		 *
		 * @see @ref init()
		 */
		static MStr verify_cfg_and_weights(std::string & cfg_filename, std::string & weights_filename, std::string & names_filename);

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
		 * constructed), or the length of time @ref predict() took to run on the last image to be processed.
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
		 */
		bool names_include_percentage;

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

	protected:

		/** Used by all the other @ref predict() calls to do the actual network prediction.  This uses the
		 * image stored in @ref original_image.
		 */
		PredictionResults predict(const float new_threshold = -1.0f);
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
 * size will be chosen that does not exceed the specified size.
 *
 * For example, if the image is 640x480, and the specified size is 400x400, the image returned will be 400x300
 * which maintains the original 1.333 aspect ratio.
 */
cv::Mat resize_keeping_aspect_ratio(cv::Mat mat, const cv::Size & desired_size);
