/* DarkHelp - C++ helper class for Darknet's C API.
 * Copyright 2019-2020 Stephane Charette <stephanecharette@gmail.com>
 * MIT license applies.  See "license.txt" for details.
 * $Id$
 */

#include <DarkHelp.hpp>
#include <fstream>
#include <cmath>
#include <ctime>


/** @warning Prior to including @p darknet.h, you @b must @p "#define GPU" and @p "#define CUDNN" @b if darknet was built
 * with support for GPU and CUDNN!  This is because the darknet structures have several optional fields that only exist
 * when @p GPU and @p CUDNN are defined, thereby changing the size of those structures.  If DarkHelp and Darknet aren't
 * using the exact same structure size, you'll see segfaults when DarkHelp calls into Darknet.
 */
#include <darknet.h>


DarkHelp::~DarkHelp()
{
	if (net)
	{
		network * nw = reinterpret_cast<network*>(net);
		free_network(*nw);
		free(net); // this was calloc()'d in load_network_custom()
		net = nullptr;
	}

	return;
}


DarkHelp::DarkHelp(const std::string & cfg_filename, const std::string & weights_filename, const std::string & names_filename)
{
	if (cfg_filename.empty())
	{
		/// @throw std::invalid_argument if the configuration filename is empty.
		throw std::invalid_argument("darknet configuration filename cannot be empty");
	}
	if (weights_filename.empty())
	{
		/// @throw std::invalid_argument if the weights filename is empty.
		throw std::invalid_argument("darknet weights filename cannot be empty");
	}

	// The calls we make into darknet are based on what was found in test_detector() from src/detector.c.

	const auto t1 = std::chrono::high_resolution_clock::now();
	net = load_network_custom(const_cast<char*>(cfg_filename.c_str()), const_cast<char*>(weights_filename.c_str()), 1, 1);
	if (net == nullptr)
	{
		/// @throw std::runtime_error if the call to darknet's @p load_network_custom() has failed.
		throw std::runtime_error("darknet failed to load the configuration, the weights, or both");
	}

	network * nw = reinterpret_cast<network*>(net);

	// what do these 2 calls do?
	fuse_conv_batchnorm(*nw);
	calculate_binary_weights(*nw);

	const auto t2 = std::chrono::high_resolution_clock::now();
	duration = t2 - t1;

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

	if (not names_filename.empty())
	{
		std::ifstream ifs(names_filename);
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

	original_image = mat;

	return predict(new_threshold);
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
		if (annotation_line_thickness > 0 and pred.best_probability >= threshold)
		{
			const auto colour = annotation_colours[pred.best_class % annotation_colours.size()];

//			std::cout << "class id=" << pred.best_class << ", probability=" << pred.best_probability << ", point=(" << pred.rect.x << "," << pred.rect.y << "), name=\"" << pred.name << "\", duration=" << duration_string() << std::endl;
			cv::rectangle(annotated_image, pred.rect, colour, annotation_line_thickness);

			int baseline = 0;
			const cv::Size text_size = cv::getTextSize(pred.name, annotation_font_face, annotation_font_scale, annotation_font_thickness, &baseline);

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
		std::cout << "timestamp=" << timestamp << std::endl;

		const cv::Size text_size	= cv::getTextSize(timestamp, annotation_font_face, annotation_font_scale, annotation_font_thickness, nullptr);

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


DarkHelp::VColours DarkHelp::get_default_annotation_colours()
{
	VColours colours =
	{
		// remember the OpenCV format blue-green-red and not RGB!
		{0x5E, 0x35, 0xFF},	// Radical Red
		{0x17, 0x96, 0x29},	// Slimy Green
		{0x33, 0xCC, 0xFF},	// Sunglow
		{0x4D, 0x6E, 0xAF},	// Brown Sugar
		{0xFF, 0x00, 0xFF},	// pure magenta
		{0xE6, 0xBF, 0x50},	// Blizzard Blue
		{0x00, 0xFF, 0xCC},	// Electric Lime
		{0xFF, 0xFF, 0x00},	// pure cyan
		{0x85, 0x4E, 0x8D},	// Razzmic Berry
		{0xCC, 0x00, 0xFF},	// Purple Pizzazz
		{0x00, 0xFF, 0x00},	// pure green
		{0x00, 0xFF, 0xFF},	// pure yellow
		{0xEC, 0xAD, 0x5D},	// Blue Jeans
		{0xFF, 0x6E, 0xFF},	// Shocking Pink
		{0x66, 0xFF, 0xFF},	// Laser Lemon
		{0xD1, 0xF0, 0xAA},	// Magic Mint
		{0x00, 0xC0, 0xFF},	// orange
		{0xB6, 0x51, 0x9C},	// Purple Plum
		{0x33, 0x99, 0xFF},	// Neon Carrot
		{0xFF, 0x00, 0xFF},	// pure purple
		{0x66, 0xFF, 0x66},	// Screamin' Green
		{0x00, 0x00, 0xFF},	// pure red
		{0x37, 0x60, 0xFF},	// Outrageous Orange
		{0x78, 0x5B, 0xFD},	// Wild Watermelon
		{0xFF, 0x00, 0x00}	// pure blue
	};

	return colours;
}


DarkHelp::PredictionResults DarkHelp::predict(const float new_threshold)
{
	prediction_results.clear();
	annotated_image = cv::Mat();

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
		pr.best_class = 0;
		pr.best_probability = 0.0f;
		
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
			pr.name = names.at(pr.best_class);
			if (names_include_percentage)
			{
				const int percentage = std::round(100.0 * pr.best_probability);
				pr.name += " " + std::to_string(percentage) + "%";
			}
			if (include_all_names and pr.all_probabilities.size() > 1)
			{
				// we have multiple probabilities!
				for (auto iter : pr.all_probabilities)
				{
					const int & key = iter.first;
					if (key != pr.best_class)
					{
						pr.name += ", " + names.at(key);
						if (names_include_percentage)
						{
							const int percentage = std::round(100.0 * iter.second);
							pr.name += " " + std::to_string(percentage) + "%";
						}
					}
				}
			}

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


std::ostream & operator<<(std::ostream & os, const DarkHelp::PredictionResult & pred)
{
	os	<< "\""			<< pred.name << "\""
		<< " #"			<< pred.best_class
		<< " prob="		<< pred.best_probability
		<< " x="		<< pred.rect.x
		<< " y="		<< pred.rect.y
		<< " w="		<< pred.rect.width
		<< " h="		<< pred.rect.height
		<< " entries="	<< pred.all_probabilities.size();

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
