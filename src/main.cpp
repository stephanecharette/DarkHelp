/* DarkHelp - C++ helper class for Darknet's C API.
 * Copyright 2019 Stephane Charette <stephanecharette@gmail.com>
 * MIT license applies.  See "license.txt" for details.
 * $Id$
 */

#include <DarkHelp.hpp>
#include <vector>
#include <string>
#include <tclap/CmdLine.h>	// "sudo apt-get install libtclap-dev"


bool get_bool(TCLAP::ValueArg<std::string> & arg)
{
	const std::string str = arg.getValue();
	if (str == "true"	||
		str == "on"		||
		str == "t"		||
		str == "1"		)
	{
		return true;
	}

	return false;
}


cv::Size get_WxH(const std::string & text)
{
	int w = 0;
	int h = 0;

	const size_t pos = text.find('x');
	if (pos != std::string::npos)
	{
		w = std::stoi(text.substr(0, pos));
		h = std::stoi(text.substr(pos + 1));
	}

	return cv::Size(w, h);
}


cv::Size get_WxH(TCLAP::ValueArg<std::string> & arg)
{
	return get_WxH(arg.getValue());
}


// Class used to validate "float" parameters.
class FloatConstraint : public TCLAP::Constraint<std::string>
{
	public:

		virtual std::string description() const	{ return "positive float"; }
		virtual std::string shortID() const		{ return "positive float"; }
		virtual bool check(const std::string & value) const
		{
			// can this value be converted to a float?
			try
			{
				size_t idx = 0;
				const float f = std::stof(value, &idx);
				if (f >= 0.0 && idx == value.size()) { return true; } // this is a valid float
			}
			catch (...) {}
			return false;
		}
};


// Class used to validate "WxH" parameters.
class WxHConstraint : public TCLAP::Constraint<std::string>
{
	public:

		virtual std::string description() const	{ return "WxH"; }
		virtual std::string shortID() const		{ return "WxH"; }
		virtual bool check(const std::string & value) const
		{
			// can this value be converted to WxH?
			try
			{
				const cv::Size s = get_WxH(value);
				if (s.width >= 10 && s.height >= 10) { return true; }
			}
			catch (...) {}
			return false;
		}
};


int main(int argc, char *argv[])
{
	try
	{
		std::srand(std::time(nullptr)); // seed random number generator

		TCLAP::CmdLine cli("Load a darknet neural net and run prediction on image files", ' ', DH_VERSION);

		std::vector<std::string> booleans = { "true", "false", "on", "off", "t", "f", "1", "0" };
		auto allowed_booleans = TCLAP::ValuesConstraint<std::string>(booleans);
		auto float_constraint = FloatConstraint();
		auto WxH_constraint = WxHConstraint();

		TCLAP::ValueArg<std::string> hierarchy	("y", "hierarchy"	, "The hierarchy threshold to use when predicting."					, false, "0.5"		, &float_constraint	, cli);
		TCLAP::ValueArg<std::string> threshold	("t", "threshold"	, "The threshold to use when predicting with the neural net."		, false, "0.5"		, &float_constraint	, cli);
		TCLAP::SwitchArg slideshow				("s", "slideshow"	, "Show the images in a slideshow."																			, cli, false );
		TCLAP::SwitchArg random					("r", "random"		, "Randomly shuffle the set of images."																		, cli, false );
		TCLAP::ValueArg<std::string> percentage	("p", "percentage"	, "Determines if percentages are added to annotations."				, false, "true"		, &allowed_booleans	, cli);
		TCLAP::ValueArg<std::string> nms		("n", "nms"			, "The non-maximal suppression threshold to use when predicting."	, false, "0.45"		, &float_constraint	, cli);
		TCLAP::ValueArg<std::string> timestamp	("i", "timestamp"	, "Determines if a timestamp is added to annotations."				, false, "false"	, &allowed_booleans	, cli);
		TCLAP::ValueArg<std::string> fontscale	("f", "fontscale"	, "Determines how the font is scaled for annotations."				, false, "0.5"		, &float_constraint	, cli);
		TCLAP::ValueArg<std::string> duration	("d", "duration"	, "Determines if the duration is added to annotations."				, false, "true"		, &allowed_booleans	, cli);
		TCLAP::ValueArg<std::string> resize1	("b", "resize1"		, "Resize the input image (\"before\") to \"WxH\"."					, false, "640x480"	, &WxH_constraint	, cli);
		TCLAP::ValueArg<std::string> resize2	("a", "resize2"		, "Resize the output image (\"after\") to \"WxH\"."					, false, "640x480"	, &WxH_constraint	, cli);

		TCLAP::UnlabeledValueArg<std::string> cfg		("config"	, "The darknet config filename, usually ends in \".cfg\"."			, true	, "", "cfg"		, cli);
		TCLAP::UnlabeledValueArg<std::string> weights	("weights"	, "The darknet weights filename, usually ends in \".weights\"."		, true	, "", "weights"	, cli);
		TCLAP::UnlabeledValueArg<std::string> names		("names"	, "The darknet class names filename, usually ends in \".names\". "
														"Set to \"none\" if you don't have (or don't care about) the class names."		, true	, "", "names"	, cli);
		TCLAP::UnlabeledMultiArg<std::string> images	("images"	, "The name of images to process with the given neural network."	, true		, "images"	, cli);

		cli.parse(argc, argv);

		if (names.getValue() == "none")
		{
			// special value -- pretend the "names" argument hasn't been set
			names.reset();
		}

		std::cout
			<< "-> config file:  " << cfg		.getValue() << std::endl
			<< "-> weights file: " << weights	.getValue() << std::endl
			<< "-> names file:   " << names		.getValue() << std::endl;

		DarkHelp dark_help(cfg.getValue(), weights.getValue(), names.getValue());
		std::cout << "-> loading network took " << dark_help.duration_string() << std::endl;

		dark_help.threshold							= std::stof(threshold.getValue());
		dark_help.hierchy_threshold					= std::stof(hierarchy.getValue());
		dark_help.non_maximal_suppression_threshold	= std::stof(nms.getValue());
		dark_help.names_include_percentage			= get_bool(percentage);
		dark_help.annotation_font_scale				= std::stod(fontscale.getValue());
		dark_help.annotation_include_duration		= get_bool(duration);
		dark_help.annotation_include_timestamp		= get_bool(timestamp);

		bool in_slideshow = slideshow.getValue();
		int wait_time_in_milliseconds_for_slideshow = 500;

		const cv::Size size1 = get_WxH(resize1);
		const cv::Size size2 = get_WxH(resize2);

		bool done = false;

		DarkHelp::VStr all_images = images.getValue();
		if (random.getValue())
		{
			std::random_shuffle(all_images.begin(), all_images.end());
		}

		size_t image_index = 0;
		while (image_index < all_images.size() && not done)
		{
			const std::string & filename = all_images.at(image_index);
			std::cout << "#" << 1+image_index << "/" << all_images.size() << ": loading image \"" << filename << "\"" << std::endl;
			cv::Mat input_image;

			try
			{
				input_image = cv::imread(filename);
			}
			catch (...) {}

			if (input_image.empty())
			{
				std::cout << "Failed to read the image \"" << filename << "\"" << std::endl;
				continue;
			}

			if (resize1.isSet())
			{
				std::cout << "-> resizing input image from " << input_image.cols << "x" << input_image.rows << " to " << size1.width << "x" << size1.height << std::endl;
				input_image = resize_keeping_aspect_ratio(input_image, size1);
			}

			const auto results = dark_help.predict(input_image);
			std::cout
				<< "-> prediction took " << dark_help.duration_string() << std::endl
				<< "-> prediction results:  " << results.size() << std::endl;
			for (size_t idx = 0; idx < results.size(); idx ++)
			{
				const auto & pred = results.at(idx);
				std::cout << "-> " << 1+idx << "/" << results.size() << ": " << pred.name << ", x=" << pred.rect.x << ", y=" << pred.rect.y << ", w=" << pred.rect.width << ", h=" << pred.rect.height << std::endl;
			}

			cv::Mat output_image = dark_help.annotate();

			if (resize2.isSet())
			{
				std::cout << "-> resizing output image from " << output_image.cols << "x" << output_image.rows << " to " << size2.width << "x" << size2.height << std::endl;
				output_image = resize_keeping_aspect_ratio(output_image, size2);
			}

			std::string short_filename = filename;
			size_t pos = short_filename.find_last_of("/\\");
			if (pos != std::string::npos)
			{
				short_filename.erase(0, pos + 1);
			}

			int delay_in_milliseconds = 0; // wait forever
			if (in_slideshow)
			{
				delay_in_milliseconds = wait_time_in_milliseconds_for_slideshow;
			}

			cv::namedWindow("prediction", cv::WINDOW_NORMAL | cv::WINDOW_KEEPRATIO | cv::WINDOW_GUI_EXPANDED);
			cv::setWindowTitle("prediction", short_filename);
			cv::imshow("prediction", output_image);

			const int key = cv::waitKeyEx(delay_in_milliseconds);
//			std::cout << "KEY=" << key << std::endl;

			switch (key)
			{
				case 0x00100071:	// 'q'
				case 0x0010001b:	// ESC
				{
					done = true;
					break;
				}
				case 0x00100077:	// 'w'
				{
					// save the file to disk, then re-load the same image
					cv::imwrite("output.png", output_image, {CV_IMWRITE_PNG_COMPRESSION, 9});
					std::cout << "-> output image saved to \"output.png\"" << std::endl;
					continue;
				}
				case 0x0010ff50:	// HOME
				{
					in_slideshow = false;
					image_index = 0;
					break;
				}
				case 0x0010ff57:	// END
				{
					in_slideshow = false;
					image_index = all_images.size() - 1;
					break;
				}
				case 0x0010ff51:	// LEFT
				{
					in_slideshow = false;
					if (image_index > 0)
					{
						image_index --;
					}
					break;
				}
				case 0x0010ff52:	// UP
				{
					// quicker slideshow
					wait_time_in_milliseconds_for_slideshow *= 0.5;
					if (wait_time_in_milliseconds_for_slideshow < 10)
					{
						wait_time_in_milliseconds_for_slideshow = 10;
					}
					std::cout << "-> slideshow timeout has been decreased to " << wait_time_in_milliseconds_for_slideshow << " milliseconds" << std::endl;
					in_slideshow = true;
					continue;
				}
				case 0x0010ff54:	// DOWN
				{
					// slower slideshow
					wait_time_in_milliseconds_for_slideshow /= 0.5;
					std::cout << "-> slideshow timeout has been increased to " << wait_time_in_milliseconds_for_slideshow << " milliseconds" << std::endl;
					in_slideshow = true;
					continue;
				}
				case 0x00100070:	// 'p'
				{
					in_slideshow = not in_slideshow;
					// don't increment the image index, stay on this image
					break;
				}
				default:
				{
					image_index ++;
					break;
				}
			}
		}

		if (image_index >= all_images.size())
		{
			std::cout << "Done showing " << all_images.size() << " images...exiting." << std::endl;
		}
		else
		{
			std::cout << "Exiting!" << std::endl;
		}
	}
	catch (const TCLAP::ArgException & e)
	{
		std::cout << "Caught exception processing args: " << e.error() << " for argument " << e.argId() << std::endl;
	}
	catch (const std::exception & e)
	{
		std::cout << "Caught exception: " << e.what() << std::endl;
	}
	catch (...)
	{
		std::cout << "Caught unknown exception." << std::endl;
	}

	return 0;
}
