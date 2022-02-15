/* DarkHelp - C++ helper class for Darknet's C API.
 * Copyright 2019-2022 Stephane Charette <stephanecharette@gmail.com>
 * MIT license applies.  See "license.txt" for details.
 */

#pragma once

// Do not include this header file directly.  Instead, your project should include DarkHelp.hpp
// which will then include all of the necessary secondary headers in the correct order.

#include <DarkHelp.hpp>
#include <fstream>

/** @file
 * Classes used to return @ref DarkHelp::NN prediction results to callers.
 */


namespace DarkHelp
{
	/** Map of a class ID to a probability that this object belongs to that class.
	 * The key is the zero-based index of the class, while the value is the probability
	 * that the object belongs to that class.
	 * @see @ref DarkHelp::PredictionResult::all_probabilities
	 */
	using MClassProbabilities = std::map<int, float>;

	/** Structure used to store interesting information on predictions.  A vector of these is created and returned
	 * to the caller every time @ref DarkHelp::NN::predict() is called.  The most recent predictions are also stored
	 * in @ref DarkHelp::NN::prediction_results.
	 */
	struct PredictionResult final
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
		 * @see @ref DarkHelp::PredictionResult::original_point @see @ref DarkHelp::PredictionResult::original_size
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
		 * @see @ref DarkHelp::PredictionResult::rect @see @ref DarkHelp::PredictionResult::original_size
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
		 * @see @ref DarkHelp::PredictionResult::rect @see @ref DarkHelp::PredictionResult::original_point
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
		 * In addition to @p %all_probabilities, the best results will @em also be duplicated in @ref DarkHelp::PredictionResult::best_class
		 * and @ref DarkHelp::PredictionResult::best_probability, which in this example would contain the values representing the truck:
		 *
		 * @li @ref DarkHelp::PredictionResult::best_class == 2
		 * @li @ref DarkHelp::PredictionResult::best_probability == 0.958
		 */
		MClassProbabilities all_probabilities;

		/** The class that obtained the highest probability.  For example, if an object is predicted to be 80% car
		 * or 60% truck, then the class id of the car would be stored in this variable.
		 * @see @ref DarkHelp::PredictionResult::best_probability
		 * @see @ref DarkHelp::PredictionResult::all_probabilities
		 */
		int best_class;

		/** The probability of the class that obtained the highest value.  For example, if an object is predicted to
		 * be 80% car or 60% truck, then the value of 0.80 would be stored in this variable.
		 * @see @ref DarkHelp::PredictionResult::best_class
		 * @see @ref DarkHelp::PredictionResult::all_probabilities
		 */
		float best_probability;

		/** A name to use for the object.  If an object has multiple probabilities, then the one with the highest
		 * probability will be listed first.  For example, a name could be @p "car 80%, truck 60%".  The @p name
		 * is used as a label when calling @ref DarkHelp::NN::annotate().
		 * @see @ref DarkHelp::Config::names_include_percentage
		 */
		std::string name;

		/** The tile number on which this object was found.  This is mostly for debug purposes and only if tiling
		 * has been enabled (see @ref DarkHelp::Config::enable_tiles), otherwise the value will always be zero.
		 */
		int tile;
	};

	/** A vector of predictions for the image analyzed by @ref DarkHelp::NN::predict().
	 * Each @ref DarkHelp::PredictionResult entry in the vector represents a different object in the image.
	 * @see @ref DarkHelp::PredictionResult
	 * @see @ref DarkHelp::NN::prediction_results
	 * @see @ref DarkHelp::Config::sort_predictions
	 */
	using PredictionResults = std::vector<PredictionResult>;
}


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
