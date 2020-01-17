/* DarkHelp - C++ helper class for Darknet's C API.
 * Copyright 2019-2020 Stephane Charette <stephanecharette@gmail.com>
 * MIT license applies.  See "license.txt" for details.
 * $Id$
 */

#include <DarkHelp.hpp>
#include <chrono>
#include <map>
#include <vector>
#include <string>
#include <tclap/CmdLine.h>	// "sudo apt-get install libtclap-dev"


// Messages that need to be shown to the user.
// They key is time at which the messages need to be dismissed,
// and the value is the actual text of the message.
typedef std::map<std::chrono::high_resolution_clock::time_point, std::string> MMsg;
MMsg messages;


void show_help_window()
{
	const std::map<std::string, std::string> help =
	{
		// slideshow
		{ "p"			, "Pause or play the slideshow."	},
		{ "DOWN"		, "Slow down the slideshow."		},
		{ "UP"			, "Speed up the slideshow."			},
		// navigation
		{ "LEFT"		, "Go to previous image."			},
		{ "HOME"		, "Go to first image."				},
		{ "END"			, "Go to last image."				},
		// options
		{ "PAGE DOWN"	, "Decrease threshold by 10%."		},
		{ "PAGE UP"		, "Increase threshold by 10%."		},
		{ "g"			, "Toggle greyscale."				},
		// misc
		{ "h"			, "Show help."						},
		{ "w"			, "Write image to disk."			},
		{ "q or ESC"	, "Exit from DarkHelp."				}
	};

	cv::Mat mat(400, 400, CV_8UC3, cv::Scalar(255, 255, 255));

	const auto font_face		= cv::HersheyFonts::FONT_HERSHEY_SIMPLEX;
	const auto font_scale		= 0.5;
	const auto font_thickness	= 1;

	int y = 25;
	for (const auto iter : help)
	{
		const auto & key = iter.first;
		const auto & val = iter.second;

		const cv::Point p1(10, y);
		const cv::Point p2(120, y);

		cv::putText(mat, key, p1, font_face, font_scale, cv::Scalar(0,0,0), font_thickness, CV_AA);
		cv::putText(mat, val, p2, font_face, font_scale, cv::Scalar(0,0,0), font_thickness, CV_AA);
		y += 25;
	}

	cv::imshow("DarkHelp v" DH_VERSION, mat);

	return;
}


void add_msg(const std::string & msg)
{
	// the thought initially was that we'd have multiple messages,
	// but it works better if newer messages completely overwrite older messages
	messages.clear();

	if (msg.empty() == false)
	{
		messages[std::chrono::high_resolution_clock::now() + std::chrono::seconds(2)] = msg;
	}

	return;
}


void clear_old_msg(const std::chrono::high_resolution_clock::time_point & now)
{
	// go through the map and delete messages expired messages

	auto iter = messages.begin();
	while (iter != messages.end())
	{
		if (now > iter->first)
		{
			iter = messages.erase(iter);
		}
		else
		{
			iter ++;
		}
	}

	return;
}


void display_current_msg(DarkHelp & dark_help, const std::chrono::high_resolution_clock::time_point & now, cv::Mat output_image, int & delay_in_milliseconds)
{
	if (not messages.empty())
	{
		const auto timestamp	= messages.begin()->first;
		const std::string & msg	= messages.begin()->second;

		const cv::Size text_size = cv::getTextSize(msg, dark_help.annotation_font_face, dark_help.annotation_font_scale, dark_help.annotation_font_thickness, nullptr);

		cv::Point p(30, 50);
		cv::Rect r(cv::Point(p.x - 5, p.y - text_size.height - 3), cv::Size(text_size.width + 10, text_size.height + 10));
		cv::rectangle(output_image, r, cv::Scalar(0,255,255), CV_FILLED);
		cv::rectangle(output_image, r, cv::Scalar(0,0,0), 1);
		cv::putText(output_image, msg, p, dark_help.annotation_font_face, dark_help.annotation_font_scale, cv::Scalar(0,0,0), dark_help.annotation_font_thickness, CV_AA);

		const int milliseconds_remaining = std::chrono::duration_cast<std::chrono::milliseconds>(timestamp - now).count();
		if (delay_in_milliseconds == 0 || delay_in_milliseconds > milliseconds_remaining)
		{
			delay_in_milliseconds = milliseconds_remaining;
		}
	}

	return;
}


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

		TCLAP::CmdLine cli("Load a darknet neural network and run prediction on the given image file(s).", ' ', DH_VERSION);

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
		TCLAP::SwitchArg greyscale				("g", "greyscale"	, "Force the images to be loaded in greyscale."																, cli, false);
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
		dark_help.hierarchy_threshold				= std::stof(hierarchy.getValue());
		dark_help.non_maximal_suppression_threshold	= std::stof(nms.getValue());
		dark_help.names_include_percentage			= get_bool(percentage);
		dark_help.annotation_font_scale				= std::stod(fontscale.getValue());
		dark_help.annotation_include_duration		= get_bool(duration);
		dark_help.annotation_include_timestamp		= get_bool(timestamp);

		bool force_greyscale = greyscale.getValue();

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

		add_msg("press 'h' for help");

		size_t image_index = 0;
		while (image_index < all_images.size() && not done)
		{
			const std::string & filename = all_images.at(image_index);
			std::cout << "#" << 1+image_index << "/" << all_images.size() << ": loading image \"" << filename << "\"" << std::endl;
			cv::Mat input_image;

			try
			{
				if (force_greyscale)
				{
					cv::Mat tmp = cv::imread(filename, cv::IMREAD_GRAYSCALE);
					// libdarknet.so segfaults when given single-channel images, so convert it back to a 3-channel image
					// forward_network() -> forward_convolutional_layer() -> im2col_cpu_ext()
					cv::cvtColor(tmp, input_image, cv::COLOR_GRAY2BGR);
				}
				else
				{
					input_image = cv::imread(filename);
				}
			}
			catch (...) {}

			if (input_image.empty())
			{
				std::cout << "Failed to read the image \"" << filename << "\"" << std::endl;
				image_index ++;
				continue;
			}

			if (resize1.isSet())
			{
				std::cout << "-> resizing input image from " << input_image.cols << "x" << input_image.rows << " to " << size1.width << "x" << size1.height << std::endl;
				input_image = resize_keeping_aspect_ratio(input_image, size1);
			}

			const auto results = dark_help.predict(input_image);
			std::cout
				<< "-> prediction took " << dark_help.duration_string()	<< std::endl
				<< "-> " << results										<< std::endl;

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

			const auto now = std::chrono::high_resolution_clock::now();
			clear_old_msg(now);
			display_current_msg(dark_help, now, output_image, delay_in_milliseconds);

			if (in_slideshow)
			{
				// slideshow delay overrides whatever delay we may have put due to a message
				delay_in_milliseconds = wait_time_in_milliseconds_for_slideshow;
			}

			cv::namedWindow("DarkHelp", cv::WINDOW_NORMAL | cv::WINDOW_KEEPRATIO | cv::WINDOW_GUI_EXPANDED);
			cv::setWindowTitle("DarkHelp", short_filename);
			cv::imshow("DarkHelp", output_image);

			const int key = cv::waitKeyEx(delay_in_milliseconds);
			std::cout << "KEY=" << key << std::endl;

			if (key == -1 && in_slideshow == false)
			{
				// we didn't really timeout, this is simply an indication that a message needs to be erased
				continue;
			}

			switch (key)
			{
				case 0x00100071:	// 'q'
				case 0x0010001b:	// ESC
				{
					done = true;
					break;
				}
				case 0x00100067:	// 'g'
				{
					force_greyscale = not force_greyscale;
					continue;
				}
				case 0x00100077:	// 'w'
				{
					// save the file to disk, then re-load the same image
					const std::string output_filename = "output.png";
					cv::imwrite(output_filename, output_image, {CV_IMWRITE_PNG_COMPRESSION, 9});
					std::cout << "-> output image saved to \"" << output_filename << "\"" << std::endl;
					add_msg("saved image to \"" + output_filename + "\"");
					continue;
				}
				case 0x00100068:	// 'h'
				{
					show_help_window();
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
					if (wait_time_in_milliseconds_for_slideshow < 50)
					{
						wait_time_in_milliseconds_for_slideshow = 50;
					}
					std::cout << "-> slideshow timeout has been decreased to " << wait_time_in_milliseconds_for_slideshow << " milliseconds" << std::endl;
					add_msg("slideshow timer: " + std::to_string(wait_time_in_milliseconds_for_slideshow) + " milliseconds");
					in_slideshow = true;
					continue;
				}
				case 0x0010ff54:	// DOWN
				{
					// slower slideshow
					wait_time_in_milliseconds_for_slideshow /= 0.5;
					std::cout << "-> slideshow timeout has been increased to " << wait_time_in_milliseconds_for_slideshow << " milliseconds" << std::endl;
					add_msg("slideshow timer: " + std::to_string(wait_time_in_milliseconds_for_slideshow) + " milliseconds");
					in_slideshow = true;
					continue;
				}
				case 0x0010ff55:	// PAGE-UP
				{
					dark_help.threshold += 0.1;
					if (dark_help.threshold > 1.0)
					{
						dark_help.threshold = 1.0;
					}
					add_msg("increased threshold: " + std::to_string((int)std::round(dark_help.threshold * 100.0)) + "%");
					continue;
				}
				case 0x0010ff56:	// PAGE-DOWN
				{
					dark_help.threshold -= 0.1;
					if (dark_help.threshold < 0.01)
					{
						dark_help.threshold = 0.001; // not a typo, allow the lower limit to be 0.1%
					}
					add_msg("decreased threshold: " + std::to_string((int)std::round(dark_help.threshold * 100.0)) + "%");
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
