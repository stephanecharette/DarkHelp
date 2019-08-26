/* DarkHelp - C++ helper class for Darknet's C API.
 * Copyright 2019 Stephane Charette <stephanecharette@gmail.com>
 * MIT license applies.  See "license.txt" for details.
 * $Id$
 */

#include <DarkHelp.hpp>
#include <fstream>
#include <cmath>
#include <ctime>
#include <darknet.h>


DarkHelp::~DarkHelp()
{
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

	const auto t1 = std::chrono::high_resolution_clock::now();
	net = load_network_custom(const_cast<char*>(cfg_filename.c_str()), const_cast<char*>(weights_filename.c_str()), 1, 1);
	const auto t2 = std::chrono::high_resolution_clock::now();
	duration = t2 - t1;

	if (net == nullptr)
	{
		/// @throw std::runtime_error if the call to darknet's load_network_custom() has failed.
		throw std::runtime_error("darknet failed to load the configuration, the weights, or both");
	}

	network * nw = reinterpret_cast<network*>(net);

	// what do these 2 calls do?
	fuse_conv_batchnorm(*nw);
	calculate_binary_weights(*nw);

	// pick some reasonable default values
	threshold							= 0.5f;
	hierchy_threshold					= 0.5f;
	non_maximal_suppression_threshold	= 0.45f;
	annotation_colour					= cv::Scalar(255, 0, 255);
	annotation_font_face				= cv::HersheyFonts::FONT_HERSHEY_SIMPLEX;
	annotation_font_scale				= 0.5;
	annotation_font_thickness			= 1;
	annotation_include_duration			= true;
	annotation_include_timestamp		= false;
	names_include_percentage			= true;

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

	for (const auto & pred : prediction_results)
	{
		if (pred.best_probability >= threshold)
		{
//			std::cout << "class id=" << pred.best_class << ", probability=" << pred.best_probability << ", point=(" << pred.rect.x << "," << pred.rect.y << "), name=\"" << pred.name << "\", duration=" << duration_string() << std::endl;
			cv::rectangle(annotated_image, pred.rect, annotation_colour, 2);

			const cv::Size text_size = cv::getTextSize(pred.name, annotation_font_face, annotation_font_scale, annotation_font_thickness, nullptr);

			cv::Rect r(cv::Point(pred.rect.x - 1, pred.rect.y - text_size.height - 2), cv::Size(text_size.width + 2, text_size.height + 2));
			cv::rectangle(annotated_image, r, annotation_colour, CV_FILLED);
			cv::putText(annotated_image, pred.name, cv::Point(r.x + 1, r.y + text_size.height), annotation_font_face, annotation_font_scale, cv::Scalar(0,0,0), annotation_font_thickness, CV_AA);
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
	auto darknet_results = get_network_boxes(nw, original_image.cols, original_image.rows, threshold, hierchy_threshold, 0, 1, &nboxes, use_letterbox);

	if (non_maximal_suppression_threshold)
	{
		auto nw_layer = nw->layers[nw->n - 1];
		do_nms_sort(darknet_results, nboxes, nw_layer.classes, non_maximal_suppression_threshold);
	}

	for (int detection_idx = 0; detection_idx < nboxes; detection_idx ++)
	{
		const auto & det = darknet_results[detection_idx];

		if (names.empty())
		{
			// we weren't given a names file to parse, but we know how many classes are defined in the network
			// so we can invest a few dummy names to use based on the class index
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
			// at least 1 class is beyong the threshold, so remember this object

			const int w = std::round(det.bbox.w * original_image.cols);
			const int h = std::round(det.bbox.h * original_image.rows);
			const int x = std::round(det.bbox.x * original_image.cols - w/2.0);
			const int y = std::round(det.bbox.y * original_image.rows - h/2.0);
			pr.rect = cv::Rect(cv::Point(x, y), cv::Size(w, h));

			// now we come up with a decent name to use for this object
			pr.name = names.at(pr.best_class);
			if (names_include_percentage)
			{
				const int percentage = std::round(100.0 * pr.best_probability);
				pr.name += " " + std::to_string(percentage) + "%";
			}
			if (pr.all_probabilities.size() > 1)
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

	free_detections(darknet_results, nboxes);
	free_image(img);

	return prediction_results;
}


cv::Mat resize_keeping_aspect_ratio(cv::Mat mat, const cv::Size & desired_size)
{
	if (mat.empty())
	{
		return mat;
	}

	if (desired_size.width == mat.cols && desired_size.height == mat.rows)
	{
		return mat;
	}

	if (desired_size.width < 1 || desired_size.height < 1)
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
	int interpolation = CV_INTER_AREA;
	if (largest_factor < 1.0)
	{
		// "... to enlarge an image, it will generally look best with CV_INTER_CUBIC"
		interpolation = CV_INTER_CUBIC;
	}

	cv::Mat dst;
	cv::resize(mat, dst, new_size, 0, 0, interpolation);

	return dst;
}
