/* DarkHelp - C++ helper class for Darknet's C API.
 * Copyright 2019-2024 Stephane Charette <stephanecharette@gmail.com>
 * MIT license applies.  See "license.txt" for details.
 */

#include "DarkHelp.hpp"
#include <random>
#include <chrono>
#include <fstream>
#include <atomic>
#include <map>
#include <vector>
#include <deque>
#include <string>
#include <ctime>
#include <csignal>
#include <tclap/CmdLine.h>	// "sudo apt-get install libtclap-dev"
#include "json.hpp"
#include "filesystem.hpp"


#ifdef WIN32

#pragma warning(disable: 4244)
#pragma warning(disable: 4305)

// I need to figure out how to get libmagic compiled in Windows, or decide
// to remove it from this project.  For now, create dummy prototypes so the
// code compiles in Windows.  https://github.com/microsoft/vcpkg/issues/13528
typedef int magic_t;
#define MAGIC_MIME_TYPE 0
magic_t magic_open(int flags) { return 0; }
void magic_close(magic_t cookie) { return; }
int magic_load(magic_t cookie, const char* filename) { return 0; }
const char* magic_file(magic_t cookie, const char* filename)
{
	// Poor man's version of libmagic for Windows.  Look for a few common
	// file types we expect DarkHelp to have to handle.  Unlike the real
	// libmagic, this doesn't examine the contents of the file.  Instead,
	// it is comparing the filename extension against a built-in list.
	//
	// Return string should look similar to "image/jpeg" or "video/mpeg".

	std::string extension = filename;
	size_t pos = extension.rfind(".");
	if (pos != std::string::npos)
	{
		extension.erase(0, pos + 1);
	}
	std::transform(extension.begin(), extension.end(), extension.begin(), [](unsigned char c) { return std::tolower(c); });

	const std::map<std::string, std::string> m =
	{
		{"avi"	, "video"},
		{"flv"	, "video"},
		{"gif"	, "image"},
		{"jpeg"	, "image"},
		{"jpg"	, "image"},
		{"m4a"	, "video"},
		{"m4v"	, "video"},
		{"mkv"	, "video"},
		{"mov"	, "video"},
		{"mp4"	, "video"},
		{"mpeg4", "video"},
		{"mpeg"	, "video"},
		{"ogg"	, "video"},
		{"png"	, "image"},
		{"qt"	, "video"},
		{"tiff"	, "image"},
		{"tif"	, "image"},
		{"webm"	, "video"},
		{"wmv"	, "video"}
	};

	static std::string result;
	result = "unknown/" + extension;

	// now we loop through the map of extensions and see if this matches one of the common ones
	for (const auto & [key, val] : m)
	{
		if (extension == key)
		{
			result = val + "/" + key;
			break;
		}
	}

	return result.c_str();
}
#else
#include <unistd.h>
#include <magic.h>
#endif


// possible return values from cv::waitKeyEx()
const int KEY_SIMPLE_MASK	= 0xff;
const int KEY_ESC			= 0x1b;
const int KEY_c				= 0x63;
const int KEY_d				= 0x64;
const int KEY_g				= 0x67;
const int KEY_h				= 0x68;
const int KEY_l				= 0x6c;
const int KEY_p				= 0x70;
const int KEY_q				= 0x71;
const int KEY_s				= 0x73;
const int KEY_t				= 0x74;
const int KEY_u				= 0x75;
const int KEY_w				= 0x77;
#ifdef WIN32
const int KEY_COMPLEX_MASK	= 0x00ffffff;
const int KEY_PAGE_UP		= 0x00210000;
const int KEY_PAGE_DOWN 	= 0x00220000;
const int KEY_END			= 0x00230000;
const int KEY_HOME			= 0x00240000;
const int KEY_LEFT			= 0x00250000;
const int KEY_UP			= 0x00260000;
const int KEY_RIGHT			= 0x00270000;
const int KEY_DOWN			= 0x00280000;
#else
const int KEY_COMPLEX_MASK	= 0x00ffffff;
const int KEY_HOME			= 0x0010ff50;
const int KEY_LEFT			= 0x0010ff51;
const int KEY_UP			= 0x0010ff52;
const int KEY_RIGHT			= 0x0010ff53;
const int KEY_DOWN			= 0x0010ff54;
const int KEY_PAGE_UP		= 0x0010ff55;
const int KEY_PAGE_DOWN 	= 0x0010ff56;
const int KEY_END			= 0x0010ff57;
#endif


std::atomic<bool> signal_raised = false;
extern "C"
{
	void cli_signal_handler(int sig)
	{
		std::cout << std::endl << "-> WARNING: signal handler called for signal #" << sig << std::endl;
		signal_raised = true;

		// reset this signal to the default handler so we don't get stuck in a loop of reporting the same error
		std::signal(sig, nullptr);

		return;
	}
}


struct Options
{
	magic_t			magic_cookie;
	std::string		cfg_fn;
	std::string		weights_fn;
	std::string		names_fn;
	std::string		neural_network_name; // descriptive name made by combining the .cfg and .weights filenames
	std::string		out_dir;
	std::string		image_type; // either "png" or "jpg"
	bool			keep_annotated_images;
	bool			use_json_output;
	nlohmann::json	json;
	DarkHelp::NN	nn;
	bool			force_greyscale;
	bool			done;
	bool			size1_is_set;
	bool			size2_is_set;
	cv::Size		size1;
	cv::Size		size2;
	DarkHelp::VStr	all_files;
	bool			in_slideshow;
	int				wait_time_in_milliseconds_for_slideshow;
	std::string		filename;
	size_t			file_index;
	std::string		message_text;	// Message that needs to be shown to the user.  This text will be printed overtop of the image.
	std::time_t		message_time;	// Time at which the message should be cleared.

	Options() :
		magic_cookie			(0),
		keep_annotated_images	(false),
		use_json_output			(false),
		force_greyscale			(false),
		done					(false),
		size1_is_set			(false),
		size2_is_set			(false),
		in_slideshow			(false),
		wait_time_in_milliseconds_for_slideshow(500),
		file_index				(0),
		message_time			(0)
	{
		return;
	}
};


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
		{ "c"			, "Combine predictions."			},
		{ "d"			, "Toggle snapping."				},
		{ "h"			, "Show help."						},
		{ "l"			, "Toggle label display."			},
		{ "t"			, "Toggle image tiling."			},
		{ "u"			, "Toggle duration."				},
		{ "w"			, "Write image to disk."			},
		{ "q or ESC"	, "Exit from DarkHelp."				}
	};

	cv::Mat mat(450, 400, CV_8UC3, cv::Scalar(255, 255, 255));

	const auto font_face		= cv::HersheyFonts::FONT_HERSHEY_SIMPLEX;
	const auto font_scale		= 0.5;
	const auto font_thickness	= 1;

	int y = 25;
	for (const auto & [key, val] : help)
	{
		const cv::Point p1(10, y);
		const cv::Point p2(120, y);

		cv::putText(mat, key, p1, font_face, font_scale, cv::Scalar(0,0,0), font_thickness, cv::LINE_AA);
		cv::putText(mat, val, p2, font_face, font_scale, cv::Scalar(0,0,0), font_thickness, cv::LINE_AA);
		y += 25;
	}

	cv::imshow("DarkHelp v" DH_VERSION, mat);

	return;
}


void set_msg(Options & options, const std::string & msg)
{
	options.message_time = 0;
	options.message_text = msg;
	if (options.message_text.empty() == false)
	{
		std::cout << "setting message: \"" << msg << "\"" << std::endl;
	}

	return;
}


void display_current_msg(Options & options, cv::Mat output_image, int & delay_in_milliseconds)
{
	const auto now = std::time(nullptr);

	if (options.message_text.empty() or (options.message_time > 0 and options.message_time <= now))
	{
		options.message_text = "";
		options.message_time = 0;
	}
	else
	{
		if (options.message_time == 0)
		{
			// this must be the first time this message is shown, so initialize the time at which point it needs to be taken down
			options.message_time = now + 2;
		}

		const cv::Size text_size = cv::getTextSize(options.message_text, options.nn.config.annotation_font_face, options.nn.config.annotation_font_scale, options.nn.config.annotation_font_thickness, nullptr);

		cv::Point p(30, 50);
		cv::Rect r(cv::Point(p.x - 5, p.y - text_size.height - 3), cv::Size(text_size.width + 10, text_size.height + 10));
		cv::rectangle(output_image, r, cv::Scalar(0,255,255), cv::FILLED);
		cv::rectangle(output_image, r, cv::Scalar(0,0,0), 1);
		cv::putText(output_image, options.message_text, p, options.nn.config.annotation_font_face, options.nn.config.annotation_font_scale, cv::Scalar(0,0,0), options.nn.config.annotation_font_thickness, cv::LINE_AA);

		const int milliseconds_remaining = 1000 * (options.message_time - now);
		if (delay_in_milliseconds == 0 or milliseconds_remaining < delay_in_milliseconds)
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
		str == "yes"	||
		str == "on"		||
		str == "t"		||
		str == "y"		||
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
				if (f >= 0.0 and idx == value.size()) { return true; } // this is a valid float
			}
			catch (...) {}
			return false;
		}
};


// Class used to validate "int" parameters.
class IntConstraint : public TCLAP::Constraint<std::string>
{
	public:

		virtual std::string description() const	{ return "positive integer"; }
		virtual std::string shortID() const		{ return "positive integer"; }
		virtual bool check(const std::string & value) const
		{
			try
			{
				size_t idx = 0;
				const int i = std::stoi(value, &idx);
				if (i >= 0 and idx == value.size()) { return true; } // valid positive int
			}
			catch (...) {}
			return false;
		}
};


// Class used to validate the output image type (png vs jpg)
class OutputImageConstraint : public TCLAP::Constraint<std::string>
{
public:

	virtual std::string description() const	{ return "known image type of \"png\" or \"jpg\""; }
	virtual std::string shortID() const		{ return "type"; }
	virtual bool check(const std::string & value) const
	{
		if (value == "png" or value == "jpg")
		{
			return true;
		}
		return false;
	}
};


// Class used to validate that an input file exists.
class FileExistConstraint : public TCLAP::Constraint<std::string>
{
	public:

		virtual std::string description() const	{ return "file must exist"; }
		virtual std::string shortID() const		{ return "filename"; }
		virtual bool check(const std::string & value) const
		{
			return std::filesystem::exists(value);
		}
};


// Class used to validate that a directory exists.
class DirExistConstraint : public TCLAP::Constraint<std::string>
{
	public:

		virtual std::string description() const	{ return "directory must exist"; }
		virtual std::string shortID() const		{ return "dirname"; }
		virtual bool check(const std::string & value) const
		{
			if (std::filesystem::exists(value))
			{
				if (std::filesystem::is_directory(value))
				{
					return true;
				}
			}
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
				if (s.width >= 10 and s.height >= 10) { return true; }
			}
			catch (...) {}
			return false;
		}
};


class DriverConstraint : public TCLAP::Constraint<std::string>
{
	public:

		virtual std::string description() const	{ return "darknet|opencv|opencvcpu"; }
		virtual std::string shortID() const		{ return "darknet|opencv|opencvcpu"; }
		virtual bool check(const std::string & value) const
		{
			return value == "darknet" or value == "opencv" or value == "opencvcpu";
		}
};


void init(Options & options, int argc, char *argv[])
{
	std::srand(std::time(nullptr)); // seed random number generator

	TCLAP::CmdLine cli("Load a darknet neural network and run prediction on the given image file(s).", ' ', DH_VERSION);

	std::vector<std::string> booleans = { "true", "false", "on", "off", "yes", "no", "t", "f", "y", "n", "1", "0" };
	auto allowed_booleans = TCLAP::ValuesConstraint<std::string>(booleans);
	auto float_constraint = FloatConstraint();
	auto int_constraint = IntConstraint();
	auto WxH_constraint = WxHConstraint();
	auto dir_exist_constraint = DirExistConstraint();
	auto file_exist_constraint = FileExistConstraint();
	auto image_type_constraint = OutputImageConstraint();
	auto driver_constraint = DriverConstraint();

	TCLAP::ValueArg<std::string> resize2			("a", "resize2"		, "Resize the output image (\"after\") to \"WxH\"."															, false, "640x480"	, &WxH_constraint		, cli);
	TCLAP::ValueArg<std::string> resize1			("b", "resize1"		, "Resize the input image (\"before\") to \"WxH\"."															, false, "640x480"	, &WxH_constraint		, cli);
	TCLAP::ValueArg<std::string> duration			("d", "duration"	, "Determines if the duration is added to annotations."														, false, "true"		, &allowed_booleans		, cli);
	TCLAP::ValueArg<std::string> driver				("D", "driver"		, "Determines if Darknet or OpenCV DNN is used. Default is \"darknet\"."									, false, "darknet"	, &driver_constraint	, cli);
	TCLAP::ValueArg<std::string> debug				("", "debug"		, "Enable debug output. Default is \"false\"."																, false, "false"	, &allowed_booleans		, cli);
	TCLAP::ValueArg<std::string> shade				("e", "shade"		, "Amount of alpha-blending to use when shading in rectangles. Default is 0.25."							, false, "0.25"		, &float_constraint		, cli);
	TCLAP::ValueArg<std::string> line_thickness		("", "line"			, "Thickness of annotation lines in pixels. Default is 2."													, false, "2"		, &int_constraint		, cli);
	TCLAP::ValueArg<std::string> fontscale			("f", "fontscale"	, "Determines how the font is scaled for annotations. Default is 0.5."										, false, "0.5"		, &float_constraint		, cli);
	TCLAP::SwitchArg greyscale						("g", "greyscale"	, "Force the images to be loaded in greyscale."																											, cli, false );
	TCLAP::ValueArg<std::string> timestamp			("i", "timestamp"	, "Determines if a timestamp is added to annotations."														, false, "false"	, &allowed_booleans		, cli);
	TCLAP::SwitchArg use_json						("j", "json"		, "Enable JSON output (useful when DarkHelp is used in a shell script)."																				, cli, false );
	TCLAP::SwitchArg keep_images					("k", "keep"		, "Keep annotated images (write images to disk). Especially useful when combined with the -j option."													, cli, false );
	TCLAP::ValueArg<std::string> inputlist			("l", "list"		, "Text file that contains a list of images to use (one per line). "
																		"Blank lines and lines beginning with '#' are ignored."														, false	, ""		, &file_exist_constraint, cli);
	TCLAP::ValueArg<std::string> nms				("n", "nms"			, "The non-maximal suppression threshold to use when predicting. Default is 0.45."							, false, "0.45"		, &float_constraint		, cli);
	TCLAP::ValueArg<std::string> autohide			("o", "autohide"	, "Auto-hide labels."																						, false, "true"		, &allowed_booleans		, cli);
	TCLAP::ValueArg<std::string> percentage			("p", "percentage"	, "Determines if percentages are added to annotations."														, false, "true"		, &allowed_booleans		, cli);
	TCLAP::SwitchArg random							("r", "random"		, "Randomly shuffle the set of images."																													, cli, false );
	TCLAP::SwitchArg slideshow						("s", "slideshow"	, "Show the images in a slideshow."																														, cli, false );
	TCLAP::ValueArg<std::string> snapping			("S", "snapping"	, "Snap the annotations."																					, false, "false"	, &allowed_booleans		, cli);
	TCLAP::ValueArg<std::string> snap_horizontal_tolerance	("", "snap-horizontal-tolerance", "Snap horizontal tolerance, in pixels. Only used when snapping is enabled. Default is 5."	, false, "5"	, &int_constraint		, cli);
	TCLAP::ValueArg<std::string> snap_vertical_tolerance	("", "snap-vertical-tolerance"	, "Snap vertical tolerance, in pixels. Only used when snapping is enabled. Default is 5."	, false, "5"	, &int_constraint		, cli);
	TCLAP::ValueArg<std::string> threshold			("t", "threshold"	, "The threshold to use when predicting with the neural net. Default is 0.5."								, false, "0.5"		, &float_constraint		, cli);
	TCLAP::ValueArg<std::string> use_tiles			("T", "tiles"		, "Determines if large images are processed by breaking into tiles. Default is \"false\"."					, false, "false"	, &allowed_booleans		, cli);
	TCLAP::ValueArg<std::string> hierarchy			("y", "hierarchy"	, "The hierarchy threshold to use when predicting. Default is 0.5."											, false, "0.5"		, &float_constraint		, cli);
	TCLAP::ValueArg<std::string> image_type			("Y", "type"		, "The image type to use when --keep has also been enabled. Can be \"png\" or \"jpg\". Default is \"png\"."	, false, "png"		, &image_type_constraint, cli);
	TCLAP::ValueArg<std::string> out_dir			("", "outdir"		, "Output directory to use when --keep has also been enabled. Default is /tmp/."							, false, ""			, &dir_exist_constraint	, cli);
	TCLAP::ValueArg<std::string> pixelate			("", "pixelate"		, "Determines if predictions are pixelated in the output annotation image. Default is false."				, false, "false"	, &allowed_booleans		, cli);
	TCLAP::ValueArg<std::string> redirection		("", "redirection"	, "Determines if STDOUT and STDERR redirection will be performed when Darknet loads. Default is false."		, false, "false"	, &allowed_booleans		, cli);
	TCLAP::SwitchArg suppress						("", "suppress"		, "Suppress all labels (bounding boxes are shown, but not the labels at the top of each bounding box)."													, cli, false );
	TCLAP::ValueArg<std::string> tile_edge			("", "tile-edge"	, "How close objects must be to tile edges to be re-combined. Range is 0.01-1.0+. Default is 0.25."			, false, "0.25"		, &float_constraint		, cli);
	TCLAP::ValueArg<std::string> tile_rect			("", "tile-rect"	, "How similarly objects must line up across tiles to be re-combined. Range is 1.0-2.0+. Default is 1.20."	, false, "1.2"		, &float_constraint		, cli);
	TCLAP::UnlabeledValueArg<std::string> cfg		("config"			, "The darknet config filename, usually ends in \".cfg\"."													, true	, ""		, &file_exist_constraint, cli);
	TCLAP::UnlabeledValueArg<std::string> weights	("weights"			, "The darknet weights filename, usually ends in \".weights\"."												, true	, ""		, &file_exist_constraint, cli);
	TCLAP::UnlabeledValueArg<std::string> names		("names"			, "The darknet class names filename, usually ends in \".names\". "
																		"Set to \"none\" if you don't have (or don't care about) the class names."									, true	, ""		, &file_exist_constraint, cli);
	TCLAP::UnlabeledMultiArg<std::string> files		("files"			, "The name of images or videos to process with the given neural network. "
																		"May be unspecified if the --list parameter is used instead."												, false				, "files..."			, cli);

	std::string cmd;
	for (int i = 0; i < argc; i++)
	{
		if (cmd.empty() == false)
		{
			cmd += " ";
		}
		cmd += argv[i];

		if (cmd.size() > 1000)
		{
			cmd += " ...";
			break;
		}
	}
	options.json["argv"] = cmd;

	cli.parse(argc, argv);

	if (names.getValue() == "none")
	{
		// special value -- pretend the "names" argument hasn't been set
		names.reset();
	}

	const std::time_t tt = std::time(nullptr);
	auto lt = std::localtime(&tt);
	char buffer[50];
	std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S %z", lt);
	options.json["timestamp"]["epoch"]	= tt;
	options.json["timestamp"]["text"]	= buffer;

	options.magic_cookie = magic_open(MAGIC_MIME_TYPE);
	magic_load(options.magic_cookie, nullptr);

	// Originally, it was very important that the cfg, weights, and names files be listed in the correct order.
	// But now we have a function that looks at the contents of the files and makes an educated guess.
	options.cfg_fn		= cfg		.getValue();
	options.weights_fn	= weights	.getValue();
	options.names_fn	= names		.getValue();
	const auto debug_messages = DarkHelp::verify_cfg_and_weights(options.cfg_fn, options.weights_fn, options.names_fn);

	options.image_type				= image_type	.getValue();
	options.out_dir					= out_dir		.getValue();
	options.keep_annotated_images	= keep_images	.getValue();
	options.use_json_output			= use_json		.getValue();

	#ifndef HAVE_OPENCV_HIGHGUI
	// if OpenCV does not have HighGUI, then pretend we're running with the --json flag
	// so we don't attempt to open up any GUI window
	options.use_json_output			= true;
	#endif

	options.json["network"]["cfg"			] = options.cfg_fn;
	options.json["network"]["weights"		] = options.weights_fn;
	options.json["network"]["names"			] = options.names_fn;
	std::cout
			<< "-> config file:  " << options.cfg_fn		<< std::endl
			<< "-> weights file: " << options.weights_fn	<< std::endl
			<< "-> names file:   " << options.names_fn		<< std::endl;

	DarkHelp::EDriver darkhelp_driver = DarkHelp::EDriver::kDarknet;
	if (driver.isSet())
	{
		if (driver.getValue() == "darknet")
		{
			darkhelp_driver = DarkHelp::EDriver::kDarknet;
		}
		else if (driver.getValue() == "opencv")
		{
			darkhelp_driver = DarkHelp::EDriver::kOpenCV;
		}
		else if (driver.getValue() == "opencvcpu")
		{
			darkhelp_driver = DarkHelp::EDriver::kOpenCVCPU;
		}
	}
	std::cout
		<< "-> driver:       "
		<< (darkhelp_driver == DarkHelp::EDriver::kDarknet ? "Darknetd" :
			darkhelp_driver == DarkHelp::EDriver::kOpenCV ? "OpenCV DNN" :
			darkhelp_driver == DarkHelp::EDriver::kOpenCVCPU ? "OpenCV DNN (CPU only)" :
			"UNKNOWN")
		<< std::endl;

	// we already verified the files several lines up, so no need to do it again
	options.nn.config.redirect_darknet_output = get_bool(redirection);
	options.nn.init(options.cfg_fn, options.weights_fn, options.names_fn, false, darkhelp_driver);

	if (options.neural_network_name.empty())
	{
		const auto cfg_fn		= std::filesystem::path(options.cfg_fn		);
		const auto weights_fn	= std::filesystem::path(options.weights_fn	);

		if (weights_fn.stem().string().find(cfg_fn.stem().string()) != std::string::npos)
		{
			// the cfg and the weights have similar names, so we only need to specify 1 file
			options.neural_network_name = weights_fn.filename().string();
		}
		else
		{
			// the cfg and the weights have completely different names
			options.neural_network_name = cfg_fn.filename().string() + "+" + weights_fn.filename().string();
		}
	}

	if (get_bool(debug))
	{
		options.nn.config.enable_debug = true;
		std::cout << "-> debug output: enabled" << std::endl;

		for (const auto & [key, val] : debug_messages)
		{
			std::cout << "-> debug msg:    " << key << ": " << val << std::endl;
		}

		for (size_t i = 0; i < options.nn.names.size(); i++)
		{
			std::cout << "-> class #" << i << ": " << options.nn.names[i] << std::endl;
		}
	}

	options.json["network"]["loading"]				=  options.nn.duration_string();
	std::cout	<< "-> loading network took "		<< options.nn.duration_string() << std::endl
				<< "-> neural network dimensions: "	<< options.nn.network_size().width << "x" << options.nn.network_size().height << std::endl;

	options.nn.config.threshold							= std::stof(threshold.getValue());
	options.nn.config.hierarchy_threshold				= std::stof(hierarchy.getValue());
	options.nn.config.non_maximal_suppression_threshold	= std::stof(nms.getValue());
	options.nn.config.names_include_percentage			= get_bool(percentage);
	options.nn.config.annotation_line_thickness			= std::stoi(line_thickness.getValue());
	options.nn.config.annotation_font_scale				= std::stod(fontscale.getValue());
	options.nn.config.annotation_include_duration		= get_bool(duration);
	options.nn.config.annotation_include_timestamp		= get_bool(timestamp);
	options.nn.config.annotation_shade_predictions		= std::stof(shade.getValue());
	options.nn.config.annotation_auto_hide_labels		= get_bool(autohide);
	options.nn.config.enable_tiles						= get_bool(use_tiles);
	options.nn.config.tile_edge_factor					= std::stof(tile_edge.getValue());
	options.nn.config.tile_rect_factor					= std::stof(tile_rect.getValue());
	options.nn.config.snapping_enabled					= get_bool(snapping);
	options.nn.config.snapping_horizontal_tolerance		= std::stoi(snap_horizontal_tolerance.getValue());
	options.nn.config.snapping_vertical_tolerance		= std::stoi(snap_vertical_tolerance.getValue());
	options.nn.config.annotation_pixelate_enabled		= get_bool(pixelate);
	options.nn.config.redirect_darknet_output			= get_bool(redirection);

	if (suppress.isSet())
	{
		// user doesn't want labels, so assume they also want thin lines
		options.nn.config.annotation_suppress_all_labels	= true;
		options.nn.config.annotation_line_thickness			= 1;
	}

	options.force_greyscale								= greyscale.getValue();
	options.json["settings"]["driver"]					= driver.getValue();
	options.json["settings"]["threshold"]				= options.nn.config.threshold;
	options.json["settings"]["hierarchy"]				= options.nn.config.hierarchy_threshold;
	options.json["settings"]["nms"]						= options.nn.config.non_maximal_suppression_threshold;
	options.json["settings"]["include_percentage"]		= options.nn.config.names_include_percentage;
	options.json["settings"]["force_greyscale"]			= options.force_greyscale;
	options.json["settings"]["keep_annotations"]		= options.keep_annotated_images;
	options.json["settings"]["enable_tiles"]			= options.nn.config.enable_tiles;
	options.json["settings"]["snapping"]				= options.nn.config.snapping_enabled;
	options.json["settings"]["output_redirection"]		= options.nn.config.redirect_darknet_output;

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
	options.file_index		= 0;

	if (options.out_dir.empty())
	{
		// default to "current directory"
		options.out_dir = ".";

		// ...but if we're going to be keeping a lot of images, create something new in /tmp/
		if (options.keep_annotated_images)
		{
			auto dir = std::filesystem::temp_directory_path();

			#ifdef WIN32
			const auto pid = _getpid();
			#else
			const auto pid = getpid();
			#endif
			dir /= "darkhelp_" + std::to_string(pid);
			std::filesystem::create_directories(dir);

			options.out_dir = dir.string();
		}
	}

	std::cout << "-> output directory: " << options.out_dir << std::endl;
	options.json["settings"]["outdir"] = options.out_dir;

	std::cout << "-> looking for image and video files" << std::endl;
	size_t number_of_files_skipped = 0;

	for (const auto & fn : files.getValue())
	{
		if (signal_raised)
		{
			break;
		}

		std::filesystem::path name(fn);
		if (std::filesystem::exists(name) == false)
		{
			throw std::invalid_argument("\"" + fn + "\" does not exist or is not accessible");
		}

		if (std::filesystem::is_directory(name) == false)
		{
			// simple filename, not subdirectory, so add the name to our list
			options.all_files.push_back(fn);
			continue;
		}

		// if we get here, then we've been given a subdirectory instead of a specific filename

		for (const auto & entry : std::filesystem::recursive_directory_iterator(name,  std::filesystem::directory_options::follow_directory_symlink))
		{
			if (signal_raised)
			{
				break;
			}

			const std::string filename = entry.path().string();
			if (std::filesystem::is_directory(entry.path()) == false)
			{
				const std::string mime_type = magic_file(options.magic_cookie, filename.c_str());
				const bool is_image = (mime_type.find("image/") == 0);
				const bool is_video = (mime_type.find("video/") == 0);
				if (is_image or is_video)
				{
					options.all_files.push_back(filename);
				}
				else
				{
					number_of_files_skipped ++;
				}
			}
		}
	}

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

	std::cout << "-> found " << options.all_files.size() << " file" << (options.all_files.size() == 1 ? "" : "s");
	if (number_of_files_skipped)
	{
		std::cout << " (" << number_of_files_skipped << " skipped)";
	}
	std::cout << std::endl;

	if (random.getValue())
	{
		std::random_device rd;
		std::mt19937 rng(rd());
		std::shuffle(options.all_files.begin(), options.all_files.end(), rng);
	}
	else
	{
		std::sort(options.all_files.begin(), options.all_files.end());
	}

	return;
}


void process_video(Options & options)
{
	cv::VideoCapture input_video;
	cv::VideoWriter output_video;

	const bool success = input_video.open(options.filename);
	if (not success)
	{
		throw std::runtime_error("failed to open video file " + options.filename);
	}

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
		mat = DarkHelp::resize_keeping_aspect_ratio(mat, options.size1);
	}
	if (options.size2_is_set)
	{
		mat = DarkHelp::resize_keeping_aspect_ratio(mat, options.size2);
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
	auto long_filename = std::filesystem::path(options.out_dir) / short_filename;

	options.json["file"][options.file_index]["output"] = long_filename.string();

	const double seconds = input_frames / input_fps;
	const std::string length_str =
			std::to_string(static_cast<size_t>(seconds / 60)) + "m " +
			std::to_string(static_cast<size_t>(seconds) % 60) + "s";

	std::cout << input_fps << " FPS, " << input_frames << " frames, " << input_width << "x" << input_height << " -> " << output_width << "x" << output_height << ", " << length_str << std::endl;

	/* For videos, having the duration flash at every frame is next to useless.  Instead, we're going to do a running average
	 * over the last few seconds.  We'll calculate the average and overwrite the value inside the DarkHelp object prior to
	 * annotating the frame.
	 */
	std::deque<std::chrono::high_resolution_clock::duration> duration_deque;

	bool show_video = false;
	if (options.use_json_output == false)
	{
		show_video = true;
		cv::namedWindow("DarkHelp", cv::WINDOW_NORMAL | cv::WINDOW_KEEPRATIO | cv::WINDOW_GUI_EXPANDED);
		cv::setWindowTitle("DarkHelp", options.neural_network_name + " - " + short_filename);
		cv::imshow("DarkHelp", mat);
		cv::resizeWindow("DarkHelp", output_width, output_height);
		cv::waitKey(50); // give the initial window some time to display itself before we start to loop
	}

	output_video.open(long_filename.string(), cv::VideoWriter::fourcc('m', 'p', '4', 'v'), input_fps, {output_width, output_height});

	// reset to the start of the video and process every frame
	input_video.set(cv::VideoCaptureProperties::CAP_PROP_POS_FRAMES, 0.0);

	double previous_fps = 0.0;
	size_t number_of_frames = 0;
	const size_t rounded_fps = std::round(input_fps);
	const auto start_time = std::chrono::high_resolution_clock::now();
	while (signal_raised == false)
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
			frame = DarkHelp::resize_keeping_aspect_ratio(frame, options.size1);
		}

		options.nn.predict(frame);

		// no need to figure out the average duration if the display of the duration field is turned off in annotate()
		if (options.nn.config.annotation_include_duration)
		{
			duration_deque.push_front(options.nn.duration);
			if (duration_deque.size() > 3 * rounded_fps)
			{
				duration_deque.resize(3 * rounded_fps);
			}
			std::chrono::high_resolution_clock::duration average = std::chrono::milliseconds(0);
			for (auto && duration : duration_deque)
			{
				average += duration;
			}
			average /= duration_deque.size();
			options.nn.duration = average;
		}

		frame = options.nn.annotate();

		if (options.size2_is_set)
		{
			frame = DarkHelp::resize_keeping_aspect_ratio(frame, options.size2);
		}

		number_of_frames ++;
		output_video.write(frame);

		if (previous_fps <= (input_fps / 10.0) or number_of_frames == input_frames or (number_of_frames % rounded_fps) == 0)
		{
			const double percentage_done		= static_cast<double>(number_of_frames) / input_frames;
			const double percentage_remaining	= 1.0 - percentage_done;
			const double milliseconds_elapsed	= std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() -start_time).count();
			const double milliseconds_remaining	= milliseconds_elapsed * percentage_remaining / percentage_done;
			const double seconds_remaining		= milliseconds_remaining / 1000.0;
			const double fps					= static_cast<double>(number_of_frames) / (milliseconds_elapsed / 1000.0);
			previous_fps						= fps;

			std::stringstream ss1;
			ss1 << " @ " << std::fixed << std::setprecision(1) << fps << " FPS";

			std::stringstream ss2;
			if (seconds_remaining >= 7200.0)
			{
				ss2 << ", done in " << int(std::round(seconds_remaining / 3600.0)) << " hours ";
			}
			else if (seconds_remaining >= 120)
			{
				ss2 << ", done in " << int(std::round(seconds_remaining / 60.0)) << " minutes ";
			}
			else if (seconds_remaining > 1.5)
			{
				ss2 << ", done in " << int(std::round(seconds_remaining)) << " seconds ";
			}
			else
			{
				ss2 << "                     "; // must be long enough to overwrite the "done in X seconds" message
			}

			std::cout << "\rprocessing frame " << number_of_frames << "/" << input_frames << " (" << std::round(100.0 * percentage_done) << "%" << ss1.str() << ")" << ss2.str() << std::flush;

			if (show_video)
			{
				cv::imshow("DarkHelp", frame);
				cv::waitKey(1);
			}
		}
	}
	std::cout << std::endl;

	const auto milliseconds_elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - start_time).count();

	options.json["file"][options.file_index]["frames"				] = number_of_frames;
	options.json["file"][options.file_index]["milliseconds_elapsed"	] = milliseconds_elapsed;
	options.json["file"][options.file_index]["average_fps"			] = number_of_frames / (milliseconds_elapsed / 1000.0);
	options.json["file"][options.file_index]["tiles"]["horizontal"	] = options.nn.horizontal_tiles;
	options.json["file"][options.file_index]["tiles"]["vertical"	] = options.nn.vertical_tiles;
	options.json["file"][options.file_index]["tiles"]["width"		] = options.nn.tile_size.width;
	options.json["file"][options.file_index]["tiles"]["height"		] = options.nn.tile_size.height;
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
			input_image = DarkHelp::resize_keeping_aspect_ratio(input_image, options.size1);
		}
	}

	options.json["file"][options.file_index]["resized_width"	] = input_image.cols;
	options.json["file"][options.file_index]["resized_height"	] = input_image.rows;

	const auto results = options.nn.predict(input_image);

	std::cout	<< "-> prediction took " << options.nn.duration_string();
	if (options.nn.horizontal_tiles > 1 or options.nn.vertical_tiles > 1)
	{
		std::cout	<< " across " << (options.nn.horizontal_tiles * options.nn.vertical_tiles) << " tiles "
					<< "(" << options.nn.horizontal_tiles << "x" << options.nn.vertical_tiles << ")"
					<< " each measuring " << options.nn.tile_size.width << "x" << options.nn.tile_size.height;
	}
	std::cout	<< std::endl
				<< "-> " << results << std::endl;

	cv::Mat output_image;
	if (options.keep_annotated_images or options.use_json_output == false)
	{
		output_image = options.nn.annotate();

		if (options.size2_is_set)
		{
			std::cout << "-> resizing output image from " << output_image.cols << "x" << output_image.rows << " to " << options.size2.width << "x" << options.size2.height << std::endl;
			output_image = DarkHelp::resize_keeping_aspect_ratio(output_image, options.size2);
		}

		if (options.keep_annotated_images)
		{
			// save the annotated image to disk

			std::string basename = options.filename;
			size_t pos = basename.find_last_of("/\\");
			if (pos != std::string::npos)
			{
				basename.erase(0, pos + 1);
			}
			pos = basename.rfind(".");
			if (pos != std::string::npos)
			{
				basename.erase(pos);
			}

			auto output_filename = std::filesystem::path(options.out_dir);

			if (options.image_type == "png")
			{
				output_filename /= (basename + ".png");
				cv::imwrite(output_filename.string(), output_image, {cv::IMWRITE_PNG_COMPRESSION, 1});
			}
			else
			{
				output_filename /= (basename + ".jpg");
				cv::imwrite(output_filename.string(), output_image, {cv::IMWRITE_JPEG_QUALITY, 75});
			}
			std::cout << "-> annotated image saved to \"" << output_filename.string() << "\"" << std::endl;
			options.json["file"][options.file_index]["annotated_image"] = output_filename.string();
		}
	}

	if (options.use_json_output)
	{
		const auto now				= std::chrono::high_resolution_clock::now();
		const auto epoch			= now.time_since_epoch();
		const auto nanoseconds		= std::chrono::duration_cast<std::chrono::nanoseconds>	(epoch).count();
		const std::time_t seconds	= std::chrono::duration_cast<std::chrono::seconds>		(epoch).count();
		const auto lt				= std::localtime(&seconds);
		char buffer[50];
		std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S %z", lt);

		options.json["file"][options.file_index]["timestamp"]["nanoseconds"	] = nanoseconds;
		options.json["file"][options.file_index]["timestamp"]["epoch"		] = seconds;
		options.json["file"][options.file_index]["timestamp"]["text"		] = buffer;

		options.json["file"][options.file_index]["count"				] = results.size();
		options.json["file"][options.file_index]["duration"				] = options.nn.duration_string();
		options.json["file"][options.file_index]["tiles"]["horizontal"	] = options.nn.horizontal_tiles;
		options.json["file"][options.file_index]["tiles"]["vertical"	] = options.nn.vertical_tiles;
		options.json["file"][options.file_index]["tiles"]["width"		] = options.nn.tile_size.width;
		options.json["file"][options.file_index]["tiles"]["height"		] = options.nn.tile_size.height;

		for (size_t idx = 0; idx < results.size(); idx ++)
		{
			const auto & pred = results[idx];

			auto & j = options.json["file"][options.file_index]["prediction"][idx];

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
				j["all_probabilities"][prop_count]["name"			] = options.nn.names[prop.first];
				prop_count ++;
			}
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

	display_current_msg(options, output_image, delay_in_milliseconds);

	if (options.in_slideshow)
	{
		// slideshow delay overrides whatever delay we may have put due to a message
		delay_in_milliseconds = options.wait_time_in_milliseconds_for_slideshow;
	}

	cv::namedWindow("DarkHelp", cv::WINDOW_NORMAL | cv::WINDOW_KEEPRATIO | cv::WINDOW_GUI_EXPANDED);
	cv::setWindowTitle("DarkHelp", options.neural_network_name + " - " + std::to_string(options.file_index + 1) + "/" + std::to_string(options.all_files.size()) + " - " + short_filename);
	cv::imshow("DarkHelp", output_image);

	const int key = cv::waitKeyEx(delay_in_milliseconds);

	/* I have no idea how these keys work.  This was working for me for years, and recently (2022) no longer
	 * works because...?  OpenCV now returns different values than what I expected.  Some of the higher bits
	 * are obviously used for flags of some sort, which I didn't use to see but now are set.  Will attempt to
	 * simplify the comparisons to see if I can get this working again on as many platforms as possible, but
	 * the complete lack of documentation for the return value of cv::waitKeyEx() does not help.
	 */
	const int key_simple	= (key & KEY_SIMPLE_MASK); // suspect this is only good for keys like A-Z, a-z, 0-9 but this should give us a start
	const int key_complex	= (key & KEY_COMPLEX_MASK); // non-alphabetical keys seem to use this, not sure what most of these bits represent

	if (key == -1 and options.in_slideshow == false)
	{
		// we didn't really timeout, this is simply an indication that a message needs to be erased
		return;
	}

	if (key_simple == KEY_ESC or key_simple == KEY_q)
	{
		options.done = true;
	}
	else if (key_simple == KEY_c)
	{
		options.nn.config.combine_tile_predictions = ! options.nn.config.combine_tile_predictions;
		set_msg(options, "combining tile predictions has been turned " + std::string(options.nn.config.combine_tile_predictions ? "on" : "off"));
	}
	else if (key_simple == KEY_d)
	{
		options.nn.config.snapping_enabled = not options.nn.config.snapping_enabled;
		set_msg(options, "snapping has been turned " + std::string(options.nn.config.snapping_enabled ? "on" : "off"));
	}
	else if (key_simple == KEY_g)
	{
		options.force_greyscale = not options.force_greyscale;
	}
	else if (key_simple == KEY_l)
	{
		if (options.nn.config.annotation_suppress_all_labels)
		{
			options.nn.config.annotation_suppress_all_labels = false;
			options.nn.config.annotation_auto_hide_labels = false;
			set_msg(options, "showing all labels");
		}
		else if (options.nn.config.annotation_auto_hide_labels == false)
		{
			options.nn.config.annotation_auto_hide_labels = true;
			set_msg(options, "auto-hide labels has been enabled");
		}
		else
		{
			options.nn.config.annotation_suppress_all_labels = true;
			set_msg(options, "suppressing all labels");
		}
	}
	else if (key_simple == KEY_s)
	{
		auto & shade = options.nn.config.annotation_shade_predictions;
		if		(shade < 0.25)	shade = 0.25;
		else if	(shade < 0.50)	shade = 0.50;
		else if	(shade < 0.75)	shade = 0.75;
		else					shade = 0.0;

		set_msg(options, "annotation shading has been set to " + std::to_string(int(shade * 100.0)) + "%");
	}
	else if (key_simple == KEY_t)
	{
		options.nn.config.enable_tiles = not options.nn.config.enable_tiles;
		set_msg(options, "image tiling has been turned " + std::string(options.nn.config.enable_tiles ? "on" : "off"));
	}
	else if (key_simple == KEY_u)
	{
		options.nn.config.annotation_include_duration = not options.nn.config.annotation_include_duration;
		set_msg(options, "duration has been turned " + std::string(options.nn.config.annotation_include_duration ? "on" : "off"));
	}
	else if (key_simple == KEY_w)
	{
		// save the file to disk, then re-load the same image
		auto output_filename = std::filesystem::path(options.out_dir);
		if (options.image_type == "png")
		{
			output_filename /= "output.png";
			cv::imwrite(output_filename.string(), output_image, {cv::IMWRITE_PNG_COMPRESSION, 1});
		}
		else
		{
			output_filename /= "output.jpg";
			cv::imwrite(output_filename.string(), output_image, {cv::IMWRITE_JPEG_QUALITY, 75});
		}
		std::cout << "-> output image saved to \"" << output_filename.string() << "\"" << std::endl;
		set_msg(options, "saved image to \"" + output_filename.string() + "\"");
	}
	else if (key_simple == KEY_h)
	{
		show_help_window();
	}
	else if (key_simple == KEY_p)
	{
		options.in_slideshow = not options.in_slideshow;
	}
	else if (key_complex == KEY_HOME)
	{
		options.in_slideshow = false;
		options.file_index = 0;
	}
	else if (key_complex == KEY_END)
	{
		options.in_slideshow = false;
		options.file_index = options.all_files.size() - 1;
	}
	else if (key_complex == KEY_LEFT)
	{
		options.in_slideshow = false;
		if (options.file_index > 0)
		{
			options.file_index --;
		}
	}
	else if (key_complex == KEY_UP)
	{
		// quicker slideshow
		options.wait_time_in_milliseconds_for_slideshow *= 0.5;
		if (options.wait_time_in_milliseconds_for_slideshow < 50)
		{
			options.wait_time_in_milliseconds_for_slideshow = 50;
		}
		std::cout << "-> slideshow timeout has been decreased to " << options.wait_time_in_milliseconds_for_slideshow << " milliseconds" << std::endl;
		set_msg(options, "slideshow timer: " + std::to_string(options.wait_time_in_milliseconds_for_slideshow) + " milliseconds");
		options.in_slideshow = true;
	}
	else if (key_complex == KEY_DOWN)
	{
		// slower slideshow
		options.wait_time_in_milliseconds_for_slideshow /= 0.5;
		std::cout << "-> slideshow timeout has been increased to " << options.wait_time_in_milliseconds_for_slideshow << " milliseconds" << std::endl;
		set_msg(options, "slideshow timer: " + std::to_string(options.wait_time_in_milliseconds_for_slideshow) + " milliseconds");
		options.in_slideshow = true;
	}
	else if (key_complex == KEY_PAGE_UP)
	{
		options.nn.config.threshold += 0.1;
		if (options.nn.config.threshold > 1.0)
		{
			options.nn.config.threshold = 1.0;
		}
		set_msg(options, "increased threshold: " + std::to_string((int)std::round(options.nn.config.threshold * 100.0)) + "%");
	}
	else if (key_complex == KEY_PAGE_DOWN)
	{
		options.nn.config.threshold -= 0.1;
		if (options.nn.config.threshold < 0.01)
		{
			options.nn.config.threshold = 0.001; // not a typo, allow the lower limit to be 0.1%
		}
		set_msg(options, "decreased threshold: " + std::to_string((int)std::round(options.nn.config.threshold * 100.0)) + "%");
	}
	else
	{
		std::cout << "KEY=0x" << std::hex << key << std::dec << std::endl;
		options.file_index ++;
	}

	return;
}


int main(int argc, char *argv[])
{
	try
	{
		// especially if we're in process of processing a .mp4 file,
		// it is nice to have some basic signal handling so the output
		// we do have gets written out correctly
		for (const int sig :
			{
				SIGINT	, // 2: CTRL+C
				SIGILL	, // 4: illegal instruction
				SIGABRT	, // 6: abort()
				SIGFPE	, // 8: floating point exception
				SIGSEGV	, // 11: segfault
				SIGTERM	, // 15: terminate
				#ifdef WIN32
				SIGBREAK, // Break is different than CTRL+C on Windows
				#else
				SIGHUP	, // 1: hangup
				SIGQUIT	, // 3: quit
				SIGUSR1	, // 10: user-defined
				SIGUSR2	, // 12: user-defined
				#endif
			})
		{
			std::signal(sig, cli_signal_handler);
		}

		Options options;

		init(options, argc, argv);

		set_msg(options, "press 'h' for help");
		options.file_index = 0;
		while (options.file_index < options.all_files.size() and not options.done and not signal_raised)
		{
			options.filename = options.all_files.at(options.file_index);

			const std::string mime_type = magic_file(options.magic_cookie, options.filename.c_str());
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

		magic_close(options.magic_cookie);
		options.magic_cookie = 0;

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

		if (options.keep_annotated_images or options.use_json_output)
		{
			std::ofstream ofs(options.out_dir + "/output.json");
			if (ofs.good())
			{
				ofs << options.json.dump(4) << std::endl;
			}
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
