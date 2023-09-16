/* DarkHelp - C++ helper class for Darknet's C API.
 * Copyright 2019-2023 Stephane Charette <stephanecharette@gmail.com>
 * MIT license applies.  See "license.txt" for details.
 */

#include <DarkHelp.hpp>

#ifdef WIN32
#pragma warning(disable: 4305)
#endif


DarkHelp::Config::~Config()
{
	return;
}

DarkHelp::Config::Config()
{
	reset();

	return;
}


DarkHelp::Config::Config(const std::string & cfg_fn, const std::string & weights_fn, const std::string & names_fn, const bool verify_files_first, const EDriver d) :
	DarkHelp::Config::Config()
{
	cfg_filename		= cfg_fn;
	weights_filename	= weights_fn;
	names_filename		= names_fn;

	if (verify_files_first)
	{
		DarkHelp::verify_cfg_and_weights(cfg_filename, weights_filename, names_filename);
	}

	#ifndef HAVE_OPENCV_DNN_OBJDETECT
		// with old versions of OpenCV, we don't have a DNN module
		driver = DarkHelp::EDriver::kDarknet;
	#else
		driver = d;
	#endif

	return;
}


DarkHelp::Config & DarkHelp::Config::reset()
{
	// pick some reasonable default values

	cfg_filename						.clear();
	weights_filename					.clear();
	names_filename						.clear();
	threshold							= 0.5f;
	hierarchy_threshold					= 0.5f;
	non_maximal_suppression_threshold	= 0.45f;
	annotation_font_face				= cv::HersheyFonts::FONT_HERSHEY_SIMPLEX;
	annotation_font_scale				= 0.5;
	annotation_font_thickness			= 1;
	annotation_line_thickness			= 2;
	annotation_include_duration			= true;
	annotation_include_timestamp		= false;
	annotation_pixelate_enabled			= false;
	annotation_pixelate_size			= 15;
	annotation_pixelate_classes			.clear();
	names_include_percentage			= true;
	include_all_names					= true;
	fix_out_of_bound_values				= true;
	annotation_colours					= DarkHelp::get_default_annotation_colours();
	sort_predictions					= ESort::kAscending;
	annotation_auto_hide_labels			= true;
	annotation_suppress_all_labels		= false;
	annotation_shade_predictions		= 0.25;
	enable_debug						= false;
	enable_tiles						= false;
	combine_tile_predictions			= true;
	only_combine_similar_predictions	= true;
	tile_edge_factor					= 0.25f;
	tile_rect_factor					= 1.20f;
	modify_batch_and_subdivisions		= true;
	driver								= EDriver::kInvalid;
	annotation_suppress_classes			.clear();
	snapping_enabled					= false;
	binary_threshold_block_size			= 25;
	binary_threshold_constant			= 25;
	snapping_horizontal_tolerance		= 5;
	snapping_vertical_tolerance			= 5;
	snapping_limit_shrink				= 0.4;
	snapping_limit_grow					= 1.25;
	redirect_darknet_output				= true;
	use_fast_image_resize				= true;

	return *this;
}
