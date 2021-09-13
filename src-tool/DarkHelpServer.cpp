/* DarkHelp - C++ helper class for Darknet's C API.
 * Copyright 2019-2021 Stephane Charette <stephanecharette@gmail.com>
 * MIT license applies.  See "license.txt" for details.
 */

#include <DarkHelp.hpp>
#include <thread>

#include "json.hpp"
#include "filesystem.hpp"

std::vector<std::string> messages;


nlohmann::json create_darkhelp_defaults()
{
	nlohmann::json j;

	j["darkhelp"]["lib"]["network"]["cfg"											] = "example.cfg";
	j["darkhelp"]["lib"]["network"]["names"											] = "example.names";
	j["darkhelp"]["lib"]["network"]["weights"										] = "example_best.weights";

	j["darkhelp"]["lib"]["settings"]["general"]["threshold"							] = 0.5;
	j["darkhelp"]["lib"]["settings"]["general"]["non_maximal_suppression_threshold"	] = 0.45,
	j["darkhelp"]["lib"]["settings"]["general"]["modify_batch_and_subdivisions"		] = true;
	j["darkhelp"]["lib"]["settings"]["general"]["names_include_percentage"			] = true;
	j["darkhelp"]["lib"]["settings"]["general"]["fix_out_of_bound_values"			] = true;
	j["darkhelp"]["lib"]["settings"]["general"]["sort_predictions"					] = 0;

	j["darkhelp"]["lib"]["settings"]["annotation"]["auto_hide_labels"				] = true;
	j["darkhelp"]["lib"]["settings"]["annotation"]["shade_predictions"				] = 0.25;
	j["darkhelp"]["lib"]["settings"]["annotation"]["include_all_names"				] = true;
	j["darkhelp"]["lib"]["settings"]["annotation"]["font_scale"						] = 0.5;
	j["darkhelp"]["lib"]["settings"]["annotation"]["font_thickness"					] = 1;
	j["darkhelp"]["lib"]["settings"]["annotation"]["include_duration"				] = true;
	j["darkhelp"]["lib"]["settings"]["annotation"]["include_timestamp"				] = false;

	j["darkhelp"]["lib"]["settings"]["tiling"]["enable_tiles"						] = false;
	j["darkhelp"]["lib"]["settings"]["tiling"]["combine_tile_predictions"			] = true;
	j["darkhelp"]["lib"]["settings"]["tiling"]["only_combine_similar_predictions"	] = true;
	j["darkhelp"]["lib"]["settings"]["tiling"]["tile_edge_factor"					] = 0.25;
	j["darkhelp"]["lib"]["settings"]["tiling"]["tile_rect_factor"					] = 1.2;

	auto tmp = std::filesystem::temp_directory_path();
	auto input_dir		= tmp / "darkhelpserver" / "input";
	auto output_dir		= tmp / "darkhelpserver" / "output";

	j["darkhelp"]["server"]["settings"]["input_directory"							] = input_dir.string();
	j["darkhelp"]["server"]["settings"]["output_directory"							] = output_dir.string();
	j["darkhelp"]["server"]["settings"]["clear_output_directory_on_startup"			] = true;
	j["darkhelp"]["server"]["settings"]["save_annotated_image"						] = false;
	j["darkhelp"]["server"]["settings"]["save_txt_annotations"						] = false;
	j["darkhelp"]["server"]["settings"]["save_json_results"							] = true;
	j["darkhelp"]["server"]["settings"]["crop_and_save_detected_objects"			] = false;
	j["darkhelp"]["server"]["settings"]["exit_if_idle"								] = false;
	j["darkhelp"]["server"]["settings"]["idle_time_in_seconds"						] = 60;
	j["darkhelp"]["server"]["settings"]["max_images_to_process_at_once"				] = 10;
	j["darkhelp"]["server"]["settings"]["run_cmd_after_processing_images"			] = "";
	j["darkhelp"]["server"]["settings"]["purge_files_after_cmd_completes"			] = true;

	return j;
}


void configure(DarkHelp & dh, const nlohmann::json & j)
{
	// this one needs to be set prior to loading the network
	dh.modify_batch_and_subdivisions = j["darkhelp"]["lib"]["settings"]["general"]["modify_batch_and_subdivisions"];

	dh.init(
		j["darkhelp"]["lib"]["network"]["cfg"		],
		j["darkhelp"]["lib"]["network"]["weights"	],
		j["darkhelp"]["lib"]["network"]["names"		]);

	std::cout
		<< "-> using DarkHelp v"		<< dh.version()			<< std::endl
		<< "-> network loaded in "		<< dh.duration_string()	<< std::endl
		<< "-> network dimensions: "	<< dh.network_size()	<< std::endl
		<< "-> number of classes: "		<< dh.names.size()		<< std::endl;

	for (size_t idx = 0; idx < dh.names.size(); idx ++)
	{
		std::cout << "   " << idx << " = " << dh.names.at(idx) << std::endl;
	}

	dh.threshold							= j["darkhelp"]["lib"]["settings"]["general"]["threshold"];
	dh.non_maximal_suppression_threshold	= j["darkhelp"]["lib"]["settings"]["general"]["non_maximal_suppression_threshold"	];
	dh.names_include_percentage				= j["darkhelp"]["lib"]["settings"]["general"]["names_include_percentage"			];
	dh.fix_out_of_bound_values				= j["darkhelp"]["lib"]["settings"]["general"]["fix_out_of_bound_values"				];
	dh.sort_predictions						= j["darkhelp"]["lib"]["settings"]["general"]["sort_predictions"					];
	dh.annotation_auto_hide_labels			= j["darkhelp"]["lib"]["settings"]["annotation"]["auto_hide_labels"					];
	dh.annotation_shade_predictions			= j["darkhelp"]["lib"]["settings"]["annotation"]["shade_predictions"				];
	dh.include_all_names					= j["darkhelp"]["lib"]["settings"]["annotation"]["include_all_names"				];
	dh.annotation_font_face					= j["darkhelp"]["lib"]["settings"]["annotation"]["font_scale"						];
	dh.annotation_font_thickness			= j["darkhelp"]["lib"]["settings"]["annotation"]["font_thickness"					];
	dh.annotation_include_duration			= j["darkhelp"]["lib"]["settings"]["annotation"]["include_duration"					];
	dh.annotation_include_timestamp			= j["darkhelp"]["lib"]["settings"]["annotation"]["include_timestamp"				];
	dh.enable_tiles							= j["darkhelp"]["lib"]["settings"]["tiling"]["enable_tiles"							];
	dh.combine_tile_predictions				= j["darkhelp"]["lib"]["settings"]["tiling"]["combine_tile_predictions"				];
	dh.only_combine_similar_predictions		= j["darkhelp"]["lib"]["settings"]["tiling"]["only_combine_similar_predictions"		];
	dh.tile_edge_factor						= j["darkhelp"]["lib"]["settings"]["tiling"]["tile_edge_factor"						];
	dh.tile_rect_factor						= j["darkhelp"]["lib"]["settings"]["tiling"]["tile_rect_factor"						];

	return;
}


nlohmann::json merge(const nlohmann::json & lhs, const nlohmann::json & rhs)
{
	nlohmann::json j = lhs;

	// if a key exists in both LHS and RHS, then we keep RHS

	for (auto it = rhs.cbegin(); it != rhs.cend(); ++it)
	{
		const auto & key = it.key();
		if (it->is_object())
		{
			if (lhs.contains(key))
			{
				j[key] = merge(lhs[key], rhs[key]);
			}
			else
			{
				messages.push_back("WARNING: The object \"" + key + "\" seems to be unknown: " + rhs.dump());
				j[key] = rhs[key];
			}
		}
		else
		{
			if (j.contains(key) == false)
			{
				messages.push_back("WARNING: The key \"" + key + "\" seems to be unknown: " + rhs.dump());
			}
			j[key] = it.value();
		}
	}

	return j;
}


void server(DarkHelp & dh, const nlohmann::json & j)
{
	const auto & server_settings = j["darkhelp"]["server"]["settings"];

	auto input_dir		= std::filesystem::path(server_settings["input_directory"]);
	auto output_dir		= std::filesystem::path(server_settings["output_directory"]);

	if (server_settings["clear_output_directory_on_startup"])
	{
		std::filesystem::remove_all(output_dir);
	}

	std::filesystem::create_directories(input_dir);
	std::filesystem::create_directories(output_dir);

	std::cout << "-> DarkHelp Server is now running..." << std::endl;

	size_t total_number_of_images_processed = 0;
	auto last_activity = std::chrono::system_clock::now();
	const auto idle_timeout_in_seconds = std::chrono::seconds(server_settings["idle_time_in_seconds"]);
	const bool exit_if_idle						= server_settings["exit_if_idle"					];
	const bool crop_and_save_detected_objects	= server_settings["crop_and_save_detected_objects"	];
	const bool save_annotated_image				= server_settings["save_annotated_image"			];
	const bool save_txt_annotations				= server_settings["save_txt_annotations"			];
	const bool save_json_results				= server_settings["save_json_results"				];
	const int max_images_to_process_at_once		= server_settings["max_images_to_process_at_once"	];
	const bool purge_files_after_cmd_completes	= server_settings["purge_files_after_cmd_completes"	];
	const std::string run_cmd_after_processing_images = server_settings["run_cmd_after_processing_images"];

	while (true)
	{
		const auto now = std::chrono::system_clock::now();

		if (exit_if_idle and now > last_activity + idle_timeout_in_seconds)
		{
			std::cout << "-> idle timeout detected after " << idle_timeout_in_seconds.count() << " seconds" << std::endl;
			break;
		}

		size_t number_of_images_processed = 0;

		std::filesystem::directory_iterator dir_iter(
			input_dir														,
			std::filesystem::directory_options::follow_directory_symlink	|
			std::filesystem::directory_options::skip_permission_denied		);

		for (const auto & entry : dir_iter)
		{
			total_number_of_images_processed ++;
			number_of_images_processed ++;
			last_activity = now;

			auto src = entry.path();
			auto dst = output_dir / src.filename();
			std::cout << "-> [" << total_number_of_images_processed << "] " << src.string() << std::endl;

			const auto results = dh.predict(entry.path().string());

			if (save_annotated_image)
			{
				auto fn = output_dir / (dst.stem().string() + "_annotated.jpg");
				cv::imwrite(fn.string(), dh.annotate(), {cv::ImwriteFlags::IMWRITE_JPEG_QUALITY, 70});
			}

			if (save_txt_annotations)
			{
				auto txt = dst;
				txt.replace_extension(".txt");
				std::ofstream ofs(txt.string());
				ofs << std::fixed << std::setprecision(10);
				for (const auto & prediction : results)
				{
					ofs	<< prediction.best_class			<< " "
						<< prediction.original_point.x		<< " "
						<< prediction.original_point.y		<< " "
						<< prediction.original_size.width	<< " "
						<< prediction.original_size.height	<< std::endl;
				}
			}

			if (save_json_results)
			{
				auto txt = dst;
				txt.replace_extension(".json");
				std::ofstream ofs(txt.string());

				nlohmann::json output;
				output["index"				] = total_number_of_images_processed;
				output["duration"			] = dh.duration_string();
				output["tiles"]["horizontal"] = dh.horizontal_tiles;
				output["tiles"]["vertical"	] = dh.vertical_tiles;
				output["tiles"]["width"		] = dh.tile_size.width;
				output["tiles"]["height"	] = dh.tile_size.height;

				size_t count = 0;
				for (const auto & pred : results)
				{
					auto & j = output["prediction"][count];

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
						j["all_probabilities"][prop_count]["name"			] = dh.names[prop.first];
						prop_count ++;
					}
					count ++;
				}

				ofs << output.dump(4) << std::endl;
			}

			if (crop_and_save_detected_objects)
			{
				for (size_t idx = 0; idx < results.size(); idx ++)
				{
					const auto & prediction = results[idx];
					auto fn = output_dir / (dst.stem().string() + "_idx_" + std::to_string(idx) + "_class_" + std::to_string(prediction.best_class) + ".jpg");
					cv::imwrite(fn.string(), dh.original_image(prediction.rect), {cv::ImwriteFlags::IMWRITE_JPEG_QUALITY, 75});
				}
			}

			std::filesystem::rename(src, dst);

			if (max_images_to_process_at_once > 0)
			{
				if (static_cast<int>(number_of_images_processed) >= max_images_to_process_at_once)
				{
					break;
				}
			}
		}

		if (number_of_images_processed == 0)
		{
			std::this_thread::sleep_for(std::chrono::seconds(1));
		}
		else
		{
			if (run_cmd_after_processing_images.empty() == false)
			{
				std::cout << "-> calling script after processing new images: " << number_of_images_processed << std::endl;
				system(run_cmd_after_processing_images.c_str());
				if (purge_files_after_cmd_completes)
				{
					std::filesystem::remove_all(output_dir);
					std::filesystem::create_directories(output_dir);
				}
			}
		}
	}

	return;
}


int main(int argc, char *argv[])
{
	std::cout << "DarkHelp Server v" << DH_VERSION << std::endl << std::endl;
	int rc = 1;

	try
	{
		const auto default_settings = create_darkhelp_defaults();

		if (argc != 2)
		{
			std::cout
				<< "ERROR: Invalid parameters."							<< std::endl
				<< ""													<< std::endl
				<< "Usage:"												<< std::endl
				<< ""													<< std::endl
				<< "\t" << argv[0] << " <json-config-filename>"			<< std::endl
				<< ""													<< std::endl
				<< "Here is a default json config file you can modify:"	<< std::endl
				<< ""													<< std::endl
				<< default_settings.dump(4)								<< std::endl;
		}
		else
		{
			nlohmann::json user_settings;

			if (true)
			{
				std::cout << "-> reading DarkHelp Server settings from \"" << argv[1] << "\"..." << std::endl;
				std::ifstream ifs(argv[1]);
				user_settings = nlohmann::json::parse(ifs);
			}

			auto settings = merge(default_settings, user_settings);

			std::cout << settings.dump(4) << std::endl;
			for (const auto & msg : messages)
			{
				std::cout << msg << std::endl;
			}
			if (messages.empty() == false)
			{
				throw std::invalid_argument(messages[0]);
			}

			DarkHelp dh;
			configure(dh, settings);
			server(dh, settings);

			rc = 0;
		}
	}
	catch (const std::exception & e)
	{
		std::cout << std::endl << "Exception thrown:" << std::endl << e.what() << std::endl;
		rc = 2;
	}

	std::cout << "-> DarkHelp Server is exiting with rc=" << rc << std::endl;

	return rc;
}
