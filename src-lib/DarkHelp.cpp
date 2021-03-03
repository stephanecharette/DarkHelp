/* DarkHelp - C++ helper class for Darknet's C API.
 * Copyright 2019-2021 Stephane Charette <stephanecharette@gmail.com>
 * MIT license applies.  See "license.txt" for details.
 */

#include <DarkHelp.hpp>
#include <fstream>
#include <regex>
#include <cmath>
#include <ctime>
#include <sys/stat.h>


/* If you are using a recent version of Darknet there is no problem, and you can skip the rest of this comment.
 *
 * If you are using a version of Darknet from before 2020-March-01, then the following comment applies:
 *
 * --------------------------------------------------------
 * Prior to including @p darknet.h, you @b must @p "#define GPU 1" and @p "#define CUDNN 1" @b if darknet was built with
 * support for GPU and CUDNN!  This is because the darknet structures have several optional fields that only exist when
 * @p GPU and @p CUDNN are defined, thereby changing the size of those structures.  If DarkHelp and Darknet aren't
 * using the exact same structure size, you'll see segfaults when DarkHelp calls into Darknet.
 * --------------------------------------------------------
 *
 * This problem was fixed in Darknet by AlexeyAB on 2020-March-01.  See this post for details:
 * https://github.com/AlexeyAB/darknet/issues/4839#issuecomment-593085313
 */
#include <darknet.h>


/* OpenCV4 has renamed some common defines and placed them in the cv namespace.  Need to deal with this until older
 * versions of OpenCV are no longer in use.
 */
#if 1 /// @todo remove this soon
#ifndef CV_INTER_CUBIC
#define CV_INTER_CUBIC cv::INTER_CUBIC
#endif
#ifndef CV_INTER_AREA
#define CV_INTER_AREA cv::INTER_AREA
#endif
#ifndef CV_AA
#define CV_AA cv::LINE_AA
#endif
#ifndef CV_FILLED
#define CV_FILLED cv::FILLED
#endif
#endif


DarkHelp::~DarkHelp()
{
	reset();

	return;
}


DarkHelp::DarkHelp() :
	net(nullptr)
{
	reset();

	/* There used to be a problem with the size of certain darknet structure being different depending on whether darknet
	 * was compiled for CPU-only, GPU, or GPU+cuDNN.  This is no longer a problem since darknet issue #4839 was fixed on
	 * 2020-Mar-01, but leaving this here (commented out) in case this needs to be debugged or re-verified every once in a
	 * while.  Last time this was recorded was 2020-03-07:
	 *		sizeof(network) ........ 504
	 *		sizeof(network_state) .. 544
	 *		sizeof(layer) .......... 2376
	 *		sizeof(image) .......... 24
	 *		sizeof(detection) ...... 64
	 *		sizeof(load_args) ...... 208
	 *		sizeof(data) ........... 64
	 *		sizeof(metadata) ....... 16
	 *		sizeof(tree) ........... 72
	 */
	#if 0
	std::cout
		<< "sizeof(network) ........ " << sizeof(network		) << std::endl
		<< "sizeof(network_state) .. " << sizeof(network_state	) << std::endl
		<< "sizeof(layer) .......... " << sizeof(layer			) << std::endl
		<< "sizeof(image) .......... " << sizeof(image			) << std::endl
		<< "sizeof(detection) ...... " << sizeof(detection		) << std::endl
		<< "sizeof(load_args) ...... " << sizeof(load_args		) << std::endl
		<< "sizeof(data) ........... " << sizeof(data			) << std::endl
		<< "sizeof(metadata) ....... " << sizeof(metadata		) << std::endl
		<< "sizeof(tree) ........... " << sizeof(tree			) << std::endl;
	#endif

	return;
}


DarkHelp::DarkHelp(const std::string & cfg_filename, const std::string & weights_filename, const std::string & names_filename, const bool verify_files_first) :
	DarkHelp()
{
	init(cfg_filename, weights_filename, names_filename, verify_files_first);

	return;
}


std::string DarkHelp::version() const
{
	return DH_VERSION;
}


DarkHelp & DarkHelp::init(const std::string & cfg_filename, const std::string & weights_filename, const std::string & names_filename, const bool verify_files_first)
{
	reset();

	// darknet behaves very badly if the .cfg and the .weights files are accidentally swapped.  I've seen it segfault when
	// it attempts to parse the weights as a text flle.  So do a bit of simple verification and swap them if necessary.
	std::string cfg_fn		= cfg_filename;
	std::string weights_fn	= weights_filename;
	std::string names_fn	= names_filename;
	if (verify_files_first)
	{
		verify_cfg_and_weights(cfg_fn, weights_fn, names_fn);
	}

	if (modify_batch_and_subdivisions)
	{
		const MStr m =
		{
			{"batch"		, "1"},
			{"subdivisions"	, "1"}
		};
		edit_cfg_file(cfg_fn, m);

		// do not combine this settings with the previous two since there is code that
		// needs to behave differently when only the batch+subdivisions are modified
		edit_cfg_file(cfg_fn, {{"use_cuda_graph", "1"}});
	}

	// The calls we make into darknet are based on what was found in test_detector() from src/detector.c.

	const auto t1 = std::chrono::high_resolution_clock::now();
	net = load_network_custom(const_cast<char*>(cfg_fn.c_str()), const_cast<char*>(weights_fn.c_str()), 1, 1);
	if (net == nullptr)
	{
		/// @throw std::runtime_error if the call to darknet's @p load_network_custom() has failed.
		throw std::runtime_error("darknet failed to load the configuration, the weights, or both");
	}

	network * nw = reinterpret_cast<network*>(net);

	// what does this call do?
	calculate_binary_weights(*nw);

	const auto t2 = std::chrono::high_resolution_clock::now();
	duration = t2 - t1;

	if (not names_fn.empty())
	{
		std::ifstream ifs(names_fn);
		std::string line;
		while (std::getline(ifs, line))
		{
			if (line.empty())
			{
				break;
			}
			names.push_back(line);
		}
	}

	// see which classes need to be suppressed (https://github.com/AlexeyAB/darknet/issues/2122)
	for (size_t i = 0; i < names.size(); i ++)
	{
		if (names.at(i).find("dont_show") == 0)
		{
			annotation_suppress_classes.insert(i);
		}
	}

	return *this;
}


void DarkHelp::reset()
{
	if (net)
	{
		network * nw = reinterpret_cast<network*>(net);
		free_network(*nw);
		free(net); // this was calloc()'d in load_network_custom()
		net = nullptr;
	}

	names								.clear();
	prediction_results					.clear();
	original_image						= cv::Mat();
	annotated_image						= cv::Mat();

	// pick some reasonable default values
	threshold							= 0.5f;
	hierarchy_threshold					= 0.5f;
	non_maximal_suppression_threshold	= 0.45f;
	annotation_font_face				= cv::HersheyFonts::FONT_HERSHEY_SIMPLEX;
	annotation_font_scale				= 0.5;
	annotation_font_thickness			= 1;
	annotation_line_thickness			= 2;
	annotation_include_duration			= true;
	annotation_include_timestamp		= false;
	names_include_percentage			= true;
	include_all_names					= true;
	fix_out_of_bound_values				= true;
	annotation_colours					= get_default_annotation_colours();
	sort_predictions					= ESort::kAscending;
	annotation_auto_hide_labels			= true;
	annotation_shade_predictions		= 0.25;
	enable_debug						= false;
	enable_tiles						= false;
	horizontal_tiles					= 1;
	vertical_tiles						= 1;
	combine_tile_predictions			= true;
	tile_edge_factor					= 0.25f;
	tile_rect_factor					= 1.20f;
	modify_batch_and_subdivisions		= true;
	annotation_suppress_classes			.clear();

	return;
}


DarkHelp::PredictionResults DarkHelp::predict(const std::string & image_filename, const float new_threshold)
{
	cv::Mat mat = cv::imread(image_filename);
	if (mat.empty())
	{
		/// @throw std::invalid_argument if the image failed to load.
		throw std::invalid_argument("failed to load image \"" + image_filename + "\"");
	}

	return predict(mat, new_threshold);
}


DarkHelp::PredictionResults DarkHelp::predict(cv::Mat mat, const float new_threshold)
{
	if (mat.empty())
	{
		/// @throw std::invalid_argument if the image is empty.
		throw std::invalid_argument("cannot predict with an empty OpenCV image");
	}

	if (enable_tiles)
	{
		return predict_tile(mat, new_threshold);
	}

	return predict_internal(mat, new_threshold);
}


#ifdef DARKHELP_CAN_INCLUDE_DARKNET
DarkHelp::PredictionResults DarkHelp::predict(image img, const float new_threshold)
{
	/* This is inefficient since we eventually need a Darknet "image", but we're going to convert the image back to a
	 * OpenCV-format cv::Mat.  This allows the DarkHelp object to be more consistent with the way it handles images.
	 */
	cv::Mat mat = convert_darknet_image_to_opencv_mat(img);
	if (mat.empty())
	{
		/// @throw std::invalid_argument if the image is empty.
		throw std::invalid_argument("image is empty or has failed to convert from Darknet's 'image' format");
	}

	return predict(mat, new_threshold);
}
#endif


DarkHelp::PredictionResults DarkHelp::predict_tile(cv::Mat mat, const float new_threshold)
{
	if (mat.empty())
	{
		/// @throw std::invalid_argument if the image is empty.
		throw std::invalid_argument("cannot predict with an empty OpenCV image");
	}

	const cv::Size network_dimensions	= network_size();
	const float horizontal_factor		= static_cast<float>(mat.cols) / static_cast<float>(network_dimensions.width);
	const float vertical_factor			= static_cast<float>(mat.rows) / static_cast<float>(network_dimensions.height);
	const float horizontal_tiles_count	= std::round(std::max(1.0f, horizontal_factor	));
	const float vertical_tiles_count	= std::round(std::max(1.0f, vertical_factor		));
	const float tile_width				= static_cast<float>(mat.cols) / horizontal_tiles_count;
	const float tile_height				= static_cast<float>(mat.rows) / vertical_tiles_count;
	const cv::Size new_tile_size		= cv::Size(std::round(tile_width), std::round(tile_height));

	if (horizontal_tiles_count == 1 and vertical_tiles_count == 1)
	{
		// image is smaller than (or equal to) the network, so use the original predict() call
		return predict_internal(mat, new_threshold);
	}

	// otherwise, if we get here then we have more than 1 tile

	// divide the original image into the right number of tiles and call predict() on each tile
	PredictionResults results;
	std::vector<size_t> indexes_of_predictions_near_edges;
	std::vector<cv::Mat> all_tile_mats;
	std::chrono::high_resolution_clock::duration total_duration = std::chrono::milliseconds(0);

	for (float y = 0.0f; y < vertical_tiles_count; y ++)
	{
		for (float x = 0.0f; x < horizontal_tiles_count; x ++)
		{
			const int tile_count = y * horizontal_tiles_count + x;

			const int x_offset = std::round(x * tile_width);
			const int y_offset = std::round(y * tile_height);
			cv::Rect r(cv::Point(x_offset, y_offset), new_tile_size);

			// make sure the rectangle does not extend beyond the edges of the image
			if (r.x + r.width >= mat.cols)
			{
				r.width = mat.cols - r.x - 1;
			}
			if (r.y + r.height >= mat.rows)
			{
				r.height = mat.rows - r.y - 1;
			}

			cv::Mat roi = mat(r);

			predict_internal(roi, new_threshold);

			total_duration += duration;

			// fix up the predictions -- need to compensate for the tile not being the top-left corner of the image, and the size of the tile being smaller than the image
			for (auto & prediction : prediction_results)
			{
				// track which predictions are near the edges, because we may need to re-examine them and join them after we finish with all the tiles
				if (combine_tile_predictions)
				{
					const int minimum_horizontal_distance	= tile_edge_factor * prediction.rect.width;
					const int minimum_vertical_distance		= tile_edge_factor * prediction.rect.height;
					if (prediction.rect.x <= minimum_horizontal_distance					or
						prediction.rect.y <= minimum_vertical_distance						or
						roi.cols - prediction.rect.br().x <= minimum_horizontal_distance	or
						roi.rows - prediction.rect.br().y <= minimum_vertical_distance		)
					{
						// this prediction is near one of the tile borders so we need to remember it
						indexes_of_predictions_near_edges.push_back(results.size());
					}
				}

				// every prediction needs to have x_offset and y_offset added to it
				prediction.rect.x += x_offset;
				prediction.rect.y += y_offset;
				prediction.tile = tile_count;

				if (enable_debug)
				{
					// draw a black-on-white debug label on the top side of the annotation

					const std::string label		= std::to_string(results.size());
					const auto font				= cv::HersheyFonts::FONT_HERSHEY_PLAIN;
					const auto scale			= 0.75;
					const auto thickness		= 1;
					int baseline				= 0;
					const cv::Size text_size	= cv::getTextSize(label, font, scale, thickness, &baseline);
					const int text_half_width	= text_size.width			/ 2;
					const int text_half_height	= text_size.height			/ 2;
					const int pred_half_width	= prediction.rect.width		/ 2;
					const int pred_half_height	= prediction.rect.height	/ 2;

					// put the text exactly in the middle of the prediction
					const cv::Rect label_rect(
							prediction.rect.x + pred_half_width - text_half_width,
							prediction.rect.y + pred_half_height - text_half_height,
							text_size.width, text_size.height);
					cv::rectangle(mat, label_rect, {255, 255, 255}, cv::FILLED, cv::LINE_AA);
					cv::putText(mat, label, cv::Point(label_rect.x, label_rect.y + label_rect.height), font, scale, cv::Scalar(0,0,0), thickness, CV_AA);
				}

				// the original point and size are based on only 1 tile, so they also need to be fixed

				prediction.original_point.x = (static_cast<float>(prediction.rect.x) + static_cast<float>(prediction.rect.width	) / 2.0f) / static_cast<float>(mat.cols);
				prediction.original_point.y = (static_cast<float>(prediction.rect.y) + static_cast<float>(prediction.rect.height) / 2.0f) / static_cast<float>(mat.rows);

				prediction.original_size.width	= static_cast<float>(prediction.rect.width	) / static_cast<float>(mat.cols);
				prediction.original_size.height	= static_cast<float>(prediction.rect.height	) / static_cast<float>(mat.rows);

				results.push_back(prediction);
			}
		}
	}

	if (indexes_of_predictions_near_edges.empty() == false)
	{
		// we need to go through all the results from the various tiles and merged together the ones that are side-by-side

		for (const auto & lhs_idx : indexes_of_predictions_near_edges)
		{
			if (results[lhs_idx].rect.area() == 0 and results[lhs_idx].tile == -1)
			{
				// this items has already been consumed and is marked for deletion
				continue;
			}

			const cv::Rect & lhs_rect = results[lhs_idx].rect;

			// now compare this rect against all other rects that come *after* this
			for (const auto & rhs_idx : indexes_of_predictions_near_edges)
			{
				if (rhs_idx <= lhs_idx)
				{
					// if the RHS object is on an earlier tile, then we've already compared it
					continue;
				}

				if (results[lhs_idx].tile == results[rhs_idx].tile)
				{
					// if two objects are on the exact same tile, don't bother trying to combine them
					continue;
				}

				if (results[rhs_idx].rect.area() == 0 and results[rhs_idx].tile == -1)
				{
					// this items has already been consumed and is marked for deletion
					continue;
				}

				const cv::Rect & rhs_rect		= results[rhs_idx].rect;
				const cv::Rect combined_rect	= lhs_rect | rhs_rect;

				// if this is a good match, then the area of the combined rect will be similar to the area of lhs+rhs
				const int lhs_area		= lhs_rect		.area();
				const int rhs_area		= rhs_rect		.area();
				const int lhs_plus_rhs	= (lhs_area + rhs_area) * tile_rect_factor;
				const int combined_area	= combined_rect	.area();

				if (combined_area <= lhs_plus_rhs)
				{
					auto & lhs = results[lhs_idx];
					auto & rhs = results[rhs_idx];

					lhs.rect = combined_rect;

					lhs.original_point.x = (static_cast<float>(lhs.rect.x) + static_cast<float>(lhs.rect.width	) / 2.0f) / static_cast<float>(mat.cols);
					lhs.original_point.y = (static_cast<float>(lhs.rect.y) + static_cast<float>(lhs.rect.height	) / 2.0f) / static_cast<float>(mat.rows);

					lhs.original_size.width		= static_cast<float>(lhs.rect.width	) / static_cast<float>(mat.cols);
					lhs.original_size.height	= static_cast<float>(lhs.rect.height) / static_cast<float>(mat.rows);

					// rebuild "all_probabilities" by combining both objects and keeping the max percentage
					for (auto iter : rhs.all_probabilities)
					{
						const auto & key		= iter.first;
						const auto & rhs_val	= iter.second;
						const auto & lhs_val	= lhs.all_probabilities[key];

						lhs.all_probabilities[key] = std::max(lhs_val, rhs_val);
					}

					// come up with a decent + consistent name to use for this object
					name_prediction(lhs);

					// mark the RHS to be deleted once we're done looping through all the results
					rhs.rect = {0, 0, 0, 0};
					rhs.tile = -1;
				}
			}
		}

		// now go through the results and delete any with an empty rect and tile of -1
		auto iter = results.begin();
		while (iter != results.end())
		{
			if (iter->rect.area() == 0 and iter->tile == -1)
			{
				// delete this prediction from the results since it has been combined with something else
				iter = results.erase(iter);
			}
			else
			{
				iter ++;
			}
		}
	}

	if (enable_debug)
	{
		// draw vertical lines to show the tiles
		for (float x=1.0; x < horizontal_tiles_count; x++)
		{
			const int x_pos = std::round(mat.cols / horizontal_tiles_count * x);
			cv::line(mat, cv::Point(x_pos, 0), cv::Point(x_pos, mat.rows), {255, 0, 0});
		}

		// draw horizontal lines to show the tiles
		for (float y=1.0; y < vertical_tiles_count; y++)
		{
			const int y_pos = std::round(mat.rows / vertical_tiles_count * y);
			cv::line(mat, cv::Point(0, y_pos), cv::Point(mat.cols, y_pos), {255, 0, 0});
		}
	}

	original_image		= mat;
	prediction_results	= results;
	duration			= total_duration;
	horizontal_tiles	= horizontal_tiles_count;
	vertical_tiles		= vertical_tiles_count;
	tile_size			= new_tile_size;

	return results;
}


cv::Mat DarkHelp::annotate(const float new_threshold)
{
	if (original_image.empty())
	{
		/// @throw std::logic_error if an attempt is made to annotate an empty image
		throw std::logic_error("cannot annotate an empty image; must call predict() first");
	}

	if (new_threshold >= 0.0)
	{
		threshold = new_threshold;
	}

	annotated_image = original_image.clone();

	// make sure we always have colours we can use
	if (annotation_colours.empty())
	{
		annotation_colours = get_default_annotation_colours();
	}

	for (const auto & pred : prediction_results)
	{
		if (annotation_suppress_classes.count(pred.best_class) != 0)
		{
			continue;
		}

		if (annotation_line_thickness > 0 and pred.best_probability >= threshold)
		{
			const auto colour = annotation_colours[pred.best_class % annotation_colours.size()];

			int line_thickness_or_fill = annotation_line_thickness;
			if (annotation_shade_predictions >= 1.0)
			{
				line_thickness_or_fill = CV_FILLED;
			}
			else if (annotation_shade_predictions > 0.0)
			{
				cv::Mat roi = annotated_image(pred.rect);
				cv::Mat coloured_rect(roi.size(), roi.type(), colour);

				const double alpha = annotation_shade_predictions;
				const double beta = 1.0 - alpha;
				cv::addWeighted(coloured_rect, alpha, roi, beta, 0.0, roi);
			}

//			std::cout << "class id=" << pred.best_class << ", probability=" << pred.best_probability << ", point=(" << pred.rect.x << "," << pred.rect.y << "), name=\"" << pred.name << "\", duration=" << duration_string() << std::endl;
			cv::rectangle(annotated_image, pred.rect, colour, line_thickness_or_fill);

			int baseline = 0;
			const cv::Size text_size = cv::getTextSize(pred.name, annotation_font_face, annotation_font_scale, annotation_font_thickness, &baseline);

			if (annotation_auto_hide_labels)
			{
				if (text_size.width >= pred.rect.width or
					text_size.height >= pred.rect.height)
				{
					// label is too large to display
					continue;
				}
			}

			cv::Rect r(cv::Point(pred.rect.x - annotation_line_thickness/2, pred.rect.y - text_size.height - baseline + annotation_line_thickness), cv::Size(text_size.width + annotation_line_thickness, text_size.height + baseline));
			if (r.x < 0) r.x = 0;																			// shift the label to the very left edge of the screen, otherwise it would be off-screen
			if (r.x + r.width >= annotated_image.cols) r.x = pred.rect.x + pred.rect.width - r.width + 1;	// first attempt at pushing the label to the left
			if (r.x + r.width >= annotated_image.cols) r.x = annotated_image.cols - r.width;				// more drastic attempt at pushing the label to the left

			if (r.y < 0) r.y = pred.rect.y + pred.rect.height;	// shift the label to the bottom of the prediction, otherwise it would be off-screen
			if (r.y + r.height >= annotated_image.rows) r.y = pred.rect.y + 1; // shift the label to the inside-top of the prediction (CV seems to have trouble drawing text where the upper bound is y=0, so move it down 1 pixel)
			if (r.y < 0) r.y = 0; // shift the label to the top of the image if it is off-screen

			cv::rectangle(annotated_image, r, colour, CV_FILLED);
			cv::putText(annotated_image, pred.name, cv::Point(r.x + annotation_line_thickness/2, r.y + text_size.height), annotation_font_face, annotation_font_scale, cv::Scalar(0,0,0), annotation_font_thickness, CV_AA);
		}
	}

	if (annotation_include_duration)
	{
		const std::string str		= duration_string();
		const cv::Size text_size	= cv::getTextSize(str, annotation_font_face, annotation_font_scale, annotation_font_thickness, nullptr);

		cv::Rect r(cv::Point(2, 2), cv::Size(text_size.width + 2, text_size.height + 2));
		cv::rectangle(annotated_image, r, cv::Scalar(255,255,255), CV_FILLED);
		cv::putText(annotated_image, str, cv::Point(r.x + 1, r.y + text_size.height), annotation_font_face, annotation_font_scale, cv::Scalar(0,0,0), annotation_font_thickness, CV_AA);
	}

	if (annotation_include_timestamp)
	{
		const std::time_t tt = std::time(nullptr);
		char timestamp[100];
		strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", std::localtime(&tt));

		const cv::Size text_size = cv::getTextSize(timestamp, annotation_font_face, annotation_font_scale, annotation_font_thickness, nullptr);

		cv::Rect r(cv::Point(2, annotated_image.rows - text_size.height - 4), cv::Size(text_size.width + 2, text_size.height + 2));
		cv::rectangle(annotated_image, r, cv::Scalar(255,255,255), CV_FILLED);
		cv::putText(annotated_image, timestamp, cv::Point(r.x + 1, r.y + text_size.height), annotation_font_face, annotation_font_scale, cv::Scalar(0,0,0), annotation_font_thickness, CV_AA);
	}

	return annotated_image;
}


#ifdef DARKHELP_CAN_INCLUDE_DARKNET
image DarkHelp::convert_opencv_mat_to_darknet_image(cv::Mat mat)
#else
static inline image convert_opencv_mat_to_darknet_image(cv::Mat mat)
#endif
{
	// this function is taken/inspired directly from Darknet:  image_opencv.cpp, mat_to_image()

	// OpenCV uses BGR, but Darknet expects RGB
	if (mat.channels() == 3)
	{
		cv::Mat rgb;
		cv::cvtColor(mat, rgb, cv::COLOR_BGR2RGB);
		mat = rgb;
	}

	const int width		= mat.cols;
	const int height	= mat.rows;
	const int channels	= mat.channels();
	const int step		= mat.step;
	image img			= make_image(width, height, channels);
	uint8_t * data		= (uint8_t*)mat.data;

	for (int y = 0; y < height; ++y)
	{
		for (int c = 0; c < channels; ++c)
		{
			for (int x = 0; x < width; ++x)
			{
				img.data[c*width*height + y*width + x] = data[y*step + x*channels + c] / 255.0f;
			}
		}
	}

	return img;
}


#ifdef DARKHELP_CAN_INCLUDE_DARKNET
cv::Mat DarkHelp::convert_darknet_image_to_opencv_mat(const image img)
#else
static inline cv::Mat convert_darknet_image_to_opencv_mat(const image img)
#endif
{
	// this function is taken/inspired directly from Darknet:  image_opencv.cpp, image_to_mat()

	const int channels	= img.c;
	const int width		= img.w;
	const int height	= img.h;
	cv::Mat mat			= cv::Mat(height, width, CV_8UC(channels));
	const int step		= mat.step;

	for (int y = 0; y < height; ++y)
	{
		for (int x = 0; x < width; ++x)
		{
			for (int c = 0; c < channels; ++c)
			{
				float val = img.data[c*height*width + y*width + x];
				mat.data[y*step + x*channels + c] = (unsigned char)(val * 255);
			}
		}
	}

	// But now the mat is in RGB instead of the BGR format that OpenCV expects to use.  See show_image_cv()
	// in Darknet which does the RGB<->BGR conversion, which we'll copy here so the mat is immediately usable.
	if (channels == 3)
	{
		cv::cvtColor(mat, mat, cv::COLOR_RGB2BGR);
	}

	return mat;
}


std::string DarkHelp::duration_string()
{
	std::string str;
	if		(duration <= std::chrono::nanoseconds(1000))	{ str = std::to_string(std::chrono::duration_cast<std::chrono::nanoseconds>	(duration).count()) + " nanoseconds";	}
	else if	(duration <= std::chrono::microseconds(1000))	{ str = std::to_string(std::chrono::duration_cast<std::chrono::microseconds>(duration).count()) + " microseconds";	}
	else if	(duration <= std::chrono::milliseconds(1000))	{ str = std::to_string(std::chrono::duration_cast<std::chrono::milliseconds>(duration).count()) + " milliseconds";	}
	else /* use milliseconds for anything longer */			{ str = std::to_string(std::chrono::duration_cast<std::chrono::milliseconds>(duration).count()) + " milliseconds";	}

	return str;
}


cv::Size DarkHelp::network_size()
{
	if (net == nullptr)
	{
		/// @throw std::logic_error if the neural network has not yet been initialized.
		throw std::logic_error("cannot determine the size of uninitialized neural network");
	}

	network * nw = reinterpret_cast<network*>(net);

	return cv::Size(nw->w, nw->h);
}


DarkHelp::VColours DarkHelp::get_default_annotation_colours()
{
	VColours colours =
	{
		// remember the OpenCV format is blue-green-red and not RGB!
		{0x5E, 0x35, 0xFF},	// Radical Red
		{0x17, 0x96, 0x29},	// Slimy Green
		{0x33, 0xCC, 0xFF},	// Sunglow
		{0x4D, 0x6E, 0xAF},	// Brown Sugar
		{0xFF, 0x00, 0xFF},	// pure magenta
		{0xE6, 0xBF, 0x50},	// Blizzard Blue
		{0x00, 0xFF, 0xCC},	// Electric Lime
		{0xFF, 0xFF, 0x00},	// pure cyan
		{0x85, 0x4E, 0x8D},	// Razzmic Berry
		{0xCC, 0x48, 0xFF},	// Purple Pizzazz
		{0x00, 0xFF, 0x00},	// pure green
		{0x00, 0xFF, 0xFF},	// pure yellow
		{0xEC, 0xAD, 0x5D},	// Blue Jeans
		{0xFF, 0x6E, 0xFF},	// Shocking Pink
		{0xD1, 0xF0, 0xAA},	// Magic Mint
		{0x00, 0xC0, 0xFF},	// orange
		{0xB6, 0x51, 0x9C},	// Purple Plum
		{0x33, 0x99, 0xFF},	// Neon Carrot
		{0x66, 0xFF, 0x66},	// Screamin' Green
		{0x00, 0x00, 0xFF},	// pure red
		{0x82, 0x00, 0x4B},	// Indigo
		{0x37, 0x60, 0xFF},	// Outrageous Orange
		{0x66, 0xFF, 0xFF},	// Laser Lemon
		{0x78, 0x5B, 0xFD},	// Wild Watermelon
		{0xFF, 0x00, 0x00}	// pure blue
	};

	return colours;
}


DarkHelp::MStr DarkHelp::verify_cfg_and_weights(std::string & cfg_filename, std::string & weights_filename, std::string & names_filename)
{
	MStr m;

	// we need a minimum of 2 unique files for things to be valid
	std::set<std::string> all_filenames;
	all_filenames.insert(cfg_filename		);
	all_filenames.insert(weights_filename	);
	all_filenames.insert(names_filename		);
	if (all_filenames.size() < 2)
	{
		/// @throw std::invalid_argument if at least 2 unique filenames have not been provided
		throw std::invalid_argument("need a minimum of 2 filenames (cfg and weights) to load darknet neural network");
	}

	// The simplest case is to look at the file extensions.  If they happen to be .cfg, and .weights, then
	// that is what we'll use to decide which file is which.  (And anything left over will be the .names file.)
	for (const auto & filename : all_filenames)
	{
		const size_t pos = filename.rfind(".");
		if (pos != std::string::npos)
		{
			const std::string ext = filename.substr(pos + 1);
			m[ext] = filename;
		}
	}
	if (m.count("cfg"		) == 1 and
		m.count("weights"	) == 1)
	{
		for (auto iter : m)
		{
			if (iter.first == "weights")	weights_filename	= iter.second;
			else if (iter.first == "cfg")	cfg_filename		= iter.second;
			else							names_filename		= iter.second;
		}

		// We *know* we have a cfg and weights, but the names is optional.  If we have a 3rd filename, then it must be the names,
		// otherwise blank out the 3rd filename in case that field was used to pass in either the cfg or the weights.
		if (names_filename == cfg_filename or names_filename == weights_filename)
		{
			names_filename = "";
		}
		m["names"] = names_filename;
	}
	else
	{
		// If we get here, then the filenames are not obvious just by looking at the extensions.
		//
		// Instead, we're going to use the size of the files to decide what is the .cfg, .weights, and .names.  The largest
		// file (MiB) will always be the .weights.  The next one (KiB) will be the configuration.  And the names, if available,
		// is only a few bytes in size.  Because the order of magnitude is so large, the three files should never have the
		// exact same size unless something has gone very wrong.

		m.clear();

		std::map<size_t, std::string> file_size_map;
		for (const auto & filename : all_filenames)
		{
			struct stat buf;
			buf.st_size = 0; // only field we actually use is the size, so initialize it to zero in case the stat() call fails
			stat(filename.c_str(), &buf);
			file_size_map[buf.st_size] = filename;
		}

		if (file_size_map.size() != 3)
		{
			/// @throw std::runtime_error if the size of the files cannot be determined (one or more file does not exist?)
			throw std::runtime_error("cannot access .cfg or .weights file");
		}

		// iterate through the files from smallest up to the largest
		auto iter = file_size_map.begin();

		names_filename		= iter->second;
		m["names"]			= iter->second;
		m[iter->second]		= std::to_string(iter->first) + " bytes";

		iter ++;
		cfg_filename		= iter->second;
		m["cfg"]			= iter->second;
		m[iter->second]		= std::to_string(iter->first) + " bytes";

		iter ++;
		weights_filename	= iter->second;
		m["weights"]		= iter->second;
		m[iter->second]		= std::to_string(iter->first) + " bytes";
	}

	// now that we know which file is which, read the first few bytes or lines to see if it contains what we'd expect

	std::ifstream ifs;

	// look for "[net]" within the first few lines of the .cfg file
	ifs.open(cfg_filename);
	if (ifs.is_open() == false)
	{
		/// @throw std::invalid_argument if the cfg file doesn't exist
		throw std::invalid_argument("failed to open the configuration file " + cfg_filename);
	}
	bool found = false;
	for (size_t line_counter = 0; line_counter < 20; line_counter ++)
	{
		std::string line;
		std::getline(ifs, line);
		if (line.find("[net]") != std::string::npos)
		{
			found = true;
			break;
		}
	}
	if (not found)
	{
		/// @throw std::invalid_argument if the cfg file doesn't contain @p "[net]" near the top of the file
		throw std::invalid_argument("failed to find [net] section in configuration file " + cfg_filename);
	}

	// keep looking until we find "classes=###" so we know how many lines the .names file should have
	int number_of_classes = 0;
	const std::regex rx("^classes[ \t]*=[ \t]*([0-9]+)");
	while (ifs.good())
	{
		std::string line;
		std::getline(ifs, line);
		std::smatch sm;
		if (std::regex_search(line, sm, rx))
		{
			m["number of classes"] = sm[1].str();
			number_of_classes = std::stoi(sm[1].str());
			break;
		}
	}

	if (number_of_classes < 1)
	{
		/// @throw std::invalid_argument if the configuration file does not have a line that says "classes=..."
		throw std::invalid_argument("failed to find the number of classes in the configuration file " + cfg_filename);
	}

	// first 4 fields in the weights file -- see save_weights_upto() in darknet's src/parser.c
	ifs.close();
	ifs.open(weights_filename, std::ifstream::in | std::ifstream::binary);
	if (ifs.is_open() == false)
	{
		/// @throw std::invalid_argument if the weights file doesn't exist
		throw std::invalid_argument("failed to open the weights file " + weights_filename);
	}
	uint32_t major	= 0;
	uint32_t minor	= 0;
	uint32_t patch	= 0;
	uint64_t seen	= 0;
	ifs.read(reinterpret_cast<char*>(&major	), sizeof(major	));
	ifs.read(reinterpret_cast<char*>(&minor	), sizeof(minor	));
	ifs.read(reinterpret_cast<char*>(&patch	), sizeof(patch	));
	ifs.read(reinterpret_cast<char*>(&seen	), sizeof(seen	));
	m["weights major"	] = std::to_string(major);
	m["weights minor"	] = std::to_string(minor);
	m["weights patch"	] = std::to_string(patch);
	m["images seen"		] = std::to_string(seen);

	if (major * 10 + minor < 2)
	{
		/// @throw std::invalid_argument if weights file has an invalid version number (or weights file is from an extremely old version of darknet?)
		throw std::invalid_argument("failed to find the version number in the weights file " + weights_filename);
	}

	if (names_filename.empty() == false)
	{
		ifs.close();
		ifs.open(names_filename);
		std::string line;
		int line_counter = 0;
		while (std::getline(ifs, line))
		{
			line_counter ++;
		}
		m["number of names"] = std::to_string(line_counter);

		if (line_counter != number_of_classes)
		{
			/// @throw std::runtime_error if the number of lines in the names file doesn't match the number of classes in the configuration file
			throw std::runtime_error("the network configuration defines " + std::to_string(number_of_classes) + " classes, but the file " + names_filename + " has " + std::to_string(line_counter) + " lines");
		}
	}

	return m;
}


size_t DarkHelp::edit_cfg_file(const std::string & cfg_filename, DarkHelp::MStr m)
{
	if (m.empty())
	{
		// nothing to do!
		return 0;
	}

	std::ifstream ifs(cfg_filename);
	if (not ifs.is_open())
	{
		/// @throw std::invalid_argument if the cfg file does not exist or cannot be opened
		throw std::invalid_argument("failed to open the configuration file " + cfg_filename);
	}

	// read the file and look for the [net] section
	bool net_section_found	= false;
	size_t net_idx_start	= 0;
	size_t net_idx_end		= 0;
	VStr v;
	std::string line;
	while (std::getline(ifs, line))
	{
		if (line == "[net]")
		{
			net_idx_start	= v.size();
			net_idx_end		= v.size();
			net_section_found = true;
		}
		else if (line.size() >= 3)
		{
			if (net_section_found == true)
			{
				if (net_idx_end == net_idx_start)
				{
					if (line[0] == '[')
					{
						// we found the start of a new section, so this must mean the end of [net] has been found
						net_idx_end = v.size();
					}
				}
			}
		}

		v.push_back(line);
	}
	ifs.close();

	if (net_idx_start == net_idx_end)
	{
		/// @throw std::runtime_error if a valid start and end to the [net] section wasn't found in the .cfg file
		throw std::runtime_error("failed to properly identify the [net] section in " + cfg_filename);
	}

	// look at every line in the [net] section to see if it matches one of the keys we want to modify
	const std::regex rx(
		"^"				// start of text
		"\\s*"			// consume all whitespace
		"("				// group #1
			"[^#=\\s]+"	// not "#", "=", or whitespace
		")"
		"\\s*"			// consume all whitespace
		"="				// "="
		"\\s*"			// consume all whitespace
		"("				// group #2
			".*"		// old value and any trailing text (e.g., comments)
		")"
		"$"				// end of text
	);

	bool initial_modification = false;
	if (m.size()				== 2	and
		m.count("batch")		== 1	and
		m.count("subdivisions")	== 1	and
		m["batch"]				== "1"	and
		m["subdivisions"]		== "1"	)
	{
		// we need to know if this is the initial batch/subdivisions modification performed by init()
		// because there are cases were we'll need to abort modifying the .cfg file if this is the case
		initial_modification = true;
	}

	size_t number_of_changed_lines = 0;
	for (size_t idx = net_idx_start; idx < net_idx_end; idx ++)
	{
		std::string & line = v[idx];

		std::smatch sm;
		if (std::regex_match(line, sm, rx))
		{
			const std::string key = sm[1].str();
			const std::string val = sm[2].str();

			if (key						== "contrastive"	and
				val						== "1"				and
				initial_modification	== true				)
			{
				// this is one of the new configuration files that uses "contrastive", so don't modify "batch" and "subdivisions" so we
				// can avoid the darknet error about "mini_batch size (batch/subdivisions) should be higher than 1 for Contrastive loss"
				return 0;
			}

			// now see if this key is one of the ones we want to modify
			if (m.count(key) == 1)
			{
				if (val != m.at(key))
				{
					line = key + "=" + m.at(key);
					number_of_changed_lines ++;
				}
				m.erase(key);
			}
		}
	}

	// whatever is left in the map at this point needs to be inserted at the end of the [net] section (must be a new key)
	for (auto iter : m)
	{
		const std::string & key = iter.first;
		const std::string & val = iter.second;
		const std::string line = key + "=" + val;

		v.insert(v.begin() + net_idx_end, line);
		number_of_changed_lines ++;
		net_idx_end ++;
	}

	if (number_of_changed_lines == 0)
	{
		// nothing to do, no need to re-write the .cfg file
		return 0;
	}

	// now we need to re-create the .cfg file
	const std::string tmp_filename = cfg_filename + "_TMP";
	std::ofstream ofs(tmp_filename);
	if (not ofs.is_open())
	{
		/// @throw std::runtime_error if we cannot write a new .cfg file
		throw std::runtime_error("failed to save changes to .cfg file " + tmp_filename);
	}
	for (const auto & line : v)
	{
		ofs << line << std::endl;
	}
	ofs.close();
	const int result = std::rename(tmp_filename.c_str(), cfg_filename.c_str());
	if (result)
	{
		/// @throw std::runtime_error if we cannot rename the .cfg file
		throw std::runtime_error("failed to overwrite .cfg file " + cfg_filename);
	}

	return number_of_changed_lines;
}


DarkHelp::PredictionResults DarkHelp::predict_internal(cv::Mat mat, const float new_threshold)
{
	// this method is private and cannot be called directly -- instead, see predict()

	prediction_results.clear();
	original_image		= mat;
	annotated_image		= cv::Mat();
	horizontal_tiles	= 1;
	vertical_tiles		= 1;
	tile_size			= cv::Size(0, 0);

	if (net == nullptr)
	{
		/// @throw std::logic_error if the network is invalid.
		throw std::logic_error("cannot predict with an empty network");
	}

	if (original_image.empty())
	{
		/// @throw std::logic_error if the image is invalid.
		throw std::logic_error("cannot predict with an empty image");
	}

	if (new_threshold >= 0.0)
	{
		threshold = new_threshold;
	}
	if (threshold > 1.0)
	{
		// user has probably specified percentages, so bring it back down to a range between 0.0 and 1.0
		threshold /= 100.0;
	}
	if (threshold < 0.0)
	{
		threshold = 0.1;
	}
	if (threshold > 1.0)
	{
		threshold = 1.0;
	}

	network * nw = reinterpret_cast<network*>(net);

	cv::Mat resized_image;
	cv::resize(original_image, resized_image, cv::Size(nw->w, nw->h));
	tile_size = cv::Size(resized_image.cols, resized_image.rows);
	image img = convert_opencv_mat_to_darknet_image(resized_image);

	float * X = img.data;

	const auto t1 = std::chrono::high_resolution_clock::now();
	network_predict(*nw, X);
	const auto t2 = std::chrono::high_resolution_clock::now();
	duration = t2 - t1;

	int nboxes = 0;
	const int use_letterbox = 0;
	auto darknet_results = get_network_boxes(nw, original_image.cols, original_image.rows, threshold, hierarchy_threshold, 0, 1, &nboxes, use_letterbox);

	if (non_maximal_suppression_threshold)
	{
		auto nw_layer = nw->layers[nw->n - 1];
		do_nms_sort(darknet_results, nboxes, nw_layer.classes, non_maximal_suppression_threshold);
	}

	for (int detection_idx = 0; detection_idx < nboxes; detection_idx ++)
	{
		auto & det = darknet_results[detection_idx];

		if (names.empty())
		{
			// we weren't given a names file to parse, but we know how many classes are defined in the network
			// so we can invent a few dummy names to use based on the class index
			for (int i = 0; i < det.classes; i++)
			{
				names.push_back("#" + std::to_string(i));
			}
		}

		/* The "det" object has an array called det.prob[].  That array is large enough for 1 entry per class in the network.
		 * Each entry will be set to 0.0, except for the ones that correspond to the class that was detected.  Note that it
		 * is possible that multiple entries are non-zero!  We need to look at every entry and remember which ones are set.
		 */

		PredictionResult pr;
		pr.tile				= 0;
		pr.best_class		= 0;
		pr.best_probability	= 0.0f;

		for (int class_idx = 0; class_idx < det.classes; class_idx ++)
		{
			if (det.prob[class_idx] >= threshold)
			{
				// remember this probability since it is higher than the threshold
				pr.all_probabilities[class_idx] = det.prob[class_idx];

				// see if this is the highest probability we've seen
				if (det.prob[class_idx] > pr.best_probability)
				{
					pr.best_class		= class_idx;
					pr.best_probability	= det.prob[class_idx];
				}
			}
		}

		if (pr.best_probability >= threshold)
		{
			// at least 1 class is beyond the threshold, so remember this object

			if (fix_out_of_bound_values)
			{
				if (det.bbox.x - det.bbox.w/2.0f < 0.0f ||	// too far left
					det.bbox.x + det.bbox.w/2.0f > 1.0f)	// too far right
				{
					// calculate a new X and width to use for this prediction
					const float new_x1 = std::max(0.0f, det.bbox.x - det.bbox.w/2.0f);
					const float new_x2 = std::min(1.0f, det.bbox.x + det.bbox.w/2.0f);
					const float new_w = new_x2 - new_x1;
					const float new_x = (new_x1 + new_x2) / 2.0f;
					det.bbox.x = new_x;
					det.bbox.w = new_w;
				}
				if (det.bbox.y - det.bbox.h/2.0f < 0.0f ||	// too far above
					det.bbox.y + det.bbox.h/2.0f > 1.0f)	// too far below
				{
					// calculate a new Y and height to use for this prediction
					const float new_y1 = std::max(0.0f, det.bbox.y - det.bbox.h/2.0f);
					const float new_y2 = std::min(1.0f, det.bbox.y + det.bbox.h/2.0f);
					const float new_h = new_y2 - new_y1;
					const float new_y = (new_y1 + new_y2) / 2.0f;
					det.bbox.y = new_y;
					det.bbox.h = new_h;
				}
			}

			const int w = std::round(det.bbox.w * original_image.cols);
			const int h = std::round(det.bbox.h * original_image.rows);
			const int x = std::round(det.bbox.x * original_image.cols - w/2.0);
			const int y = std::round(det.bbox.y * original_image.rows - h/2.0);

			pr.rect				= cv::Rect(cv::Point(x, y), cv::Size(w, h));
			pr.original_point	= cv::Point2f(det.bbox.x, det.bbox.y);
			pr.original_size	= cv::Size2f(det.bbox.w, det.bbox.h);

			// now we come up with a decent name to use for this object
			name_prediction(pr);

			prediction_results.push_back(pr);
		}
	}

	if (sort_predictions == ESort::kAscending)
	{
		std::sort(prediction_results.begin(), prediction_results.end(),
				  [](const PredictionResult & lhs, const PredictionResult & rhs)
				  {
					  return lhs.best_probability < rhs.best_probability;
				  } );
	}
	else if (sort_predictions == ESort::kDescending)
	{
		std::sort(prediction_results.begin(), prediction_results.end(),
				  [](const PredictionResult & lhs, const PredictionResult & rhs)
				  {
					  return rhs.best_probability < lhs.best_probability;
				  } );
	}

	free_detections(darknet_results, nboxes);
	free_image(img);

	return prediction_results;
}


DarkHelp & DarkHelp::name_prediction(PredictionResult & pred)
{
	pred.best_class = 0;
	pred.best_probability = 0.0f;

	for (auto iter : pred.all_probabilities)
	{
		const auto & key = iter.first;
		const auto & val = iter.second;

		if (val > pred.best_probability)
		{
			pred.best_class			= key;
			pred.best_probability	= val;
		}
	}

	pred.name = names.at(pred.best_class);
	if (names_include_percentage)
	{
		const int percentage = std::round(100.0 * pred.best_probability);
		pred.name += " " + std::to_string(percentage) + "%";
	}

	if (include_all_names and pred.all_probabilities.size() > 1)
	{
		// we have multiple probabilities!
		for (auto iter : pred.all_probabilities)
		{
			const int & key = iter.first;
			if (key != pred.best_class)
			{
				pred.name += ", " + names.at(key);
				if (names_include_percentage)
				{
					const int percentage = std::round(100.0 * iter.second);
					pred.name += " " + std::to_string(percentage) + "%";
				}
			}
		}
	}

	return *this;
}


std::ostream & operator<<(std::ostream & os, const DarkHelp::PredictionResult & pred)
{
	os	<< "\""			<< pred.name << "\""
		<< " #"			<< pred.best_class
		<< " prob="		<< pred.best_probability
		<< " x="		<< pred.rect.x
		<< " y="		<< pred.rect.y
		<< " w="		<< pred.rect.width
		<< " h="		<< pred.rect.height
		<< " tile="		<< pred.tile
		<< " entries="	<< pred.all_probabilities.size()
		;

	if (pred.all_probabilities.size() > 1)
	{
		os << " [";
		for (auto iter : pred.all_probabilities)
		{
			const auto & key = iter.first;
			const auto & val = iter.second;
			os << " " << key << "=" << val;
		}
		os << " ]";
	}

	return os;
}


std::ostream & operator<<(std::ostream & os, const DarkHelp::PredictionResults & results)
{
	const size_t number_of_results = results.size();
	os << "prediction results: " << number_of_results;

	for (size_t idx = 0; idx < number_of_results; idx ++)
	{
		os << std::endl << "-> " << (idx+1) << "/" << number_of_results << ": ";
		operator<<(os, results.at(idx));
	}

	return os;
}


cv::Mat resize_keeping_aspect_ratio(cv::Mat mat, const cv::Size & desired_size)
{
	if (mat.empty())
	{
		return mat;
	}

	if (desired_size.width == mat.cols and desired_size.height == mat.rows)
	{
		return mat;
	}

	if (desired_size.width < 1 or desired_size.height < 1)
	{
		// return an empty image
		return cv::Mat();
	}

	const double image_width		= static_cast<double>(mat.cols);
	const double image_height		= static_cast<double>(mat.rows);
	const double horizontal_factor	= image_width	/ static_cast<double>(desired_size.width);
	const double vertical_factor	= image_height	/ static_cast<double>(desired_size.height);
	const double largest_factor 	= std::max(horizontal_factor, vertical_factor);
	const double new_width			= image_width	/ largest_factor;
	const double new_height			= image_height	/ largest_factor;
	const cv::Size new_size(std::round(new_width), std::round(new_height));

	// "To shrink an image, it will generally look best with CV_INTER_AREA interpolation ..."
	auto interpolation = CV_INTER_AREA;
	if (largest_factor < 1.0)
	{
		// "... to enlarge an image, it will generally look best with CV_INTER_CUBIC"
		interpolation = CV_INTER_CUBIC;
	}

	cv::Mat dst;
	cv::resize(mat, dst, new_size, 0, 0, interpolation);

	return dst;
}
