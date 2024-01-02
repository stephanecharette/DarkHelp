/* DarkHelp - C++ helper class for Darknet's C API.
 * Copyright 2019-2023 Stephane Charette <stephanecharette@gmail.com>
 * MIT license applies.  See "license.txt" for details.
 */

#include "DarkHelp.hpp"
#include "json.hpp"

// If you get an error with this next include file, it probably means you are using an old version of Darknet which is
// no longer supported.  You should be using this version instead:  https://github.com/hank-ai/darknet#table-of-contents
#include <darknet_version.h>


/** @file
 * The @p C API is a wrapper around some of the most common @p C++ %DarkHelp objects and methods.  If you have the
 * ability to use @p C++, you should instead be using the objects in @p DarkHelpNN.hpp and @p DarkHelpConfig.hpp.
 */


const char * DarkHelpVersion()
{
	const static auto version = DarkHelp::version();
	return version.c_str();
}


const char * DarknetVersion()
{
	return DARKNET_VERSION_SHORT;
}


void ToggleOutputRedirection()
{
	try
	{
		DarkHelp::toggle_output_redirection();
	}
	catch (const std::exception & e)
	{
		std::cerr << "ERROR: " << e.what() << std::endl;
	}

	return;
}


DarkHelpPtr CreateDarkHelpNN(const char * const fn1, const char * const fn2, const char * const fn3)
{
	DarkHelpPtr ptr = nullptr;
	try
	{
		ptr = reinterpret_cast<DarkHelpPtr>(new DarkHelp::NN(fn1, fn2, fn3));
	}
	catch (const std::exception & e)
	{
		std::cerr << "ERROR: " << e.what() << std::endl;
	}

	return ptr;
}


void DestroyDarkHelpNN(DarkHelpPtr ptr)
{
	if (ptr == nullptr)
	{
		std::cerr << "ignoring call to " << __func__ << " with a null pointer" << std::endl;
		return;
	}

	try
	{
		DarkHelp::NN * nn = reinterpret_cast<DarkHelp::NN*>(ptr);

		delete nn;
	}
	catch (const std::exception & e)
	{
		std::cerr << e.what() << std::endl;
	}

	return;
}


int PredictFN(DarkHelpPtr ptr, const char * const image_filename)
{
	if (ptr == nullptr)
	{
		std::cerr << "ignoring call to " << __func__ << " with a null pointer" << std::endl;
		return -1;
	}

	int size = 0;
	try
	{
		DarkHelp::NN * nn = reinterpret_cast<DarkHelp::NN*>(ptr);

		nn->predict(image_filename);
		size = (int)nn->prediction_results.size();
	}
	catch (const std::exception & e)
	{
		std::cerr << e.what() << std::endl;
	}

	/// @todo what should this return?
	return size;
}


int Predict(DarkHelpPtr ptr, const int width, const int height, uint8_t * image, const int number_of_bytes)
{
	if (ptr == nullptr)
	{
		std::cerr << "ignoring call to " << __func__ << " with a null pointer" << std::endl;
		return -1;
	}

	if (image == nullptr)
	{
		std::cerr << "ignoring call to " << __func__ << " with a null image data pointer" << std::endl;
		return -1;
	}

	if (width * height <= 0)
	{
		std::cerr << "ignoring call to " << __func__ << " with invalid image width and height" << std::endl;
		return -1;
	}

	if (number_of_bytes <= 0)
	{
		std::cerr << "ignoring call to " << __func__ << " with invalid image data bytes size" << std::endl;
		return -1;
	}

	const int channels = number_of_bytes / width / height;
	if (number_of_bytes != width * height * channels)
	{
		std::cerr << "ignoring call to " << __func__ << " with invalid image data size (width, height, and channels don't match the image data size)" << std::endl;
		return -1;
	}

	int number_of_predictions = 0;
	try
	{
		DarkHelp::NN * nn = reinterpret_cast<DarkHelp::NN*>(ptr);

		const auto type = CV_8UC(channels);
		cv::Mat mat(height, width, type, static_cast<void*>(image));

		nn->predict(mat);

		number_of_predictions = (int)nn->prediction_results.size();
	}
	catch (const std::exception & e)
	{
		std::cerr << e.what() << std::endl;
	}

	return number_of_predictions;
}


const char * GetPredictionResults(DarkHelpPtr ptr)
{
	static std::string buffer;
	buffer = "\0";

	if (ptr == nullptr)
	{
		std::cerr << "ignoring call to " << __func__ << " with a null pointer" << std::endl;
		return buffer.c_str();
	}

	try
	{
		DarkHelp::NN * nn = reinterpret_cast<DarkHelp::NN*>(ptr);

		nlohmann::json json;
		json["file"][0]["count"]				= nn->prediction_results.size();
		json["file"][0]["duration"]				= nn->duration_string();
		json["file"][0]["filename"]				= "unknown";
		json["file"][0]["original_height"]		= nn->original_image.rows;
		json["file"][0]["original_width"]		= nn->original_image.cols;
		json["file"][0]["tiles"]["horizontal"]	= nn->horizontal_tiles;
		json["file"][0]["tiles"]["vertical"]	= nn->vertical_tiles;
		json["file"][0]["tiles"]["width"]		= nn->tile_size.width;
		json["file"][0]["tiles"]["height"]		= nn->tile_size.height;
		json["file"][0]["resized_width"]		= nn->network_size().width;
		json["file"][0]["resized_height"]		= nn->network_size().height;
		json["file"][0]["prediction"]			= nlohmann::json::array();

		json["network"]["cfg"]					= nn->config.cfg_filename;
		json["network"]["names"]				= nn->config.names_filename;
		json["network"]["weights"]				= nn->config.weights_filename;

		json["settings"]["driver"]				= nn->config.driver;
		json["settings"]["threshold"]			= nn->config.threshold;
		json["settings"]["nms"]					= nn->config.non_maximal_suppression_threshold;
		json["settings"]["include_percentage"]	= nn->config.names_include_percentage;
		json["settings"]["enable_tiles"]		= nn->config.enable_tiles;
		json["settings"]["snapping"]			= nn->config.snapping_enabled;
		json["settings"]["output_redirection"]	= nn->config.redirect_darknet_output;

		const auto & results = nn->prediction_results;
		for (size_t idx = 0; idx < results.size(); idx ++)
		{
			const auto & pred = results[idx];

			auto & j = json["file"][0]["prediction"][idx];

			j["prediction_index"]			= idx;
			j["name"]						= pred.name;
			j["best_class"]					= pred.best_class;
			j["best_probability"]			= pred.best_probability;
			j["original_size"]["width"]		= pred.original_size.width;
			j["original_size"]["height"]	= pred.original_size.height;
			j["original_point"]["x"]		= pred.original_point.x;
			j["original_point"]["y"]		= pred.original_point.y;
			j["rect"]["x"]					= pred.rect.x;
			j["rect"]["y"]					= pred.rect.y;
			j["rect"]["width"]				= pred.rect.width;
			j["rect"]["height"]				= pred.rect.height;

			size_t prop_count = 0;
			for (const auto & prop : pred.all_probabilities)
			{
				j["all_probabilities"][prop_count]["class"			] = prop.first;
				j["all_probabilities"][prop_count]["probability"	] = prop.second;
				j["all_probabilities"][prop_count]["name"			] = nn->names[prop.first];
				prop_count ++;
			}
		}

		const std::time_t tt = std::time(nullptr);
		auto lt = std::localtime(&tt);
		char time_buffer[50];
		std::strftime(time_buffer, sizeof(time_buffer), "%Y-%m-%d %H:%M:%S %z", lt);
		json["timestamp"]["epoch"] = tt;
		json["timestamp"]["text"] = time_buffer;

		buffer = json.dump(4);
	}
	catch (const std::exception & e)
	{
		std::cerr << e.what() << std::endl;
	}

	return buffer.c_str();
}


void Annotate(DarkHelpPtr ptr, const char * const output_image_filename)
{
	if (ptr == nullptr)
	{
		std::cerr << "ignoring call to " << __func__ << " with a null pointer" << std::endl;
		return;
	}

	if (output_image_filename == nullptr)
	{
		std::cerr << "ignoring call to " << __func__ << " with a null output image filename" << std::endl;
		return;
	}

	try
	{
		DarkHelp::NN * nn = reinterpret_cast<DarkHelp::NN*>(ptr);
		cv::Mat mat = nn->annotate();

		std::string fn = output_image_filename;
		if (fn.find(".jpg"	) != std::string::npos or
			fn.find(".JPG"	) != std::string::npos or
			fn.find(".jpeg"	) != std::string::npos or
			fn.find(".JPEG"	) != std::string::npos)
		{
			cv::imwrite(fn, mat, {cv::IMWRITE_JPEG_QUALITY, 75});
		}
		else
		{
			// ...otherwise, assume the file is PNG

			if (fn.find(".png") == std::string::npos and
				fn.find(".PNG") == std::string::npos)
			{
				fn += ".png";
			}
			cv::imwrite(fn, mat, {cv::IMWRITE_PNG_COMPRESSION, 3});
		}
	}
	catch (const std::exception & e)
	{
		std::cerr << e.what() << std::endl;
	}

	return;
}


float SetThreshold(DarkHelpPtr ptr, float threshold)
{
	if (ptr == nullptr)
	{
		std::cerr << "ignoring call to " << __func__ << " with a null pointer" << std::endl;
		return -1.0f;
	}

	DarkHelp::NN * nn = reinterpret_cast<DarkHelp::NN*>(ptr);
	std::swap(threshold, nn->config.threshold);

	return threshold;
}


float SetNonMaximalSuppression(DarkHelpPtr ptr, float nms)
{
	if (ptr == nullptr)
	{
		std::cerr << "ignoring call to " << __func__ << " with a null pointer" << std::endl;
		return -1.0f;
	}

	DarkHelp::NN * nn = reinterpret_cast<DarkHelp::NN*>(ptr);
	std::swap(nms, nn->config.non_maximal_suppression_threshold);

	return nms;
}


bool EnableNamesIncludePercentage(DarkHelpPtr ptr, bool enabled)
{
	if (ptr == nullptr)
	{
		std::cerr << "ignoring call to " << __func__ << " with a null pointer" << std::endl;
		return false;
	}

	DarkHelp::NN * nn = reinterpret_cast<DarkHelp::NN*>(ptr);
	std::swap(enabled, nn->config.names_include_percentage);

	return enabled;
}


bool EnableAnnotationAutoHideLabels(DarkHelpPtr ptr, bool enabled)
{
	if (ptr == nullptr)
	{
		std::cerr << "ignoring call to " << __func__ << " with a null pointer" << std::endl;
		return false;
	}

	DarkHelp::NN * nn = reinterpret_cast<DarkHelp::NN*>(ptr);
	std::swap(enabled, nn->config.annotation_auto_hide_labels);

	return enabled;
}


bool EnableAnnotationSuppressAllLabels(DarkHelpPtr ptr, bool enabled)
{
	if (ptr == nullptr)
	{
		std::cerr << "ignoring call to " << __func__ << " with a null pointer" << std::endl;
		return false;
	}

	DarkHelp::NN * nn = reinterpret_cast<DarkHelp::NN*>(ptr);
	std::swap(enabled, nn->config.annotation_suppress_all_labels);

	return enabled;
}


float SetAnnotationShadePredictions(DarkHelpPtr ptr, float shading)
{
	if (ptr == nullptr)
	{
		std::cerr << "ignoring call to " << __func__ << " with a null pointer" << std::endl;
		return -1.0f;
	}

	DarkHelp::NN * nn = reinterpret_cast<DarkHelp::NN*>(ptr);
	std::swap(shading, nn->config.annotation_shade_predictions);

	return shading;
}


bool EnableIncludeAllNames(DarkHelpPtr ptr, bool enabled)
{
	if (ptr == nullptr)
	{
		std::cerr << "ignoring call to " << __func__ << " with a null pointer" << std::endl;
		return false;
	}

	DarkHelp::NN * nn = reinterpret_cast<DarkHelp::NN*>(ptr);
	std::swap(enabled, nn->config.include_all_names);

	return enabled;
}


double SetAnnotationFontScale(DarkHelpPtr ptr, double scale)
{
	if (ptr == nullptr)
	{
		std::cerr << "ignoring call to " << __func__ << " with a null pointer" << std::endl;
		return -1.0;
	}

	DarkHelp::NN * nn = reinterpret_cast<DarkHelp::NN*>(ptr);
	std::swap(scale, nn->config.annotation_font_scale);

	return scale;
}


int SetAnnotationFontThickness(DarkHelpPtr ptr, int thickness)
{
	if (ptr == nullptr)
	{
		std::cerr << "ignoring call to " << __func__ << " with a null pointer" << std::endl;
		return -1;
	}

	DarkHelp::NN * nn = reinterpret_cast<DarkHelp::NN*>(ptr);
	std::swap(thickness, nn->config.annotation_font_thickness);

	return thickness;
}


int SetAnnotationLineThickness(DarkHelpPtr ptr, int thickness)
{
	if (ptr == nullptr)
	{
		std::cerr << "ignoring call to " << __func__ << " with a null pointer" << std::endl;
		return -1;
	}

	DarkHelp::NN * nn = reinterpret_cast<DarkHelp::NN*>(ptr);
	std::swap(thickness, nn->config.annotation_line_thickness);

	return thickness;
}


bool EnableAnnotationIncludeDuration(DarkHelpPtr ptr, bool enabled)
{
	if (ptr == nullptr)
	{
		std::cerr << "ignoring call to " << __func__ << " with a null pointer" << std::endl;
		return false;
	}

	DarkHelp::NN * nn = reinterpret_cast<DarkHelp::NN*>(ptr);
	std::swap(enabled, nn->config.annotation_include_duration);

	return enabled;
}


bool EnableAnnotationIncludeTimestamp(DarkHelpPtr ptr, bool enabled)
{
	if (ptr == nullptr)
	{
		std::cerr << "ignoring call to " << __func__ << " with a null pointer" << std::endl;
		return false;
	}

	DarkHelp::NN * nn = reinterpret_cast<DarkHelp::NN*>(ptr);
	std::swap(enabled, nn->config.annotation_include_timestamp);

	return enabled;
}


bool EnableAnnotationPixelate(DarkHelpPtr ptr, bool enabled)
{
	if (ptr == nullptr)
	{
		std::cerr << "ignoring call to " << __func__ << " with a null pointer" << std::endl;
		return false;
	}

	DarkHelp::NN * nn = reinterpret_cast<DarkHelp::NN*>(ptr);
	std::swap(enabled, nn->config.annotation_pixelate_enabled);

	return enabled;
}


int SetAnnotationPixelateSize(DarkHelpPtr ptr, int size)
{
	if (ptr == nullptr)
	{
		std::cerr << "ignoring call to " << __func__ << " with a null pointer" << std::endl;
		return -1;
	}

	DarkHelp::NN * nn = reinterpret_cast<DarkHelp::NN*>(ptr);
	std::swap(size, nn->config.annotation_pixelate_size);

	return size;
}


bool EnableTiles(DarkHelpPtr ptr, bool enabled)
{
	if (ptr == nullptr)
	{
		std::cerr << "ignoring call to " << __func__ << " with a null pointer" << std::endl;
		return false;
	}

	DarkHelp::NN * nn = reinterpret_cast<DarkHelp::NN*>(ptr);
	std::swap(enabled, nn->config.enable_tiles);

	return enabled;
}


bool EnableCombineTilePredictions(DarkHelpPtr ptr, bool enabled)
{
	if (ptr == nullptr)
	{
		std::cerr << "ignoring call to " << __func__ << " with a null pointer" << std::endl;
		return false;
	}

	DarkHelp::NN * nn = reinterpret_cast<DarkHelp::NN*>(ptr);
	std::swap(enabled, nn->config.combine_tile_predictions);

	return enabled;
}


bool EnableOnlyCombineSimilarPredictions(DarkHelpPtr ptr, bool enabled)
{
	if (ptr == nullptr)
	{
		std::cerr << "ignoring call to " << __func__ << " with a null pointer" << std::endl;
		return false;
	}

	DarkHelp::NN * nn = reinterpret_cast<DarkHelp::NN*>(ptr);
	std::swap(enabled, nn->config.only_combine_similar_predictions);

	return enabled;
}


float SetTileEdgeFactor(DarkHelpPtr ptr, float factor)
{
	if (ptr == nullptr)
	{
		std::cerr << "ignoring call to " << __func__ << " with a null pointer" << std::endl;
		return -1.0f;
	}

	DarkHelp::NN * nn = reinterpret_cast<DarkHelp::NN*>(ptr);
	std::swap(factor, nn->config.tile_edge_factor);

	return factor;
}


float SetTileRectFactor(DarkHelpPtr ptr, float factor)
{
	if (ptr == nullptr)
	{
		std::cerr << "ignoring call to " << __func__ << " with a null pointer" << std::endl;
		return -1.0f;
	}

	DarkHelp::NN * nn = reinterpret_cast<DarkHelp::NN*>(ptr);
	std::swap(factor, nn->config.tile_rect_factor);

	return factor;
}


bool EnableSnapping(DarkHelpPtr ptr, bool enabled)
{
	if (ptr == nullptr)
	{
		std::cerr << "ignoring call to " << __func__ << " with a null pointer" << std::endl;
		return false;
	}

	DarkHelp::NN * nn = reinterpret_cast<DarkHelp::NN*>(ptr);
	std::swap(enabled, nn->config.snapping_enabled);

	return enabled;
}


int SetBinaryThresholdBlockSize(DarkHelpPtr ptr, int blocksize)
{
	if (ptr == nullptr)
	{
		std::cerr << "ignoring call to " << __func__ << " with a null pointer" << std::endl;
		return -1;
	}

	DarkHelp::NN * nn = reinterpret_cast<DarkHelp::NN*>(ptr);
	std::swap(blocksize, nn->config.binary_threshold_block_size);

	return blocksize;
}


double SetBinaryThresholdConstant(DarkHelpPtr ptr, double threshold)
{
	if (ptr == nullptr)
	{
		std::cerr << "ignoring call to " << __func__ << " with a null pointer" << std::endl;
		return -1.0;
	}

	DarkHelp::NN * nn = reinterpret_cast<DarkHelp::NN*>(ptr);
	std::swap(threshold, nn->config.binary_threshold_constant);

	return threshold;
}


int SetSnappingHorizontalTolerance(DarkHelpPtr ptr, int tolerance)
{
	if (ptr == nullptr)
	{
		std::cerr << "ignoring call to " << __func__ << " with a null pointer" << std::endl;
		return -1;
	}

	DarkHelp::NN * nn = reinterpret_cast<DarkHelp::NN*>(ptr);
	std::swap(tolerance, nn->config.snapping_horizontal_tolerance);

	return tolerance;
}


int SetSnappingVerticalTolerance(DarkHelpPtr ptr, int tolerance)
{
	if (ptr == nullptr)
	{
		std::cerr << "ignoring call to " << __func__ << " with a null pointer" << std::endl;
		return -1;
	}

	DarkHelp::NN * nn = reinterpret_cast<DarkHelp::NN*>(ptr);
	std::swap(tolerance, nn->config.snapping_vertical_tolerance);

	return tolerance;
}


float SetSnappingLimitShrink(DarkHelpPtr ptr, float limit)
{
	if (ptr == nullptr)
	{
		std::cerr << "ignoring call to " << __func__ << " with a null pointer" << std::endl;
		return -1.0f;
	}

	DarkHelp::NN * nn = reinterpret_cast<DarkHelp::NN*>(ptr);
	std::swap(limit, nn->config.snapping_limit_shrink);

	return limit;
}


float SetSnappingLimitGrow(DarkHelpPtr ptr, float limit)
{
	if (ptr == nullptr)
	{
		std::cerr << "ignoring call to " << __func__ << " with a null pointer" << std::endl;
		return -1.0f;
	}

	DarkHelp::NN * nn = reinterpret_cast<DarkHelp::NN*>(ptr);
	std::swap(limit, nn->config.snapping_limit_grow);

	return limit;
}


bool EnableUseFastImageResize(DarkHelpPtr ptr, bool enabled)
{
	if (ptr == nullptr)
	{
		std::cerr << "ignoring call to " << __func__ << " with a null pointer" << std::endl;
		return false;
	}

	DarkHelp::NN * nn = reinterpret_cast<DarkHelp::NN*>(ptr);
	std::swap(enabled, nn->config.use_fast_image_resize);

	return enabled;
}
