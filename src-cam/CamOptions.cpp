/* DarkHelp - C++ helper class for Darknet's C API.
 * Copyright 2019-2024 Stephane Charette <stephanecharette@gmail.com>
 * MIT license applies.  See "license.txt" for details.
 */


#include "CamOptions.hpp"
#include <tclap/CmdLine.h>
#include <filesystem>


DarkHelp::CamOptions::CamOptions()
{
	reset();

	return;
}


DarkHelp::CamOptions::~CamOptions()
{
	return;
}


DarkHelp::CamOptions & DarkHelp::CamOptions::reset()
{
	device_filename	= "";
	device_index	= -1;
	device_backend	= cv::CAP_ANY;
	fps_request		= -1.0;
	fps_actual		= -1.0;
	size_request	= cv::Size(-1, -1);
	size_actual		= cv::Size(-1, -1);
	resize_before	= cv::Size(-1, -1);
	resize_after	= cv::Size(-1, -1);
	capture_seconds	= -1;

	return *this;
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


cv::Size get_WxH(const std::string & text)
{
	int w = -1;
	int h = -1;

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
				if (s.width > 0 and s.height > 0)
				{
					return true;
				}
			}
			catch (...) {}
			return false;
		}
};



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
				if (i >= 0 and idx == value.size())
				{
					// valid positive int
					return true;
				}
			}
			catch (...) {}
			return false;
		}
};


class FloatConstraint : public TCLAP::Constraint<std::string>
{
	public:

		virtual std::string description() const	{ return "positive float"; }
		virtual std::string shortID() const		{ return "positive float"; }
		virtual bool check(const std::string & value) const
		{
			// can this value be converted to a positive float?
			try
			{
				size_t idx = 0;
				const float f = std::stof(value, &idx);
				if (f >= 0.0f and idx == value.size())
				{
					// this is a valid float
					return true;
				}
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


class CameraConstraint : public TCLAP::Constraint<std::string>
{
	public:

		virtual std::string description() const	{ return "<device index>|<device filename>"; }
		virtual std::string shortID() const		{ return "<device index>|<device filename>"; }
		virtual bool check(const std::string & value) const
		{
			if (value.empty())
			{
				return false;
			}
			if (std::filesystem::exists(value) and not std::filesystem::is_directory(value))
			{
				return true;
			}
			if (value.find_first_not_of("0123456789") == std::string::npos)
			{
				return true;
			}
			return false;
		}
};


void DarkHelp::parse(CamOptions & cam_options, DarkHelp::Config & config, int argc, char * argv[])
{
	auto file_exist_constraint = FileExistConstraint();
	auto driver_constraint = DriverConstraint();
	auto float_constraint = FloatConstraint();
	auto int_constraint = IntConstraint();
	auto WxH_constraint = WxHConstraint();
	auto camera_constraint = CameraConstraint();

	std::vector<std::string> booleans = { "true", "false", "on", "off", "yes", "no", "t", "f", "y", "n", "1", "0" };
	auto allowed_booleans = TCLAP::ValuesConstraint<std::string>(booleans);

	TCLAP::CmdLine cli("Load a darknet neural network and process frames from a camera (webcam).", ' ', DH_VERSION);

	TCLAP::ValueArg<std::string> resize_after				("a", "after"					, "Resize the output image (\"after\") to \"WxH\", such as 640x480."										, false, ""			, &WxH_constraint		, cli);
	TCLAP::ValueArg<std::string> resize_before				("b", "before"					, "Resize the input image (\"before\") to \"WxH\", such as 640x480."										, false, ""			, &WxH_constraint		, cli);
	TCLAP::ValueArg<std::string> camera						("c", "camera"					, "Camera index or filename to use. Default is 0 (first webcam)."											, false, "0"		, &camera_constraint	, cli);
	TCLAP::ValueArg<std::string> duration					("d", "duration"				, "Determines if the duration is added to annotations."														, false, "true"		, &allowed_booleans		, cli);
	TCLAP::ValueArg<std::string> driver						("D", "driver"					, "Determines if Darknet or OpenCV DNN is used. Default is \"darknet\"."									, false, "darknet"	, &driver_constraint	, cli);
	TCLAP::ValueArg<std::string> shade						("e", "shade"					, "Amount of alpha-blending to use when shading in rectangles. Default is 0.25."							, false, "0.25"		, &float_constraint		, cli);
	TCLAP::ValueArg<std::string> fontscale					("f", "fontscale"				, "Determines how the font is scaled for annotations. Default is 0.5."										, false, "0.5"		, &float_constraint		, cli);
	TCLAP::ValueArg<std::string> height						("H", "height"					, "The camera height to use. Default is 480."																, false, "480"		, &int_constraint		, cli);
	TCLAP::ValueArg<std::string> timestamp					("i", "timestamp"				, "Determines if a timestamp is added to annotations."														, false, "false"	, &allowed_booleans		, cli);
	TCLAP::ValueArg<std::string> nms						("n", "nms"						, "The non-maximal suppression threshold to use when predicting. Default is 0.45."							, false, "0.45"		, &float_constraint		, cli);
	TCLAP::ValueArg<std::string> autohide					("o", "autohide"				, "Auto-hide labels."																						, false, "true"		, &allowed_booleans		, cli);
	TCLAP::ValueArg<std::string> percentage					("p", "percentage"				, "Determines if percentages are added to annotations."														, false, "true"		, &allowed_booleans		, cli);
	TCLAP::ValueArg<std::string> size						("s", "size"					, "The camera width and height to set. Combines \"w\" and \"h\" options. Default is 640x480."				, false, "640x480"	, &WxH_constraint		, cli);
	TCLAP::ValueArg<std::string> snapping					("S", "snapping"				, "Snap the annotations."																					, false, "false"	, &allowed_booleans		, cli);
	TCLAP::ValueArg<std::string> threshold					("t", "threshold"				, "The threshold to use when predicting with the neural net. Default is 0.5."								, false, "0.5"		, &float_constraint		, cli);
	TCLAP::ValueArg<std::string> use_tiles					("T", "tiles"					, "Determines if large images are processed by breaking into tiles. Default is \"false\"."					, false, "false"	, &allowed_booleans		, cli);
	TCLAP::ValueArg<std::string> width						("W", "width"					, "The camera width to use. Default is 640."																, false, "640"		, &int_constraint		, cli);

	TCLAP::ValueArg<std::string> capture_time				("", "capture-time"				, "Length of time (in seconds) to run before automatically exiting."										, false, ""			, &int_constraint		, cli);
	TCLAP::ValueArg<std::string> fps						("", "fps"						, "Frames-per-second."																						, false, ""			, &float_constraint		, cli);
	TCLAP::ValueArg<std::string> line_thickness				("", "line"						, "Thickness of annotation lines in pixels. Default is 2."													, false, "2"		, &int_constraint		, cli);
	TCLAP::ValueArg<std::string> show_gui					("", "gui"						, "Determines if the output is shown in a GUI window using OpenCV's HighGUI. Default is true."				, false, "true"		, &allowed_booleans		, cli);
	TCLAP::ValueArg<std::string> pixelate					("", "pixelate"					, "Determines if predictions are pixelated in the output annotation image. Default is false."				, false, "false"	, &allowed_booleans		, cli);
	TCLAP::ValueArg<std::string> redirection				("", "redirection"				, "Determines if STDOUT and STDERR redirection will be performed when Darknet loads. Default is false."		, false, "false"	, &allowed_booleans		, cli);
	TCLAP::ValueArg<std::string> snap_horizontal_tolerance	("", "snap-horizontal-tolerance", "Snap horizontal tolerance, in pixels. Only used when snapping is enabled. Default is 5."					, false, "5"		, &int_constraint		, cli);
	TCLAP::ValueArg<std::string> snap_vertical_tolerance	("", "snap-vertical-tolerance"	, "Snap vertical tolerance, in pixels. Only used when snapping is enabled. Default is 5."					, false, "5"		, &int_constraint		, cli);
	TCLAP::SwitchArg suppress								("", "suppress"					, "Suppress all labels (bounding boxes are shown, but not the labels at the top of each bounding box)."													, cli, false);
	TCLAP::ValueArg<std::string> tile_edge					("", "tile-edge"				, "How close objects must be to tile edges to be re-combined. Range is 0.01-1.0+. Default is 0.25."			, false, "0.25"		, &float_constraint		, cli);
	TCLAP::ValueArg<std::string> tile_rect					("", "tile-rect"				, "How similarly objects must line up across tiles to be re-combined. Range is 1.0-2.0+. Default is 1.20."	, false, "1.2"		, &float_constraint		, cli);

	TCLAP::UnlabeledValueArg<std::string> cfg				("config"						, "The darknet config filename, usually ends in \".cfg\"."													, true	, ""		, &file_exist_constraint, cli);
	TCLAP::UnlabeledValueArg<std::string> weights			("weights"						, "The darknet weights filename, usually ends in \".weights\"."												, true	, ""		, &file_exist_constraint, cli);
	TCLAP::UnlabeledValueArg<std::string> names				("names"						, "The darknet class names filename, usually ends in \".names\"."											, true	, ""		, &file_exist_constraint, cli);

	cli.parse(argc, argv);

	config.cfg_filename		= cfg		.getValue();
	config.names_filename	= names		.getValue();
	config.weights_filename	= weights	.getValue();

	DarkHelp::verify_cfg_and_weights(config.cfg_filename, config.weights_filename, config.names_filename);

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

	#ifndef HAVE_OPENCV_DNN_OBJDETECT
	// if we get here, then we don't have OpenCV DNN module so we *must* use Darknet
	if (darkhelp_driver != DarkHelp::EDriver::kDarknet)
	{
		std::cout << "OpenCV DNN module is not available.  Changing driver to \"darknet\"." << std::endl;
		darkhelp_driver = DarkHelp::EDriver::kDarknet;
	}
	#endif

	config.driver								= darkhelp_driver;
	config.threshold							= std::stof(threshold.getValue());
	config.non_maximal_suppression_threshold	= std::stof(nms.getValue());
	config.names_include_percentage				= get_bool(percentage);
	config.annotation_line_thickness			= std::stoi(line_thickness.getValue());
	config.annotation_font_scale				= std::stod(fontscale.getValue());
	config.annotation_include_duration			= get_bool(duration);
	config.annotation_include_timestamp			= get_bool(timestamp);
	config.annotation_shade_predictions			= std::stof(shade.getValue());
	config.annotation_auto_hide_labels			= get_bool(autohide);
	config.enable_tiles							= get_bool(use_tiles);
	config.tile_edge_factor						= std::stof(tile_edge.getValue());
	config.tile_rect_factor						= std::stof(tile_rect.getValue());
	config.snapping_enabled						= get_bool(snapping);
	config.snapping_horizontal_tolerance		= std::stoi(snap_horizontal_tolerance.getValue());
	config.snapping_vertical_tolerance			= std::stoi(snap_vertical_tolerance.getValue());
	config.annotation_pixelate_enabled			= get_bool(pixelate);
	config.redirect_darknet_output				= get_bool(redirection);
	config.annotation_suppress_all_labels		= suppress.getValue();

	#ifdef WIN32
	cam_options.device_backend = cv::CAP_ANY;
	#else
	cam_options.device_backend =
//		cv::CAP_ANY;
		cv::CAP_V4L2;
//		cv::CAP_GSTREAMER;
//		cv::CAP_FFMPEG;
	#endif

	const std::string cam = camera.getValue();
	if (cam.find_first_not_of("0123456789") == std::string::npos)
	{
		cam_options.device_index = std::stoi(cam);
	}
	else
	{
		cam_options.device_index = -1;
		cam_options.device_filename = cam;
	}

	if (resize_before.isSet())
	{
		cam_options.resize_before = get_WxH(resize_before);
	}
	if (resize_after.isSet())
	{
		cam_options.resize_after = get_WxH(resize_after);
	}
	if (fps.isSet())
	{
		cam_options.fps_request = std::stod(fps.getValue());
	}

	cam_options.size_request.width = std::stoi(width.getValue());
	cam_options.size_request.height = std::stoi(height.getValue());

	if (size.isSet())
	{
		cam_options.size_request = get_WxH(size);
	}

	if (capture_time.isSet())
	{
		cam_options.capture_seconds = std::stoi(capture_time.getValue());
	}

	cam_options.show_gui = get_bool(show_gui);
	if (cam_options.show_gui == false and cam_options.capture_seconds <= -1)
	{
		cam_options.capture_seconds = 60;
	}

	return;
}
