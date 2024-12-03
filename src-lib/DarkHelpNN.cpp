/* DarkHelp - C++ helper class for Darknet's C API.
 * Copyright 2019-2024 Stephane Charette <stephanecharette@gmail.com>
 * MIT license applies.  See "license.txt" for details.
 */

#define DARKNET_INCLUDE_ORIGINAL_API
#include <darknet.hpp>

#include "DarkHelp.hpp"

#include <fstream>
#include <regex>
#include <cmath>
#include <ctime>
#include <sys/stat.h>

#ifdef HAVE_OPENCV_CUDAWARPING
#include <opencv2/cudawarping.hpp>
#endif

#ifdef WIN32
#pragma warning(disable: 4267)
#pragma warning(disable: 4244)
#pragma warning(disable: 4305)
#endif


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


DarkHelp::NN::~NN()
{
	reset();

	return;
}


DarkHelp::NN::NN() :
	darknet_net(nullptr),
	number_of_channels(-1)
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
	 *
	 * On 2022-01-28, the same output looks like this:
	 *		sizeof(network) ........ 656	<--
	 *		sizeof(network_state) .. 696	<--
	 *		sizeof(layer) .......... 2616	<--
	 *		sizeof(image) .......... 24
	 *		sizeof(detection) ...... 88		<--
	 *		sizeof(load_args) ...... 240	<--
	 *		sizeof(data) ........... 64
	 *		sizeof(metadata) ....... 16
	 *		sizeof(tree) ........... 72
	 *
	 * This shows that the darknet structures are changing in size over time, and why it is so important to recompile
	 * DarkHelp when Darknet gets updated.
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


DarkHelp::NN::NN(const DarkHelp::Config & cfg) :
	NN()
{
	config = cfg;

	init();

	return;
}


DarkHelp::NN::NN(const std::string & cfg_filename, const std::string & weights_filename, const std::string & names_filename, const bool verify_files_first, const EDriver d) :
	NN()
{
	init(cfg_filename, weights_filename, names_filename, verify_files_first, d);

	return;
}


DarkHelp::NN::NN(const char * cfg_filename, const char * weights_filename, const char * names_filename, const bool verify_files_first, const EDriver driver) :
	NN(std::string(cfg_filename), std::string(weights_filename), std::string(names_filename), verify_files_first, driver)
{
	return;
}


DarkHelp::NN::NN(const bool delete_combined_bundle_once_loaded, const std::string & filename, const std::string & key, const EDriver d) :
	NN()
{
	init(delete_combined_bundle_once_loaded, filename, key, d);

	return;
}


DarkHelp::NN & DarkHelp::NN::init(const bool delete_combined_bundle_once_loaded, const std::string & filename, const std::string & key, const EDriver driver)
{
	std::filesystem::path cfg_filename;
	std::filesystem::path names_filename;
	std::filesystem::path weights_filename;

	auto cleanup = [&]()
	{
		if (not cfg_filename	.empty()) std::filesystem::remove(cfg_filename);
		if (not names_filename	.empty()) std::filesystem::remove(names_filename);
		if (not weights_filename.empty()) std::filesystem::remove(weights_filename);

		if (delete_combined_bundle_once_loaded)
		{
			std::filesystem::remove(filename);
		}
	};

	try
	{
		DarkHelp::extract(key, filename, cfg_filename, names_filename, weights_filename);
		init(cfg_filename.string(), weights_filename.string(), names_filename.string(), false, driver);

		cleanup();
	}
	catch (...)
	{
		cleanup();

		throw;
	}

	return *this;
}


DarkHelp::NN & DarkHelp::NN::init(const std::string & cfg_filename, const std::string & weights_filename, const std::string & names_filename, const bool verify_files_first, const EDriver d)
{
	config.cfg_filename		= cfg_filename;
	config.weights_filename	= weights_filename;
	config.names_filename	= names_filename;

	// darknet behaves very badly if the .cfg and the .weights files are accidentally swapped.  I've seen it segfault when
	// it attempts to parse the weights as a text flle.  So do a bit of simple verification and swap them if necessary.
	if (verify_files_first)
	{
		verify_cfg_and_weights(config.cfg_filename, config.weights_filename, config.names_filename);
	}

	#ifndef HAVE_OPENCV_DNN_OBJDETECT
		// with old versions of OpenCV, we don't have a DNN module
		config.driver = DarkHelp::EDriver::kDarknet;
	#else
		config.driver = d;
	#endif

	return init();
}


DarkHelp::NN & DarkHelp::NN::init()
{
	if (config.cfg_filename.empty() or
		config.weights_filename.empty())
	{
		/// @throw std::invalid_argument if the .cfg or .weights filenames have not been set.
		throw std::invalid_argument("cannot initialize the network without a .cfg or .weights file");
	}

	if (config.modify_batch_and_subdivisions)
	{
		const MStr m =
		{
			{"batch"		, "1"},
			{"subdivisions"	, "1"}
		};
		edit_cfg_file(config.cfg_filename, m);

		// do not combine this settings with the previous two since there is code that
		// needs to behave differently when only the batch+subdivisions are modified
		//
		// 2021-04-08:  It looks like use_cuda_graph _may_ be causing problems.  Don't explicitely set it in DarkHelp.
		//		edit_cfg_file(cfg_fn, {{"use_cuda_graph", "1"}});
	}

	if (config.driver < EDriver::kMin or
		config.driver > EDriver::kMax)
	{
		config.driver = EDriver::kDarknet;
	}

	const auto t1 = std::chrono::high_resolution_clock::now();
	if (config.driver == EDriver::kDarknet)
	{
		// The calls we make into darknet are based on what was found in test_detector() from src/detector.c.

		if (config.redirect_darknet_output)
		{
			toggle_output_redirection();
		}

		darknet_net = load_network_custom(const_cast<char*>(config.cfg_filename.c_str()), const_cast<char*>(config.weights_filename.c_str()), 1, 1);

		if (config.redirect_darknet_output)
		{
			toggle_output_redirection();
		}

		if (darknet_net == nullptr)
		{
			/// @throw std::runtime_error if the call to darknet's @p load_network_custom() has failed.
			throw std::runtime_error("darknet failed to load the configuration, the weights, or both");
		}

		Darknet::NetworkPtr nw = reinterpret_cast<Darknet::NetworkPtr>(darknet_net);

		// what does this call do?
		calculate_binary_weights(nw);
	}
#if CV_VERSION_MAJOR >= 4 && defined(HAVE_OPENCV_DNN_OBJDETECT)
	else
	{
		opencv_net = cv::dnn::readNetFromDarknet(config.cfg_filename, config.weights_filename);

#if CV_VERSION_MAJOR > 4 || (CV_VERSION_MAJOR == 4 && CV_VERSION_MINOR >= 2)
		if (config.driver == EDriver::kOpenCVCPU)
		{
			opencv_net.setPreferableBackend(cv::dnn::DNN_BACKEND_OPENCV);
			opencv_net.setPreferableTarget(cv::dnn::DNN_TARGET_CPU);
		}
		else
		{
			opencv_net.setPreferableBackend(cv::dnn::DNN_BACKEND_CUDA);
			opencv_net.setPreferableTarget(cv::dnn::DNN_TARGET_CUDA);
		}
#else
		opencv_net.setPreferableBackend(cv::dnn::DNN_BACKEND_OPENCV);
		opencv_net.setPreferableTarget(cv::dnn::DNN_TARGET_CPU);
#endif
	}
#endif

	if (not config.names_filename.empty())
	{
		std::ifstream ifs(config.names_filename);
		std::string line;
		while (std::getline(ifs, line))
		{
			// truncate leading/trailing whitespace
			// (helps deal with CRLF when .names was edited on Windows)

			auto p = line.find_last_not_of(" \t\r\n");
			if (p != std::string::npos)
			{
				line.erase(p + 1);
			}

			p = line.find_first_not_of(" \t\r\n");
			if (p == std::string::npos)
			{
				/// @throw std::invalid_argument if there is a blank line in the .names file.
				throw std::runtime_error("unexpected blank line detected at " + config.names_filename + " line #" + std::to_string(names.size() + 1));
			}
			line.erase(0, p);

			names.push_back(line);
		}
	}

	// see which classes need to be suppressed (https://github.com/AlexeyAB/darknet/issues/2122)
	for (size_t i = 0; i < names.size(); i ++)
	{
		if (names.at(i).find("dont_show") == 0)
		{
			config.annotation_suppress_classes.insert(i);
		}
	}

	// cache the network network_dimensions (read the "width" and "height" from the .cfg file)
	network_dimensions = cv::Size(0, 0);
	number_of_channels = -1;
	const std::regex rx("^\\s*(channels|width|height)\\s*=\\s*(\\d+)");
	std::ifstream ifs(config.cfg_filename);
	while (ifs.good() and (network_dimensions.area() <= 0 or number_of_channels <= 0))
	{
		std::string line;
		std::getline(ifs, line);
		std::smatch sm;
		if (std::regex_search(line, sm, rx))
		{
			const std::string key = sm.str(1);
			const int value = std::stoi(sm.str(2));
			if (key == "width")
			{
				network_dimensions.width = value;
			}
			else if (key == "height")
			{
				network_dimensions.height = value;
			}
			else
			{
				number_of_channels = value;
			}
		}
	}

	if (network_dimensions.area() <= 0)
	{
		/// @throw std::invalid_argument if the network dimensions cannot be read from the .cfg file
		throw std::invalid_argument("failed to read the network width or height from " + config.cfg_filename);
	}

	if (number_of_channels != 1 and number_of_channels != 3)
	{
		/// @throw std::invalid_argument if the @p channels=... line in the .cfg file is not 1 or 3
		throw std::invalid_argument("invalid number of channels in " + config.cfg_filename);
	}

	// OpenCV's construction uses lazy initialization, and doesn't actually happen until we call into it.
	// This can have a huge impact on FPS calculations when the initial image pauses for a "long" time as
	// the network is loaded.  So pass a "dummy" image through the network to force everything to load.
	if (config.driver != DarkHelp::EDriver::kDarknet)
	{
		cv::Mat mat;
		if (number_of_channels == 1)
		{
			mat = cv::Mat(network_dimensions, CV_8UC1, cv::Scalar(0));
		}
		else
		{
			mat = cv::Mat(network_dimensions, CV_8UC3, cv::Scalar(0, 0, 0));
		}

		predict_internal(mat);
		clear();
	}

	const auto t2 = std::chrono::high_resolution_clock::now();
	duration = t2 - t1;

	return *this;
}


DarkHelp::NN & DarkHelp::NN::reset()
{
	if (darknet_net)
	{
		Darknet::NetworkPtr nw = reinterpret_cast<Darknet::NetworkPtr>(darknet_net);
		free_network_ptr(nw);
		free(darknet_net); // this was calloc()'d in load_network_custom()
		darknet_net = nullptr;
	}

	#ifdef HAVE_OPENCV_DNN_OBJDETECT
		opencv_net = cv::dnn::Net();
	#endif

	clear();
	names.clear();
	network_dimensions = {0, 0};

	config.reset();

	return *this;
}


DarkHelp::NN & DarkHelp::NN::clear()
{
	prediction_results		.clear();
	original_image			= cv::Mat();
	binary_inverted_image	= cv::Mat();
	annotated_image			= cv::Mat();
	horizontal_tiles		= 1;
	vertical_tiles			= 1;
	tile_size				= cv::Size(0, 0);

	return *this;
}


bool DarkHelp::NN::is_initialized() const
{
	if (config.driver == EDriver::kDarknet and darknet_net == nullptr)
	{
		return false;
	}

	if (names.empty())
	{
		return false;
	}

	if (network_dimensions.area() <= 0)
	{
		return false;
	}

	return true;
}


bool DarkHelp::NN::empty() const
{
	return prediction_results.empty() and original_image.empty();
}


DarkHelp::PredictionResults DarkHelp::NN::predict(const std::string & image_filename, const float new_threshold)
{
	cv::Mat mat = cv::imread(image_filename);
	if (mat.empty())
	{
		/// @throw std::invalid_argument if the image failed to load.
		throw std::invalid_argument("failed to load image \"" + image_filename + "\"");
	}

	return predict(mat, new_threshold);
}


DarkHelp::PredictionResults DarkHelp::NN::predict(cv::Mat mat, const float new_threshold)
{
	if (mat.empty())
	{
		/// @throw std::invalid_argument if the image is empty.
		throw std::invalid_argument("cannot predict with an empty OpenCV image");
	}

	if (config.enable_tiles)
	{
		return predict_tile(mat, new_threshold);
	}

	return predict_internal(mat, new_threshold);
}


#ifdef DARKHELP_CAN_INCLUDE_DARKNET
DarkHelp::PredictionResults DarkHelp::NN::predict(image img, const float new_threshold)
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


DarkHelp::PredictionResults DarkHelp::NN::predict_tile(cv::Mat mat, const float new_threshold)
{
	if (mat.empty())
	{
		/// @throw std::invalid_argument if the image is empty.
		throw std::invalid_argument("cannot predict with an empty OpenCV image");
	}

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
				if (config.combine_tile_predictions)
				{
					const int minimum_horizontal_distance	= config.tile_edge_factor * prediction.rect.width;
					const int minimum_vertical_distance		= config.tile_edge_factor * prediction.rect.height;
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

				if (config.enable_debug)
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
					// this item has already been consumed and is marked for deletion
					continue;
				}

				if (config.only_combine_similar_predictions)
				{
					// check the probabilities to see if there is any similarity:
					//
					// 1) does the LHS contain the best class from the RHS?
					// 2) does the RHS contain the best class from the LHS?
					//
					if (results[lhs_idx].all_probabilities.count(results[rhs_idx].best_class) == 0 and
						results[rhs_idx].all_probabilities.count(results[lhs_idx].best_class) == 0)
					{
						// the two objects have completely different classes, so we cannot combine them together
						continue;
					}
				}

				const cv::Rect & rhs_rect		= results[rhs_idx].rect;
				const cv::Rect combined_rect	= lhs_rect | rhs_rect;

				// if this is a good match, then the area of the combined rect will be similar to the area of lhs+rhs
				const int lhs_area		= lhs_rect		.area();
				const int rhs_area		= rhs_rect		.area();
				const int lhs_plus_rhs	= (lhs_area + rhs_area) * config.tile_rect_factor;
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

	if (config.enable_debug)
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

	original_image			= mat;
	binary_inverted_image	= cv::Mat();
	prediction_results		= results;
	duration				= total_duration;
	horizontal_tiles		= horizontal_tiles_count;
	vertical_tiles			= vertical_tiles_count;
	tile_size				= new_tile_size;

	return results;
}


cv::Mat DarkHelp::NN::annotate(const float new_threshold)
{
	if (original_image.empty())
	{
		/// @throw std::logic_error if an attempt is made to annotate an empty image
		throw std::logic_error("cannot annotate an empty image; must call predict() first");
	}

	if (new_threshold >= 0.0)
	{
		config.threshold = new_threshold;
	}

	annotated_image = original_image.clone();

	if (config.annotation_pixelate_enabled)
	{
		pixelate_rectangles(original_image, annotated_image, prediction_results, config.annotation_pixelate_classes, config.annotation_pixelate_size);
	}

	// make sure we always have colours we can use
	if (config.annotation_colours.empty())
	{
		config.annotation_colours = get_default_annotation_colours();
	}

	for (const auto & pred : prediction_results)
	{
		if (config.annotation_suppress_classes.count(pred.best_class) != 0)
		{
			continue;
		}

		if (config.annotation_line_thickness > 0 and pred.best_probability >= config.threshold)
		{
			const auto colour = config.annotation_colours[pred.best_class % config.annotation_colours.size()];

			int line_thickness_or_fill = config.annotation_line_thickness;
			if (config.annotation_shade_predictions >= 1.0)
			{
				line_thickness_or_fill = CV_FILLED;
			}
			else if (config.annotation_shade_predictions > 0.0)
			{
				cv::Mat roi = annotated_image(pred.rect);
				cv::Mat coloured_rect(roi.size(), roi.type(), colour);

				const double alpha = config.annotation_shade_predictions;
				const double beta = 1.0 - alpha;
				cv::addWeighted(coloured_rect, alpha, roi, beta, 0.0, roi);
			}

//			std::cout << "class id=" << pred.best_class << ", probability=" << pred.best_probability << ", point=(" << pred.rect.x << "," << pred.rect.y << "), name=\"" << pred.name << "\", duration=" << duration_string() << std::endl;
			cv::rectangle(annotated_image, pred.rect, colour, line_thickness_or_fill);

			if (config.annotation_suppress_all_labels)
			{
				continue;
			}

			int baseline = 0;
			const cv::Size text_size = cv::getTextSize(pred.name, config.annotation_font_face, config.annotation_font_scale, config.annotation_font_thickness, &baseline);

			if (config.annotation_auto_hide_labels)
			{
				if (text_size.width >= pred.rect.width or
					text_size.height >= pred.rect.height)
				{
					// label is too large to display
					continue;
				}
			}

			cv::Rect r(cv::Point(pred.rect.x - config.annotation_line_thickness/2, pred.rect.y - text_size.height - baseline + config.annotation_line_thickness), cv::Size(text_size.width + config.annotation_line_thickness, text_size.height + baseline));
			if (r.x < 0) r.x = 0;																			// shift the label to the very left edge of the screen, otherwise it would be off-screen
			if (r.x + r.width >= annotated_image.cols) r.x = pred.rect.x + pred.rect.width - r.width + 1;	// first attempt at pushing the label to the left
			if (r.x + r.width >= annotated_image.cols) r.x = annotated_image.cols - r.width;				// more drastic attempt at pushing the label to the left

			if (r.y < 0) r.y = pred.rect.y + pred.rect.height;	// shift the label to the bottom of the prediction, otherwise it would be off-screen
			if (r.y + r.height >= annotated_image.rows) r.y = pred.rect.y + 1; // shift the label to the inside-top of the prediction (CV seems to have trouble drawing text where the upper bound is y=0, so move it down 1 pixel)
			if (r.y < 0) r.y = 0; // shift the label to the top of the image if it is off-screen

			cv::rectangle(annotated_image, r, colour, CV_FILLED);
			cv::putText(annotated_image, pred.name, cv::Point(r.x + config.annotation_line_thickness/2, r.y + text_size.height), config.annotation_font_face, config.annotation_font_scale, cv::Scalar(0,0,0), config.annotation_font_thickness, CV_AA);
		}
	}

	if (config.annotation_include_duration)
	{
		const std::string str		= duration_string();
		const cv::Size text_size	= cv::getTextSize(str, config.annotation_font_face, config.annotation_font_scale, config.annotation_font_thickness, nullptr);

		cv::Rect r(cv::Point(2, 2), cv::Size(text_size.width + 2, text_size.height + 2));
		cv::rectangle(annotated_image, r, cv::Scalar(255,255,255), CV_FILLED);
		cv::putText(annotated_image, str, cv::Point(r.x + 1, r.y + text_size.height), config.annotation_font_face, config.annotation_font_scale, cv::Scalar(0,0,0), config.annotation_font_thickness, CV_AA);
	}

	if (config.annotation_include_timestamp)
	{
		const std::time_t tt = std::time(nullptr);
		char timestamp[100];
		strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", std::localtime(&tt));

		const cv::Size text_size = cv::getTextSize(timestamp, config.annotation_font_face, config.annotation_font_scale, config.annotation_font_thickness, nullptr);

		cv::Rect r(cv::Point(2, annotated_image.rows - text_size.height - 4), cv::Size(text_size.width + 2, text_size.height + 2));
		cv::rectangle(annotated_image, r, cv::Scalar(255,255,255), CV_FILLED);
		cv::putText(annotated_image, timestamp, cv::Point(r.x + 1, r.y + text_size.height), config.annotation_font_face, config.annotation_font_scale, cv::Scalar(0,0,0), config.annotation_font_thickness, CV_AA);
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


std::string DarkHelp::NN::duration_string()
{
	return DarkHelp::duration_string(duration);
}


cv::Size DarkHelp::NN::network_size()
{
	// This used to be more complicated, but now we get and cache
	// the network dimensions when the DarkHelp object is initialized.
	return network_dimensions;
}


int DarkHelp::NN::image_channels()
{
	return number_of_channels;
}


DarkHelp::PredictionResults DarkHelp::NN::predict_internal(cv::Mat mat, const float new_threshold)
{
	// this method is private and cannot be called directly -- instead, see predict()

	clear();
	original_image = mat;

	if (config.driver == EDriver::kInvalid)
	{
		/// @throw std::logic_error if the %DarkHelp object has not been initialized.
		throw std::logic_error("cannot predict with an uninitialized object");
	}

	if (config.driver == EDriver::kDarknet and darknet_net == nullptr)
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
		config.threshold = new_threshold;
	}
	if (config.threshold > 1.0)
	{
		// user has probably specified percentages, so bring it back down to a range between 0.0 and 1.0
		config.threshold /= 100.0;
	}
	if (config.threshold < 0.0)
	{
		config.threshold = 0.1;
	}
	if (config.threshold > 1.0)
	{
		config.threshold = 1.0;
	}

	const auto t1 = std::chrono::high_resolution_clock::now();

	if (config.driver == EDriver::kDarknet)
	{
		predict_internal_darknet();
	}
	else
	{
		predict_internal_opencv();
	}

	if (config.sort_predictions == ESort::kAscending)
	{
		std::sort(prediction_results.begin(), prediction_results.end(),
				[](const PredictionResult & lhs, const PredictionResult & rhs)
				{
					return lhs.best_probability < rhs.best_probability;
				} );
	}
	else if (config.sort_predictions == ESort::kDescending)
	{
		std::sort(prediction_results.begin(), prediction_results.end(),
				[](const PredictionResult & lhs, const PredictionResult & rhs)
				{
					return rhs.best_probability < lhs.best_probability;
				} );
	}
	else if (config.sort_predictions == ESort::kPageOrder)
	{
		std::sort(prediction_results.begin(), prediction_results.end(),
				[](const PredictionResult & lhs, const PredictionResult & rhs)
				{
					const int lhs_y = std::round(10.0f * lhs.original_point.y);
					const int rhs_y = std::round(10.0f * rhs.original_point.y);
					if (lhs_y != rhs_y) return lhs_y < rhs_y;

					const int lhs_x = std::round(10.0f * lhs.original_point.x);
					const int rhs_x = std::round(10.0f * rhs.original_point.x);
					if (lhs_x != rhs_x) return lhs_x < rhs_x;

					return lhs.best_probability < rhs.best_probability;
				} );
	}

	if (config.snapping_enabled)
	{
		snap_annotations();
	}

	const auto t2 = std::chrono::high_resolution_clock::now();
	duration = t2 - t1;

	return prediction_results;
}


void DarkHelp::NN::predict_internal_darknet()
{
	Darknet::NetworkPtr nw = reinterpret_cast<Darknet::NetworkPtr>(darknet_net);

	cv::Mat resized_image;
	if (config.use_fast_image_resize)
	{
		resized_image = fast_resize_ignore_aspect_ratio(original_image, network_dimensions);
	}
	else
	{
		resized_image = slow_resize_ignore_aspect_ratio(original_image, network_dimensions);
	}

	tile_size = network_dimensions;
	DarknetImage img = convert_opencv_mat_to_darknet_image(resized_image);

	network_predict_ptr(nw, img.data);

	int nboxes = 0;
	const int use_letterbox = 0;
	auto darknet_results = get_network_boxes(nw, original_image.cols, original_image.rows, config.threshold, config.hierarchy_threshold, 0, 1, &nboxes, use_letterbox);

	if (config.non_maximal_suppression_threshold)
	{
		do_nms_sort(darknet_results, nboxes, names.size(), config.non_maximal_suppression_threshold);
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
			if (det.prob[class_idx] >= config.threshold)
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

		if (pr.best_probability >= config.threshold)
		{
			// at least 1 class is beyond the threshold, so remember this object

			if (config.fix_out_of_bound_values)
			{
				fix_out_of_bound_normalized_rect(det.bbox.x, det.bbox.y, det.bbox.w, det.bbox.h);
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

	free_detections(darknet_results, nboxes);
	free_image(img);

	return;
}


void DarkHelp::NN::predict_internal_opencv()
{
	#ifndef HAVE_OPENCV_DNN_OBJDETECT
	throw std::runtime_error("OpenCV DNN driver is not supported with this version of OpenCV");
	#else

	const size_t number_of_classes = names.size();

	cv::Mat resized_image;
	if (config.use_fast_image_resize)
	{
		resized_image = fast_resize_ignore_aspect_ratio(original_image, network_dimensions);
	}
	else
	{
		resized_image = slow_resize_ignore_aspect_ratio(original_image, network_dimensions);
	}

	tile_size = network_dimensions;

	/* OpenCV images are BGR, but DNN (or maybe specific to Darknet?) requires RGB,
	 * so make sure to set the "swap" option, otherwise detection won't behave as
	 * well as expected.
	 */
	auto blob = cv::dnn::blobFromImage(resized_image, 1.0 / 255.0, network_dimensions, {}, /* swapRB=*/true, /* crop=*/false);
	opencv_net.setInput(blob);

	/* Get the names of all the layers we're interested in (should start with "yolo_").
	 * This is important!  We're going to have to combine the results from all these layers.
	 */
	VStr yolo_layer_names;
	for (const auto & name : opencv_net.getLayerNames())
	{
		if (name.find("yolo_") == 0)
		{
			yolo_layer_names.push_back(name);
		}
	}

	/* The output mat is float and will have thousands of rows.
	 * Each row has the following fields, each of which is a "float":
	 *
	 *		center x (0-1f)
	 *		center y (0-1f)
	 *		width (0-1f)
	 *		hight (0-1f)
	 *		objectness (0-1f)
	 *		% class 1 (0-1f)
	 *		% class 2 (0-1f)
	 *		% class 3 ...etc...
	 *
	 * For every class, another field with the probability for that class.
	 */
	std::vector<std::vector<cv::Mat>> output_mats;
	opencv_net.forward(output_mats, yolo_layer_names);

	/* To get the final output to behave/look as similar as we can to the original
	 * darknet results, we'll need to refer back to the OpenCV results as we build
	 * up the results vector.  For this reason, we need to know where in the matrix
	 * to look up the individual results.
	 */
	struct Lookup
	{
		size_t	idx; // each of the YOLO layer will have an entry in "output_mats"
		int		row; // cv::Mat rows is of type "int", not size_t
	};
	using VLookups = std::vector<Lookup>;

	std::vector<VRect2d>	boxes	(number_of_classes);
	std::vector<VFloat>		scores	(number_of_classes);
	std::vector<VLookups>	lookups	(number_of_classes);

	for (size_t output_idx = 0; output_idx < yolo_layer_names.size(); output_idx ++)
	{
		cv::Mat & output = output_mats[output_idx][0];
		if (config.enable_debug)
		{
			std::cout << "Layer \"" << yolo_layer_names[output_idx] << "\":" << std::endl;
		}

		for (int row = 0; row < output.rows; row ++)
		{
			// get a pointer to the 1st float for this row, which we easily increment to get all the floats
			const float * const ptr = output.ptr<float>(row);

			// [4] is the "objectness"
			if (ptr[4] >= 0.01f)
			{
				// *** DEBUG ***
				if (config.enable_debug)
				{
					std::cout << "i=" << std::setw(4) << row;
					for (size_t offset = 0; offset < number_of_classes + 5; offset ++)
					{
						std::cout
							<< " "
							<< (offset==0 ? "cx" :
								offset==1 ? "cy" :
								offset==2 ? "w" :
								offset==3 ? "h" :
								offset==4 ? "obj" :
								names.at(offset-5).substr(0, 3))
							<< "=";
						if (ptr[offset] != 0.0f)
						{
							std::cout << std::fixed << std::setprecision(8) << ptr[offset];
						}
						else
						{
							std::cout << "0.0       ";
						}
					}
					std::cout << std::endl;
				}
				// *** DEBUG ***

				const float & cx	= ptr[0];
				const float & cy	= ptr[1];
				const float & w		= ptr[2];
				const float & h		= ptr[3];

				const cv::Rect2d r(
					cx - w / 2.0f	,
					cy - h / 2.0f	,
					w				,
					h				);

				for (size_t c = 0; c < number_of_classes; c++)
				{
					const auto & confidence = ptr[5 + c];
					if (confidence >= config.threshold)
					{
						lookups	[c].push_back({output_idx, row});
						boxes	[c].push_back(r);
						scores	[c].push_back(confidence);
					}
				}
			}
		}
	}

	// This is where we run non maximal suppression, which tells us which indices we need to keep.
	// We'll take the output of NMS and keep track of all the mat rows which need to be in the results.
	VLookups rows_of_interest;
	for (size_t c = 0; c < number_of_classes; c++)
	{
		VInt indices;
		cv::dnn::NMSBoxes(boxes[c], scores[c], 0.0, config.non_maximal_suppression_threshold, indices);

		for (const auto & i : indices)
		{
			rows_of_interest.push_back(lookups[c][i]);
		}

		// *** DEBUG ***
		if (config.enable_debug)
		{
			if (boxes[c].size() > 0 or indices.size() > 0)
			{
				std::cout << "-> class #" << c << " (" << names.at(c) << ") contains " << boxes[c].size() << " entries";
				if (indices.size() != boxes[c].size())
				{
					std::cout << " but NMS returned " << indices.size() << " indices";
				}
				std::cout << ":";
				for (const auto & i : indices)
				{
					std::cout << " " << i << "=[" << lookups[c][i].idx << "," << lookups[c][i].row << "]";
				}
				std::cout << std::endl;
			}
		}
		// *** DEBUG ***
	}

	// now iterate through just those indexes returned by NMS and build the DarkHelp-style results
	for (const auto iter : rows_of_interest)
	{
		const auto & output_idx	= iter.idx;
		const auto & row		= iter.row;

		cv::Mat & output = output_mats[output_idx][0];
		float * ptr	= output.ptr<float>(row);

		PredictionResult pr;
		pr.tile				= 0;
		pr.best_class		= 0;
		pr.best_probability	= 0.0f;

		// loop through all of the classes this could be and see if we can find something we can use
		for (size_t c = 0; c < number_of_classes; c++)
		{
			const float & probability = ptr[5 + c];

			if (probability >= config.threshold)
			{
				if (probability > pr.best_probability)
				{
					pr.best_class = c;
					pr.best_probability = probability;
				}

				pr.all_probabilities[c] = probability;
			}
		}

		if (pr.best_probability > 0.0f)
		{
			float & cx	= ptr[0];
			float & cy	= ptr[1];
			float & w	= ptr[2];
			float & h	= ptr[3];

			if (config.fix_out_of_bound_values)
			{
				fix_out_of_bound_normalized_rect(cx, cy, w, h);
			}

			const int new_w = std::round(original_image.cols * w				);
			const int new_h = std::round(original_image.rows * h				);
			const int new_x = std::round(original_image.cols * (cx - w / 2.0f)	);
			const int new_y = std::round(original_image.rows * (cy - h / 2.0f)	);

			pr.rect				= cv::Rect(cv::Point(new_x, new_y), cv::Size(new_w, new_h));
			pr.original_point	= cv::Point2f(cx, cy);
			pr.original_size	= cv::Size2f(w, h);

			name_prediction(pr);

			prediction_results.push_back(pr);
		}
	}

	#endif

	return;
}


DarkHelp::NN & DarkHelp::NN::name_prediction(PredictionResult & pred)
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
	if (config.names_include_percentage)
	{
		const int percentage = std::round(100.0 * pred.best_probability);
		pred.name += " " + std::to_string(percentage) + "%";
	}

	if (config.include_all_names and pred.all_probabilities.size() > 1)
	{
		// we have multiple probabilities!
		for (auto iter : pred.all_probabilities)
		{
			const int & key = iter.first;
			if (key != pred.best_class)
			{
				pred.name += ", " + names.at(key);
				if (config.names_include_percentage)
				{
					const int percentage = std::round(100.0 * iter.second);
					pred.name += " " + std::to_string(percentage) + "%";
				}
			}
		}
	}

	return *this;
}


DarkHelp::NN & DarkHelp::NN::snap_annotations()
{
	for (auto & pred : prediction_results)
	{
		snap_annotation(pred);
	}

	return *this;
}


DarkHelp::NN & DarkHelp::NN::snap_annotation(DarkHelp::PredictionResult & pred)
{
	if (config.snapping_limit_shrink	>= 1.0f and	// cannot shrink
		config.snapping_limit_grow		<= 1.0f)	// cannot grow
	{
		// nothing we can do with snapping, the limits wont let us either shrink nor grow annotations
		return *this;
	}

	if (binary_inverted_image.empty())
	{
		cv::Mat greyscale;
		cv::Mat threshold;
		cv::cvtColor(original_image, greyscale, cv::COLOR_BGR2GRAY);
		cv::adaptiveThreshold(greyscale, threshold, 255, cv::ADAPTIVE_THRESH_GAUSSIAN_C, cv::THRESH_BINARY, config.binary_threshold_block_size, config.binary_threshold_constant);
		binary_inverted_image = ~ threshold;
	}

	const auto original_rect	= pred.rect;
	const float original_area	= original_rect.area();
	auto final_rect				= original_rect;
	bool use_snap				= true;

	if (config.snapping_limit_shrink < 1.0f and config.snapping_limit_grow > 1.0f)
	{
		const int hgrow = 2;
		const int vgrow = 2;

		// start by slightly shrinking the rectangle, and then we'll grow it out if necessary during snapping
		if (final_rect.width >= (5 * hgrow))
		{
			final_rect.x		+= (2 * hgrow);
			final_rect.width	-= (4 * hgrow);
		}

		if (final_rect.height >= (5 * vgrow))
		{
			final_rect.y		+= (2 * vgrow);
			final_rect.height	-= (4 * vgrow);
		}
	}

	int attempt = 0;
	while (true)
	{
		attempt ++;
		const auto horizontal_snap_distance	= std::min(attempt, config.snapping_horizontal_tolerance);
		const auto vertical_snap_distance	= std::min(attempt, config.snapping_vertical_tolerance);

		auto roi = final_rect;
		roi.x		-= (1 * horizontal_snap_distance	);
		roi.y		-= (1 * vertical_snap_distance		);
		roi.width	+= (2 * horizontal_snap_distance	);
		roi.height	+= (2 * vertical_snap_distance		);

		if (roi.x < 0)
		{
			const auto delta = 0 - roi.x;
			roi.x += delta;
			roi.width -= delta;
		}

		if (roi.y < 0)
		{
			const auto delta = 0 - roi.y;
			roi.y += delta;
			roi.width -= delta;
		}

		if (roi.x + roi.width > binary_inverted_image.cols)
		{
			roi.width = binary_inverted_image.cols - roi.x;
		}

		if (roi.y + roi.height > binary_inverted_image.rows)
		{
			roi.height = binary_inverted_image.rows - roi.y;
		}

		cv::Mat nonzero;
		cv::findNonZero(binary_inverted_image(roi), nonzero);
		auto new_rect = cv::boundingRect(nonzero);

		// note that new_rect is relative to the RoI, so we need to move it back into the full image coordinate space
		new_rect.x += roi.x;
		new_rect.y += roi.y;

		if (new_rect == final_rect)
		{
			attempt ++;

			if (attempt >= std::max(config.snapping_horizontal_tolerance, config.snapping_vertical_tolerance))
			{
				// we found a rectangle size that is no longer growing or shrinking
				// consider this mark as having snapped into place
				break;
			}
		}
		else
		{
			// rectangle is still growing
			final_rect = new_rect;
			attempt = 0;

			// Have we grown beyond the limit of what we'd accept?
			// If so, then stop now, no use in trying to grow more.
			if (config.snapping_limit_grow >= 1.0f)
			{
				const float new_area	= new_rect.area();
				const float snap_factor	= new_area / original_area;
				if (snap_factor > config.snapping_limit_grow)
				{
					use_snap = false;
					break;
				}
			}
		}
	}

	if (use_snap and final_rect != original_rect and final_rect.width >= 10 and final_rect.height >= 10)
	{
		const float new_area	= final_rect.area();
		const float snap_factor	= new_area / original_area;

		if (config.snapping_limit_shrink > 0.0f and snap_factor < config.snapping_limit_shrink)
		{
			use_snap = false;
		}

		if (config.snapping_limit_grow >= 1.0f and snap_factor > config.snapping_limit_grow)
		{
			use_snap = false;
		}

		if (use_snap)
		{
			pred.rect = final_rect;

			const double w = final_rect.width;
			const double h = final_rect.height;
			const double x = final_rect.x + w / 2.0;
			const double y = final_rect.y + h / 2.0;

			const double image_w		= binary_inverted_image.cols;
			const double image_h		= binary_inverted_image.rows;
			pred.original_point.x		= x / image_w;
			pred.original_point.y		= y / image_h;
			pred.original_size.width	= w / image_w;
			pred.original_size.height	= h / image_h;
		}
	}

	return *this;
}


cv::Mat DarkHelp::NN::heatmap_combined(const float threshold)
{
	cv::Mat mat;

	if (config.driver == EDriver::kDarknet and darknet_net)
	{
		Darknet::NetworkPtr nw = reinterpret_cast<Darknet::NetworkPtr>(darknet_net);

		auto mm = Darknet::create_yolo_heatmaps(nw, threshold);

		mat = mm[-1];
	}

	return mat;
}


DarkHelp::MMats DarkHelp::NN::heatmaps_all(const float threshold)
{
	MMats mm;

	if (config.driver == EDriver::kDarknet and darknet_net)
	{
		Darknet::NetworkPtr nw = reinterpret_cast<Darknet::NetworkPtr>(darknet_net);

		mm = Darknet::create_yolo_heatmaps(nw, threshold);
	}

	return mm;
}
