/* DarkHelp - C++ helper class for Darknet's C API.
 * Copyright 2019-2020 Stephane Charette <stephanecharette@gmail.com>
 * MIT license applies.  See "license.txt" for details.
 * $Id$
 */

#include <DarkHelp.hpp>
#include <chrono>
#include <fstream>
#include <map>
#include <vector>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <magic.h>
#include <tclap/CmdLine.h>	// "sudo apt-get install libtclap-dev"
#include "json.hpp"


/* OpenCV4 has renamed some common defines and placed them in the cv namespace.  Need to deal with this until older
 * versions of OpenCV are no longer in use.
 */
#ifndef CV_AA
#define CV_AA cv::LINE_AA
#endif
#ifndef CV_FILLED
#define CV_FILLED cv::FILLED
#endif
#ifndef CV_IMWRITE_PNG_COMPRESSION
#define CV_IMWRITE_PNG_COMPRESSION cv::IMWRITE_PNG_COMPRESSION
#endif


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


// Class used to validate that an input file exists
class FileExistConstraint : public TCLAP::Constraint<std::string>
{
	public:

		virtual std::string description() const	{ return "file must exist"; }
		virtual std::string shortID() const		{ return "filename"; }
		virtual bool check(const std::string & value) const
		{
			struct stat s;
			const int result = stat(value.c_str(), &s);
			return (result == 0);
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


struct Options
{
	std::string cfg_fn;
	std::string weights_fn;
	std::string names_fn;
	bool keep_annotated_images;
	bool use_json_output;
	nlohmann::json json;
	DarkHelp dark_help;
	bool force_greyscale;
	bool done;
	bool size1_is_set;
	bool size2_is_set;
	cv::Size size1;
	cv::Size size2;
	DarkHelp::VStr all_files;
	bool in_slideshow;
	int wait_time_in_milliseconds_for_slideshow;
	std::string filename;
	size_t file_index;
};


void init(Options & options, int argc, char *argv[])
{
	std::srand(std::time(nullptr)); // seed random number generator

	TCLAP::CmdLine cli("Load a darknet neural network and run prediction on the given image file(s).", ' ', DH_VERSION);

	std::vector<std::string> booleans = { "true", "false", "on", "off", "t", "f", "1", "0" };
	auto allowed_booleans = TCLAP::ValuesConstraint<std::string>(booleans);
	auto float_constraint = FloatConstraint();
	auto WxH_constraint = WxHConstraint();
	auto exist_constraint = FileExistConstraint();

	TCLAP::ValueArg<std::string> hierarchy	("y", "hierarchy"	, "The hierarchy threshold to use when predicting."					, false, "0.5"		, &float_constraint	, cli);
	TCLAP::ValueArg<std::string> threshold	("t", "threshold"	, "The threshold to use when predicting with the neural net."		, false, "0.5"		, &float_constraint	, cli);
	TCLAP::SwitchArg slideshow				("s", "slideshow"	, "Show the images in a slideshow."																			, cli, false );
	TCLAP::SwitchArg random					("r", "random"		, "Randomly shuffle the set of images."																		, cli, false );
	TCLAP::ValueArg<std::string> percentage	("p", "percentage"	, "Determines if percentages are added to annotations."				, false, "true"		, &allowed_booleans	, cli);
	TCLAP::ValueArg<std::string> nms		("n", "nms"			, "The non-maximal suppression threshold to use when predicting."	, false, "0.45"		, &float_constraint	, cli);
	TCLAP::ValueArg<std::string> timestamp	("i", "timestamp"	, "Determines if a timestamp is added to annotations."				, false, "false"	, &allowed_booleans	, cli);
	TCLAP::SwitchArg use_json				("j", "json"		, "Enable JSON output (useful when DarkHelp is used in a shell script)."									, cli, false );
	TCLAP::SwitchArg keep_images			("k", "keep"		, "Keep annotated images (write images to disk). Especially useful when combined with the -j option."		, cli, false );
	TCLAP::SwitchArg greyscale				("g", "greyscale"	, "Force the images to be loaded in greyscale."																, cli, false );
	TCLAP::ValueArg<std::string> fontscale	("f", "fontscale"	, "Determines how the font is scaled for annotations."				, false, "0.5"		, &float_constraint	, cli);
	TCLAP::ValueArg<std::string> duration	("d", "duration"	, "Determines if the duration is added to annotations."				, false, "true"		, &allowed_booleans	, cli);
	TCLAP::ValueArg<std::string> resize1	("b", "resize1"		, "Resize the input image (\"before\") to \"WxH\"."					, false, "640x480"	, &WxH_constraint	, cli);
	TCLAP::ValueArg<std::string> resize2	("a", "resize2"		, "Resize the output image (\"after\") to \"WxH\"."					, false, "640x480"	, &WxH_constraint	, cli);
	TCLAP::ValueArg<std::string> inputlist	("l", "list"		, "Text file that contains a list of images to use (one per line). "
																	"Blank lines and lines beginning with '#' are ignored."			, false	, ""		, &exist_constraint	, cli);
	TCLAP::UnlabeledValueArg<std::string> cfg		("config"	, "The darknet config filename, usually ends in \".cfg\"."			, true	, ""		, &exist_constraint	, cli);
	TCLAP::UnlabeledValueArg<std::string> weights	("weights"	, "The darknet weights filename, usually ends in \".weights\"."		, true	, ""		, &exist_constraint	, cli);
	TCLAP::UnlabeledValueArg<std::string> names		("names"	, "The darknet class names filename, usually ends in \".names\". "
													"Set to \"none\" if you don't have (or don't care about) the class names."		, true	, ""		, &exist_constraint	, cli);
	TCLAP::UnlabeledMultiArg<std::string> files		("files"	, "The name of images or videos to process with the given neural network. "
																	"May be unspecified if the --list parameter is used instead."	, false				, "files..."		, cli);

	cli.parse(argc, argv);

	if (names.getValue() == "none")
	{
		// special value -- pretend the "names" argument hasn't been set
		names.reset();
	}

	// Originally, it was very important that the cfg, weights, and names files be listed in the correct order.
	// But now we have a function that looks at the contents of the files and makes an educated guess.
	options.cfg_fn		= cfg		.getValue();
	options.weights_fn	= weights	.getValue();
	options.names_fn	= names		.getValue();
	DarkHelp::verify_cfg_and_weights(options.cfg_fn, options.weights_fn, options.names_fn);

	options.keep_annotated_images	= keep_images.getValue();
	options.use_json_output			= use_json.getValue();

	options.json["network"]["cfg"			] = options.cfg_fn;
	options.json["network"]["weights"		] = options.weights_fn;
	options.json["network"]["names"			] = options.names_fn;
	std::cout
			<< "-> config file:  " << options.cfg_fn		<< std::endl
			<< "-> weights file: " << options.weights_fn	<< std::endl
			<< "-> names file:   " << options.names_fn		<< std::endl;

	// we already verified the files several lines up, so no need to do it again
	options.dark_help.init(options.cfg_fn, options.weights_fn, options.names_fn, false);

	options.json["network"]["loading"]			=  options.dark_help.duration_string();
	std::cout << "-> loading network took " << options.dark_help.duration_string() << std::endl;

	options.dark_help.threshold							= std::stof(threshold.getValue());
	options.dark_help.hierarchy_threshold				= std::stof(hierarchy.getValue());
	options.dark_help.non_maximal_suppression_threshold	= std::stof(nms.getValue());
	options.dark_help.names_include_percentage			= get_bool(percentage);
	options.dark_help.annotation_font_scale				= std::stod(fontscale.getValue());
	options.dark_help.annotation_include_duration		= get_bool(duration);
	options.dark_help.annotation_include_timestamp		= get_bool(timestamp);
	options.force_greyscale								= greyscale.getValue();
	options.json["settings"]["threshold"]				= options.dark_help.threshold;
	options.json["settings"]["hierarchy"]				= options.dark_help.hierarchy_threshold;
	options.json["settings"]["nms"]						= options.dark_help.non_maximal_suppression_threshold;
	options.json["settings"]["include_percentage"]		= options.dark_help.names_include_percentage;
	options.json["settings"]["force_greyscale"]			= options.force_greyscale;
	options.json["settings"]["keep_annotations"]		= options.keep_annotated_images;
	if (resize1.isSet())
	{
		options.json["settings"]["resize"]				= resize1.getValue();
	}

	options.in_slideshow	= slideshow.getValue();
	options.wait_time_in_milliseconds_for_slideshow = 500;
	options.size1_is_set	= resize1.isSet();
	options.size2_is_set	= resize2.isSet();
	options.size1			= get_WxH(resize1);
	options.size2			= get_WxH(resize2);
	options.done			= false;
	options.all_files		= files.getValue();
	options.file_index		= 0;

	if (inputlist.isSet())
	{
		// we have an input list to use -- need to read the file one line at a time
		const std::string input_list_filename = inputlist.getValue();
		std::cout << "-> reading input list " << input_list_filename << std::endl;
		std::ifstream file(input_list_filename);
		std::string line;
		while (std::getline(file, line))
		{
			line.erase(0, line.find_first_not_of(" \t\r\n")); // ltrim
			if (line.empty() || line[0] == '#')
			{
				// we need to ignore this line
				continue;
			}
			options.all_files.push_back(line);
		}
	}

	if (random.getValue())
	{
		std::random_shuffle(options.all_files.begin(), options.all_files.end());
	}

	return;
}


void process_video(Options & options)
{
	cv::VideoCapture input_video;
	cv::VideoWriter output_video;

	input_video.open(options.filename);
	const double input_width	= input_video.get(cv::VideoCaptureProperties::CAP_PROP_FRAME_WIDTH	);
	const double input_height	= input_video.get(cv::VideoCaptureProperties::CAP_PROP_FRAME_HEIGHT	);
	const double input_fps		= input_video.get(cv::VideoCaptureProperties::CAP_PROP_FPS			);
	const double input_frames	= input_video.get(cv::VideoCaptureProperties::CAP_PROP_FRAME_COUNT	);

	options.json["file"][options.file_index]["original_width"	] = input_width;
	options.json["file"][options.file_index]["original_height"	] = input_height;
	options.json["file"][options.file_index]["original_fps"		] = input_fps;
	options.json["file"][options.file_index]["original_frames"	] = input_frames;

	// figure out the final image size
	cv::Mat mat;
	input_video >> mat;

	if (options.size1_is_set)
	{
		mat = resize_keeping_aspect_ratio(mat, options.size1);
	}
	if (options.size2_is_set)
	{
		mat = resize_keeping_aspect_ratio(mat, options.size2);
	}
	const int output_width	= mat.cols;
	const int output_height	= mat.rows;
	options.json["file"][options.file_index]["resized_width"	] = output_width;
	options.json["file"][options.file_index]["resized_height"	] = output_height;

	std::string short_filename = options.filename;
	size_t pos = short_filename.find_last_of("/\\");
	if (pos != std::string::npos)
	{
		short_filename.erase(0, pos + 1);
	}
	pos = short_filename.rfind(".");
	if (pos != std::string::npos)
	{
		short_filename.erase(pos);
	}
	short_filename += "_output.mp4";
	options.json["file"][options.file_index]["output"] = short_filename;

	const double seconds = input_frames / input_fps;
	const std::string length_str =
			std::to_string(static_cast<size_t>(seconds / 60)) + "m " +
			std::to_string(static_cast<size_t>(seconds) % 60) + "s";

	std::cout << input_fps << " FPS, " << input_frames << " frames, " << input_width << "x" << input_height << " -> " << output_width << "x" << output_height << ", " << length_str << std::endl;

	output_video.open(short_filename, CV_FOURCC('m', 'p', '4', 'v'), input_fps, {output_width, output_height});

	// reset to the start of the video and process every frame
	input_video.set(cv::VideoCaptureProperties::CAP_PROP_POS_FRAMES, 0.0);

	const size_t rounded_fps = std::round(input_fps);
	size_t number_of_frames = 0;
	while (true)
	{
		cv::Mat frame;
		input_video >> frame;
		if (frame.empty())
		{
			break;
		}

		if (options.force_greyscale)
		{
			// libdarknet.so segfaults when given single-channel images, so convert it back to a 3-channel image
			cv::Mat tmp;
			cv::cvtColor(frame, tmp, cv::COLOR_BGR2GRAY);
			cv::cvtColor(tmp, frame, cv::COLOR_GRAY2BGR);
		}

		if (options.size1_is_set)
		{
			frame = resize_keeping_aspect_ratio(frame, options.size1);
		}

		options.dark_help.predict(frame);
		frame = options.dark_help.annotate();

		if (options.size2_is_set)
		{
			frame = resize_keeping_aspect_ratio(frame, options.size2);
		}

		number_of_frames ++;
		output_video.write(frame);

		if (number_of_frames == input_frames or (number_of_frames % rounded_fps) == 0)
		{
			std::cout << "\rprocessing frame " << number_of_frames << "/" << input_frames << " (" << std::round(number_of_frames * 100 / input_frames) << "%)" << std::flush;

			if (options.use_json_output == false)
			{
				cv::namedWindow("DarkHelp", cv::WINDOW_NORMAL | cv::WINDOW_KEEPRATIO | cv::WINDOW_GUI_EXPANDED);
				cv::setWindowTitle("DarkHelp", short_filename);
				cv::imshow("DarkHelp", frame);
				cv::waitKeyEx(1);
			}
		}
	}
	std::cout << std::endl;

	options.json["file"][options.file_index]["frames"] = number_of_frames;
	options.file_index ++;

	return;
}


void process_image(Options & options)
{
	cv::Mat input_image;

	try
	{
		if (options.force_greyscale)
		{
			cv::Mat tmp = cv::imread(options.filename, cv::IMREAD_GRAYSCALE);
			// libdarknet.so segfaults when given single-channel images, so convert it back to a 3-channel image
			// forward_network() -> forward_convolutional_layer() -> im2col_cpu_ext()
			cv::cvtColor(tmp, input_image, cv::COLOR_GRAY2BGR);
		}
		else
		{
			input_image = cv::imread(options.filename);
		}
	}
	catch (...) {}

	if (input_image.empty())
	{
		const auto msg = "Failed to read the image \"" + options.filename + "\".";
		options.json["file"][options.file_index]["error"] = msg;
		std::cout << msg << std::endl;
		options.file_index ++;
		return;
	}

	options.json["file"][options.file_index]["original_width"	] = input_image.cols;
	options.json["file"][options.file_index]["original_height"	] = input_image.rows;

	if (options.size1_is_set)
	{
		if (input_image.cols != options.size1.width or
			input_image.rows != options.size1.height)
		{
			const auto msg = "resizing input image from " + std::to_string(input_image.cols) + "x" + std::to_string(input_image.rows) + " to " + std::to_string(options.size1.width) + "x" + std::to_string(options.size1.height);

			options.json["file"][options.file_index]["msg"] = msg;
			std::cout << "-> " << msg << std::endl;
			input_image = resize_keeping_aspect_ratio(input_image, options.size1);
		}
	}

	options.json["file"][options.file_index]["resized_width"	] = input_image.cols;
	options.json["file"][options.file_index]["resized_height"	] = input_image.rows;

	const auto results = options.dark_help.predict(input_image);

	std::cout	<< "-> prediction took " << options.dark_help.duration_string()	<< std::endl
				<< "-> " << results												<< std::endl;

	cv::Mat output_image;
	if (options.keep_annotated_images or options.use_json_output == false)
	{
		output_image = options.dark_help.annotate();
		if (options.size2_is_set)
		{
			std::cout << "-> resizing output image from " << output_image.cols << "x" << output_image.rows << " to " << options.size2.width << "x" << options.size2.height << std::endl;
			output_image = resize_keeping_aspect_ratio(output_image, options.size2);
		}

		if (options.keep_annotated_images)
		{
			// save the annotated image to disk

			const auto pid = getpid();
			std::string basedir = "/tmp";
			if (options.all_files.size() > 1)
			{
				// since we're dealing with multiple images at once, put them in a subdirectory
				basedir += "/darkhelp_" + std::to_string(pid);
				mkdir(basedir.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
			}

			const std::string output_filename = basedir + "/darkhelp_" + std::to_string(pid) + "_output_" + std::to_string(options.file_index) + ".png";
			cv::imwrite(output_filename, output_image, {CV_IMWRITE_PNG_COMPRESSION, 9});
			std::cout << "-> annotated image saved to \"" << output_filename << "\"" << std::endl;
			options.json["file"][options.file_index]["annotated_image"] = output_filename;
		}
	}

	if (options.use_json_output)
	{
		options.json["file"][options.file_index]["duration"	] = options.dark_help.duration_string();
		options.json["file"][options.file_index]["count"	] = results.size();

		size_t count = 0;
		for (const auto & pred : results)
		{
			auto & j = options.json["file"][options.file_index]["prediction"][count];

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
				j["all_probabilities"][prop_count]["name"			] = options.dark_help.names[prop.first];
				prop_count ++;
			}

			count ++;
		}

		// move to the next image
		options.file_index ++;
		return;
	}

	// If we get here then we're showing GUI windows to the user with the results.

	std::string short_filename = options.filename;
	size_t pos = short_filename.find_last_of("/\\");
	if (pos != std::string::npos)
	{
		short_filename.erase(0, pos + 1);
	}

	int delay_in_milliseconds = 0; // wait forever

	const auto now = std::chrono::high_resolution_clock::now();
	clear_old_msg(now);
	display_current_msg(options.dark_help, now, output_image, delay_in_milliseconds);

	if (options.in_slideshow)
	{
		// slideshow delay overrides whatever delay we may have put due to a message
		delay_in_milliseconds = options.wait_time_in_milliseconds_for_slideshow;
	}

	cv::namedWindow("DarkHelp", cv::WINDOW_NORMAL | cv::WINDOW_KEEPRATIO | cv::WINDOW_GUI_EXPANDED);
	cv::setWindowTitle("DarkHelp", short_filename);
	cv::imshow("DarkHelp", output_image);

	const int key = cv::waitKeyEx(delay_in_milliseconds);
	std::cout << "KEY=" << key << std::endl;

	if (key == -1 && options.in_slideshow == false)
	{
		// we didn't really timeout, this is simply an indication that a message needs to be erased
		return;
	}

	switch (key)
	{
		case 0x00100071:	// 'q'
		case 0x0010001b:	// ESC
		{
			options.done = true;
			break;
		}
		case 0x00100067:	// 'g'
		{
			options.force_greyscale = not options.force_greyscale;
			break;
		}
		case 0x00100077:	// 'w'
		{
			// save the file to disk, then re-load the same image
			const std::string output_filename = "output.png";
			cv::imwrite(output_filename, output_image, {CV_IMWRITE_PNG_COMPRESSION, 9});
			std::cout << "-> output image saved to \"" << output_filename << "\"" << std::endl;
			add_msg("saved image to \"" + output_filename + "\"");
			break;
		}
		case 0x00100068:	// 'h'
		{
			show_help_window();
			break;
		}
		case 0x0010ff50:	// HOME
		{
			options.in_slideshow = false;
			options.file_index = 0;
			break;
		}
		case 0x0010ff57:	// END
		{
			options.in_slideshow = false;
			options.file_index = options.all_files.size() - 1;
			break;
		}
		case 0x0010ff51:	// LEFT
		{
			options.in_slideshow = false;
			if (options.file_index > 0)
			{
				options.file_index --;
			}
			break;
		}
		case 0x0010ff52:	// UP
		{
			// quicker slideshow
			options.wait_time_in_milliseconds_for_slideshow *= 0.5;
			if (options.wait_time_in_milliseconds_for_slideshow < 50)
			{
				options.wait_time_in_milliseconds_for_slideshow = 50;
			}
			std::cout << "-> slideshow timeout has been decreased to " << options.wait_time_in_milliseconds_for_slideshow << " milliseconds" << std::endl;
			add_msg("slideshow timer: " + std::to_string(options.wait_time_in_milliseconds_for_slideshow) + " milliseconds");
			options.in_slideshow = true;
			break;
		}
		case 0x0010ff54:	// DOWN
		{
			// slower slideshow
			options.wait_time_in_milliseconds_for_slideshow /= 0.5;
			std::cout << "-> slideshow timeout has been increased to " << options.wait_time_in_milliseconds_for_slideshow << " milliseconds" << std::endl;
			add_msg("slideshow timer: " + std::to_string(options.wait_time_in_milliseconds_for_slideshow) + " milliseconds");
			options.in_slideshow = true;
			break;
		}
		case 0x0010ff55:	// PAGE-UP
		{
			options.dark_help.threshold += 0.1;
			if (options.dark_help.threshold > 1.0)
			{
				options.dark_help.threshold = 1.0;
			}
			add_msg("increased threshold: " + std::to_string((int)std::round(options.dark_help.threshold * 100.0)) + "%");
			break;
		}
		case 0x0010ff56:	// PAGE-DOWN
		{
			options.dark_help.threshold -= 0.1;
			if (options.dark_help.threshold < 0.01)
			{
				options.dark_help.threshold = 0.001; // not a typo, allow the lower limit to be 0.1%
			}
			add_msg("decreased threshold: " + std::to_string((int)std::round(options.dark_help.threshold * 100.0)) + "%");
			break;
		}
		case 0x00100070:	// 'p'
		{
			options.in_slideshow = not options.in_slideshow;
			// don't increment the image index, stay on this image
			break;
		}
		default:
		{
			options.file_index ++;
			break;
		}
	}

	return;
}


int main(int argc, char *argv[])
{
	try
	{
		Options options;

		init(options, argc, argv);

		magic_t magic_cookie = magic_open(MAGIC_MIME_TYPE);
		magic_load(magic_cookie, nullptr);

		add_msg("press 'h' for help");

		options.file_index = 0;
		while (options.file_index < options.all_files.size() && not options.done)
		{
			options.filename = options.all_files.at(options.file_index);

			const std::string mime_type = magic_file(magic_cookie, options.filename.c_str());
			const bool is_image = (mime_type.find("image/") == 0);
			const bool is_video = (mime_type.find("video/") == 0);

			options.json["file"][options.file_index]["filename"	] = options.filename;
			options.json["file"][options.file_index]["type"		] = mime_type;
			std::cout << "#" << (1 + options.file_index) << "/" << options.all_files.size() << ": loading \"" << options.filename << "\"" << std::endl;

			if (is_video)
			{
				process_video(options);
			}
			else if (is_image)
			{
				process_image(options);
			}
			else
			{
				// what type of file is this!?
				const auto msg = "Unknown file type: \"" + options.filename + "\".";
				options.json["file"][options.file_index]["error"] = msg;
				std::cout << msg << std::endl;
				options.file_index ++;
			}
		}

		// Once we get here, we're done the loop.  Either we've shown all the images, or the user has pressed 'q' to quit.

		magic_close(magic_cookie);

		if (options.json.empty() or options.use_json_output == false)
		{
			if (options.file_index >= options.all_files.size())
			{
				std::cout << "Done processing " << options.all_files.size() << " files...exiting." << std::endl;
			}
			else
			{
				std::cout << "Exiting!" << std::endl;
			}
		}
		else
		{
			std::cout	<< "JSON OUTPUT"		<< std::endl
						<< options.json.dump(4)	<< std::endl;
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
