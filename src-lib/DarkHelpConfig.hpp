/* DarkHelp - C++ helper class for Darknet's C API.
 * Copyright 2019-2022 Stephane Charette <stephanecharette@gmail.com>
 * MIT license applies.  See "license.txt" for details.
 */

#pragma once

// Do not include this header file directly.  Instead, your project should include DarkHelp.hpp
// which will then include all of the necessary secondary headers in the correct order.

#include <DarkHelp.hpp>

/** @file
 * DarkHelp's configuration class.
 */


namespace DarkHelp
{
	/** All of %DarkHelp's configuration is stored within an instance of this class.  You can either instantiate a
	 * @ref DarkHelp::NN object and then access @ref DarkHelp::NN::config to set configuration as desired, or you can
	 * intantiate a @ref DarkHelp::Config object and pass it in to the @ref DarkHelp::NN constructor where it will be
	 * copied.
	 *
	 * @note Some fields such as the neural network filenames are only used once when @ref DarkHelp::NN::init() is called
	 * and then never referenced again.
	 *
	 * @since November 2021
	 */
	class Config final
	{
		public:

			/// Constructor.
			Config();

			/** Constructor.
			 *
			 * @note The order in which you pass the various filenames is @em not important if @p verify_files_first is set
			 * to @p true (the default value).  This is because @ref DarkHelp::verify_cfg_and_weights() is called to correctly
			 * determine which is the @p .cfg, @p .weights, and @p .names file, and swap the names around as necessary so
			 * Darknet is given the correct filenames.
			 */
			Config(const std::string & cfg_fn, const std::string & weights_fn, const std::string & names_fn = "", const bool verify_files_first = true, const EDriver d = EDriver::kDarknet);

			/// Destructor.
			~Config();

			/// Reset all config values to their default settings.
			Config & reset();

			/** Filename (relative or absolute) for the Darknet/YOLO @p .cfg file.
			 * Call @ref DarkHelp::NN::init() after changing this field to force the neural network to be re-loaded.
			 */
			std::string cfg_filename;

			/** Filename (relative or absolute) for the Darknet/YOLO @p .weights file.
			 * Call @ref DarkHelp::NN::init() after changing this field to force the neural network to be re-loaded.
			 */
			std::string weights_filename;

			/** Filename (relative or absolute) for the Darknet/YOLO @p .names file.
			 * Call @ref DarkHelp::NN::init() after changing this field to force the neural network to be re-loaded.
			 */
			std::string names_filename;

			/** Image prediction threshold.  Defaults to @p 0.5.
			 *
			 * @see @ref DarkHelp::NN::predict()
			 * @see @ref DarkHelp::NN::annotate()
			 *
			 * Quote: <blockquote> [...] threshold is what is used to determine whether or not there is an object in the predicted
			 * bounding box. The network predicts an explicit 'objectness' score separate from the class predictions that if above
			 * the threshold indicates that a bounding box will be returned.
			 * [<a href="https://github.com/philipperemy/yolo-9000/issues/3#issuecomment-304208297">source</a>]
			 * </blockquote>
			 */
			float threshold;

			/** Used during prediction.  Defaults to @p 0.5.  @see @ref DarkHelp::NN::predict()
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
			 * @see @ref DarkHelp::NN::predict()
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
			 *
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
			 *
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

			/** The colours to use in @ref DarkHelp::NN::annotate().  Defaults to @ref DarkHelp::get_default_annotation_colours().
			 *
			 * Remember that OpenCV uses BGR, not RGB.  So pure red is @p "(0, 0, 255)".
			 */
			VColours annotation_colours;

			/// Font face to use in @ref DarkHelp::NN::annotate().  Defaults to @p cv::HersheyFonts::FONT_HERSHEY_SIMPLEX.
			cv::HersheyFonts annotation_font_face;

			/// Scaling factor used for the font in @ref DarkHelp::NN::annotate().  Defaults to @p 0.5.
			double annotation_font_scale;

			/// Thickness of the font in @ref DarkHelp::NN::annotate().  Defaults to @p 1.
			int annotation_font_thickness;

			/// Thickness of the lines to draw in @ref DarkHelp::NN::annotate().  Defaults to @p 2.
			int annotation_line_thickness;

			/** If set to @p true then @ref DarkHelp::NN::annotate() will call @ref DarkHelp::NN::duration_string() and display
			 * on the top-left of the image the length of time @ref DarkHelp::NN::predict() took to process the image.  Defaults
			 * to @p true.
			 *
			 * When enabed, the duration may look similar to this:
			 * @image html barcode_100_percent.png
			 */
			bool annotation_include_duration;

			/** If set to @p true then @ref DarkHelp::NN::annotate() will display a timestamp on the bottom-left corner of the
			 * image.  Defaults to @p false.
			 *
			 * When enabled, the timestamp may look similar to this:
			 * @image html barcode_with_timestamp.png
			 */
			bool annotation_include_timestamp;

			/** Darknet sometimes will return values that are out-of-bound, especially when working with low thresholds.
			 * For example, the @p X or @p Y coordinates might be less than zero, or the @p width and @p height might extend
			 * beyond the edges of the image.  When @p %fix_out_of_bound_values is set to @p true (the default) then the
			 * results (@ref DarkHelp::NN::prediction_results) after calling @ref DarkHelp::NN::predict() will be capped so all
			 * values are positive and do not extend beyond the edges of the image.  When set to @p false, the exact values as
			 * returned by darknet will be used.  Defaults to @p true.
			 */
			bool fix_out_of_bound_values;

			/** Determines if the predictions will be sorted the next time @ref DarkHelp::NN::predict() is called.  When set to
			 * @ref DarkHelp::ESort::kUnsorted, the predictions are in the exact same order as they were returned by Darknet.
			 * When set to @ref DarkHelp::ESort::kAscending or @ref DarkHelp::ESort::kDescending, the predictions will be sorted
			 * according to @ref DarkHelp::PredictionResult::best_probability.
			 *
			 * If annotations will be drawn on the image for visual consumption, then it is often preferable to have the higher
			 * probability predictions drawn last so they appear "on top".  Otherwise, lower probability predictions may overwrite
			 * or obscure the more important ones.  This means using @ref DarkHelp::ESort::kAscending (the default).
			 *
			 * If you want to process only the first few predictions instead of drawing annotations, then you may want to sort
			 * using @ref DarkHelp::ESort::kDescending to ensure you handle the most likely predictions first.
			 *
			 * Defaults to @ref DarkHelp::ESort::kAscending.
			 */
			ESort sort_predictions;

			/** This enables some non-specific debug functionality within the DarkHelp library.  The exact results of enabling
			 * this is undocumented, and will change or may be completely removed without prior notice.  It is not meant for the
			 * end-user, but instead is used for developers debugging DarkHelp and Darknet.  Default value is @p false.
			 */
			bool enable_debug;

			/** Determines if calls to @ref DarkHelp::NN::predict() are sent directly to Darknet, or processed first by
			 * @ref DarkHelp::NN::predict_tile() to break the image file into smaller sections.
			 *
			 * This flag is only checked when @ref DarkHelp::NN::predict() is called.  If you call
			 * @ref DarkHelp::NN::predict_tile() directly, then it bypasses the check for @p DarkHelp::Config::enable_tiles and
			 * %DarkHelp will assume that the image is a candidate for tiling.
			 *
			 * @note Only images which are much larger than the network dimensions will be considered for tiles.  If an image is
			 * <b>less than</b> approximately 1.5 times the size of the network, then a 1x1 tile (meaning no tiling) will be used.
			 *
			 * Both @ref DarkHelp::NN::predict() and @ref DarkHelp::NN::predict_tile() will set the values
			 * @ref DarkHelp::NN::tile_size, @ref DarkHelp::NN::vertical_tiles, and @ref DarkHelp::NN::horizontal_tiles once they
			 * have finished running.  The caller can then reference these to determine what kind of tiling was used.  Even when
			 * an image is not tiled, these variables will be set; for example, @ref DarkHelp::NN::tile_size may be set to 1x1,
			 * and the horizontal and vertical sizes will match the neural network dimensions.
			 *
			 * The default value for @p DarkHelp::Config::enable_tiles is @p false, meaning that calling @ref DarkHelp::NN::predict()
			 * wont automatically result in image tiling.
			 *
			 * @see @ref Tiling
			 * @see @ref DarkHelp::Config::combine_tile_predictions
			 * @see @ref DarkHelp::NN::horizontal_tiles
			 * @see @ref DarkHelp::NN::vertical_tiles
			 * @see @ref DarkHelp::NN::tile_size
			 */
			bool enable_tiles;

			/** When training, the @p "batch=..." and @p "subdivisions=..." values in the .cfg file are typically set to a large
			 * value.  But when loading a neural network for inference as %DarkHelp is designed to help with, @em both of those
			 * values in the .cfg should be set to @p "1".  When @p modify_batch_and_subdivisions is enabled, %DarkHelp will edit
			 * the configuration file once @ref DarkHelp::NN::init() is called.  This ensures the values are set as needed prior
			 * to Darknet loading the .cfg file.
			 *
			 * The default value for @p modify_batch_and_subdivisions is @p true, meaning the .cfg file will be modified.  If set
			 * to @p false, %DarkHelp will not modify the configuration file.
			 *
			 * Example use:
			 *
			 * ~~~~
			 * DarkHelp::Config cfg("cars.cfg", "cars_best.weights", "cars.names");
			 * cfg.modify_batch_and_subdivisions = true;
			 * DarkHelp::NN nn(cfg);
			 * ~~~~
			 *
			 * @since Between version 1.1.9 on 2021-03-02 until 1.1.12 on 2021-04-08, this was modified to also toggle the new
			 * @p use_cuda_graph=1 flag in the @p [net] section.  See
			 * <a target="_blank" href="https://github.com/AlexeyAB/darknet/issues/7444">issue #7444</a> for additional details on
			 * @p use_cuda_graph.  Because this new option was suspected of causing some problems with some segfaults and object
			 * detection problems in some situations, this code was commented out again in v1.1.13 on 2021-04-08.
			 */
			bool modify_batch_and_subdivisions;

			/** Determines which classes to suppress during the call to @ref DarkHelp::NN::annotate().  Any prediction returned
			 * by Darknet for a class listed in this @p std::set will be ignored:  no bounding box will be drawn, and no label
			 * will be shown.  The set may be modified at any point and will take effect the next time
			 * @ref DarkHelp::NN::annotate() is called.
			 *
			 * It is initialized by @ref DarkHelp::NN::init() to contain any classes where the label name begins with the text
			 * @p "dont_show" as described in https://github.com/AlexeyAB/darknet/issues/2122.
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
			 * @note This does not suppress the @em detection of classes.  The vector returned when calling
			 * @ref DarkHelp::NN::predict() will contain all of the objects found by Darknet, regardless of what
			 * classes are listed in @p DarkHelp::Config::annotation_suppress_classes.
			 */
			std::set<int> annotation_suppress_classes;

			/** When tiling is enabled, objects may span multiple tiles.  When this flag is set to @p true, %DarkHelp will attempt
			 * to combine predictions that cross two or more tiles into a single prediction.  This has no impact when tiling is off,
			 * or when the image processed fits within a single tile.  Default is @p true.
			 *
			 * @see @ref DarkHelp::Config::enable_tiles
			 * @see @ref DarkHelp::Config::tile_edge_factor
			 * @see @ref DarkHelp::Config::tile_rect_factor
			 * @see @ref DarkHelp::Config::only_combine_similar_predictions
			 *
			 * Image								| Description
			 * -------------------------------------|------------
			 * @image html tile_combine_1.png ""	| For example, the image on the left is a portion of a much larger image.
			 * @image html tile_combine_2.png ""	| The blue horizontal line in this image shows the location of the boundary between two image tiles.
			 * @image html tile_combine_3.png ""	| When @p combine_tile_predictions=false, the predictions which are split by a tile boundary look like this.
			 * @image html tile_combine_4.png ""	| When @p combine_tile_predictions=true, the predictions split by a tile boundary are re-combined into a single object.
			 */
			bool combine_tile_predictions;

			/** When @p combine_tile_predictions is enabled, this determines if an attempt is made to combine predictions even
			 * when the class does not match.  Default is @p true.
			 *
			 * Note that when set to @p true, this compares @em all classes for the annotations.  For example:
			 *
			 * Image										| Description
			 * ---------------------------------------------|------------
			 * @image html tile_combine_similar_0.png ""	| Portion of original image.
			 * @image html tile_combine_similar_1.png ""	| Annotated image when tiling is enabled and combination is disabled.  This shows how the lock and @p "5" are split across multiple tiles, and how a tiny portion of the @p "5" seems to initially be annotated as a @p "6".
			 * @image html tile_combine_similar_2.png ""	| The blue lines show where the tile edges are located.  There are 4 tiles involved in this example.
			 * @image html tile_combine_similar_3.png ""	| Annotated image with full text labels.  It is important to note how the small sliver on the right is 49% likely to be a @p "6" and 20% to be a @p "5".  This is important since by default %DarkHelp only combines annotations which have a class in common.
			 * @image html tile_combine_similar_4.png ""	| Once @ref DarkHelp::Config::combine_tile_predictions has been enabled, the small sliver on the right was successfully combined into the @p "5".
			 * @image html tile_combine_similar_5.png ""	| The final annotations once combination has been performed.
			 *
			 * @see @ref DarkHelp::Config::combine_tile_predictions
			 * @see @ref DarkHelp::Config::tile_edge_factor
			 * @see @ref DarkHelp::Config::tile_rect_factor
			 */
			bool only_combine_similar_predictions;

			/** This value controls how close to the edge of a tile an object must be to be considered for re-combining when both
			 * tiling and recombining have been enabled.  The smaller the value, the closer the object must be to the edge of a
			 * tile.  The factor is multiplied by the width and height of the detected object.
			 *
			 * Possible range to consider would be @p 0.01 to @p 0.5.  If set to zero, then the detected object must be right on the
			 * tile boundary to be considered.  The default is @p 0.25.
			 *
			 * Image							| Setting
			 * ---------------------------------|--------
			 * @image html tile_edge_0.png ""	| @p combine_tile_predictions=false
			 * @image html tile_edge_1.png ""	| @p combine_tile_predictions=true <br/> @p tile_edge_factor=0.05
			 * @image html tile_edge_2.png ""	| @p combine_tile_predictions=true <br/> @p tile_edge_factor=0.15
			 *
			 * @see @ref DarkHelp::Config::enable_tiles
			 * @see @ref DarkHelp::Config::combine_tile_predictions
			 * @see @ref DarkHelp::Config::only_combine_similar_predictions
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
			 * Image							| Setting
			 * ---------------------------------|--------
			 * @image html tile_rect_0.png ""	| @p tile_rect_factor=1.00
			 * @image html tile_rect_1.png ""	| @p tile_rect_factor=1.10
			 * @image html tile_rect_2.png ""	| @p tile_rect_factor=1.20
			 *
			 * @see @ref DarkHelp::Config::enable_tiles
			 * @see @ref DarkHelp::Config::combine_tile_predictions
			 * @see @ref DarkHelp::Config::only_combine_similar_predictions
			 */
			float tile_rect_factor;

			/** The @p driver initialization happens in @ref DarkHelp::NN::init().  If you change
			 * this configuration value, you must remember to call @ref DarkHelp::NN::init() to
			 * force the neural network to be re-initialized.
			 */
			EDriver driver;

			/** Toggle annotation snapping.  This automatically creates and uses a binary black-and-white image of the original
			 * image to try and "snap" the annotations to an object.  Note that this is non-trivial and will slightly increase
			 * the processing time.  The default is @p false.
			 *
			 * Snapping is mostly intended to be used with dark-coloured items on a light background, such as text on paper.
			 * If you turn this on, you may also need to modify @ref DarkHelp::Config::binary_threshold_block_size,
			 * @ref DarkHelp::Config::binary_threshold_constant, @ref DarkHelp::Config::snapping_horizontal_tolerance,
			 * and @ref DarkHelp::Config::snapping_vertical_tolerance.
			 *
			 * @see @ref DarkHelp::NN::snap_annotations()
			 * @see @ref DarkHelp::Config::snapping_horizontal_tolerance
			 *
			 * Image								| Setting
			 * -------------------------------------|--------
			 * @image html snapping_disabled.png ""	| @p snapping_enabled=false
			 * @image html snapping_enabled.png ""	| @p snapping_enabled=true
			 */
			bool snapping_enabled;

			/** Block size (in pixels) to use when creating a black-and-white binary image.
			 * This is only used when @ref snapping_enabled is set to @p true or manually calling @ref DarkHelp::NN::snap_annotations().
			 * Default size is @p 25.
			 *
			 * @see 6th parameter of OpenCV's @p cv::adaptiveThreshold()
			 */
			int binary_threshold_block_size;

			/** Constant to remove from each pixel value when converting image during thresholding.
			 * This is only used when @ref snapping_enabled is set to @p true or manually calling @ref DarkHelp::NN::snap_annotations().
			 * Default is @p 25.0.
			 *
			 * @see 7th parameter of OpenCV's @p cv::adaptiveThreshold()
			 */
			double binary_threshold_constant;

			/** Horizontal tolerance (in pixels) used when snapping annotations.
			 * This is only used when @ref snapping_enabled is set to @p true or manually calling @ref DarkHelp::NN::snap_annotations().
			 * Default is @p 5.
			 *
			 * For example, when working with text, you may need to increase this if you want to automatically snap the annotations
			 * to grab multiple consecutive words in a sentence.  If you want to select a single word, then decrease this value so
			 * snapping ends at the end of a word.  If you lower it too much, you'll grab individual letters instead of words.
			 *
			 * Image								| Setting
			 * -------------------------------------|--------
			 * @image html snapping_h01_v01.png ""	| @p snapping_horizontal_tolerance=1 <br/> @p snapping_vertical_tolerance=1
			 * @image html snapping_h02_v01.png ""	| @p snapping_horizontal_tolerance=2 <br/> @p snapping_vertical_tolerance=1
			 * @image html snapping_h03_v01.png ""	| @p snapping_horizontal_tolerance=3 <br/> @p snapping_vertical_tolerance=1
			 * @image html snapping_h07_v01.png ""	| @p snapping_horizontal_tolerance=7 <br/> @p snapping_vertical_tolerance=1
			 * @image html snapping_h07_v03.png ""	| @p snapping_horizontal_tolerance=7 <br/> @p snapping_vertical_tolerance=3
			 * @image html snapping_h07_v15.png ""	| @p snapping_horizontal_tolerance=7 <br/> @p snapping_vertical_tolerance=15
			 */
			int snapping_horizontal_tolerance;

			/** Vertical tolerance (in pixels) used when snapping annotations.
			 * This is only used when @ref snapping_enabled is set to @p true or manually calling @ref DarkHelp::NN::snap_annotations().
			 * Default is @p 5.
			 *
			 * For example, when working with text, you may need to increase this if you want to automatically snap the annotations
			 * to grab an entire paragraph.  Smaller values will limit the snap to a single line of text.
			 *
			 * @see @ref DarkHelp::Config::snapping_horizontal_tolerance
			 */
			int snapping_vertical_tolerance;

			/** When snapping is enabled, this is used to establish a @b minimum for snapping.  If the snapped annotation shrinks
			 * less than this amount, the "snap" is ignored and the original annotation is retained.
			 *
			 * The valid range for this is any number between @p 0 and @p 1.0.  If set to @p 0, then no minimum limit will be
			 * applied during snapping.  If set to exactly @p 1, then snapping cannot shrink annotations, it can only grow them.
			 * The default is @p 0.4.
			 *
			 * @see @ref DarkHelp::Config::snapping_enabled
			 * @see @ref DarkHelp::Config::snapping_limit_grow
			 */
			float snapping_limit_shrink;

			/** When snapping is enabled, this is used to establish a @b maximum for snapping.  If the snapped annotation grows
			 * more than this amount, the "snap" is ignored and the original annotation is retained.
			 *
			 * The valid range for this is any number larger than or equal to @p 1.0.  If set to @p 0, then no maximum limit
			 * will be applied during snapping.  If set to exactly @p 1, then snapping cannot grow annotations, it can only
			 * shrink them.  The default is @p 1.25.
			 *
			 * @see @ref DarkHelp::Config::snapping_enabled
			 * @see @ref DarkHelp::Config::snapping_limit_shrink
			 */
			float snapping_limit_grow;
	};
}
