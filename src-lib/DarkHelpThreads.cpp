/* DarkHelpOnThreads - C++ threads helper class for DarkHelp and Darknet.
 * Copyright 2024 Stephane Charette <stephanecharette@gmail.com>
 * MIT license applies.  See "license.txt" for details.
 */

#include "DarkHelpThreads.hpp"


DarkHelp::DHThreads::DHThreads() :
	detele_input_file_after_processing(false),
	annotate_output_images(false),
	worker_threads_to_start(0),
	threads_ready(0),
	files_processing(0)
{
	return;
}


DarkHelp::DHThreads::DHThreads(const DarkHelp::Config & c, const size_t workers, const std::filesystem::path output_directory) :
		DHThreads()
{
	init(c, workers, output_directory);

	return;
}


DarkHelp::DHThreads::~DHThreads()
{
	stop();

	return;
}


DarkHelp::DHThreads & DarkHelp::DHThreads::init(const DarkHelp::Config & c, const size_t workers, const std::filesystem::path output_directory)
{
	stop();

	if (workers < 1 or workers > 32)
	{
		/* For safety reasons, an upper bound has been set on the number of workers threads to start.  If you have a really
		 * beefy system with an incredible amount of vram and ram, it is possible you might want more than this, in which
		 * case you'll have to edit the limit above.  But as of March 2024, it is unlikely you'd have more than 32 parallel
		 * copies of DarkHelp running at once.
		 */
		throw std::invalid_argument("number of worker threads seems to be unusual: " + std::to_string(workers));
	}

	std::filesystem::create_directories(output_directory);
	output_dir = std::filesystem::canonical(output_directory);

	cfg = c;
	worker_threads_to_start = workers;

	// if the .cfg file needs to be modified, make sure you do this just once before all the threads are started,
	// otherwise we may end up re-writing the .cfg file in the middle of another thread attempting to load the network
	if (cfg.modify_batch_and_subdivisions)
	{
		DarkHelp::verify_cfg_and_weights(cfg.cfg_filename, cfg.weights_filename, cfg.names_filename);
		const MStr m =
		{
			{"batch"		, "1"},
			{"subdivisions"	, "1"}
		};
		edit_cfg_file(cfg.cfg_filename, m);
	}

	// start all of the necessary threads
	restart();

	return *this;
}


DarkHelp::DHThreads & DarkHelp::DHThreads::restart()
{
	stop();

	stop_requested = false;
	threads.reserve(worker_threads_to_start);
	networks.reserve(worker_threads_to_start);
	for (size_t idx = 0; idx < worker_threads_to_start; idx ++)
	{
		networks.push_back(nullptr);
		threads.emplace_back(std::thread(&DHThreads::run, this, idx));
	}

	return *this;
}


DarkHelp::DHThreads & DarkHelp::DHThreads::stop()
{
	stop_requested = true;

	for (auto & t : threads)
	{
		trigger.notify_all();

		if (t.joinable())
		{
			t.join();
		}
	}

	threads				.clear();
	networks			.clear();
	input_files			.clear();
	all_results			.clear();
	threads_ready		= 0;
	files_processing	= 0;

	return *this;
}


DarkHelp::DHThreads & DarkHelp::DHThreads::add_images(const std::filesystem::path & dir)
{
	if (worker_threads_to_start < 1)
	{
		std::logic_error("DHThreads worker threads and neural networks have not yet been initialized");
	}

	const auto path = std::filesystem::canonical(dir);
//	std::cout << "adding " << path.string() << std::endl;

	if (std::filesystem::is_regular_file(path))
	{
		std::scoped_lock lock(input_files_lock);
		input_files.push_back(path.string());
		trigger.notify_all();
	}
	else if (std::filesystem::is_directory(path))
	{
		for (const auto & entry : std::filesystem::recursive_directory_iterator(path))
		{
			if (stop_requested)
			{
				break;
			}

			if (not entry.is_regular_file())
			{
				continue;
			}

			const auto ext = entry.path().extension().string();
			if (ext == ".jpeg"	or
				ext == ".jpg"	or
				ext == ".png"	or
				ext == ".JPEG"	or
				ext == ".JPG"	or
				ext == ".PNG"	)
			{
				std::scoped_lock lock(input_files_lock);
				input_files.push_back(entry.path().string());
				trigger.notify_all();
			}
		}
	}

	return *this;
}


DarkHelp::DHThreads & DarkHelp::DHThreads::purge()
{
	if (worker_threads_to_start < 1)
	{
		std::logic_error("DHThreads worker threads and neural networks have not yet been initialized");
	}

	if (not input_files.empty())
	{
		std::scoped_lock lock(input_files_lock);
		input_files.clear();
	}

	wait_for_results();

	return *this;
}


DarkHelp::DHThreads::ResultsMap DarkHelp::DHThreads::wait_for_results()
{
	if (worker_threads_to_start < 1)
	{
		std::logic_error("DHThreads worker threads and neural networks have not yet been initialized");
	}

	while (not stop_requested)
	{
		if (files_remaining() == 0)
		{
			break;
		}

		std::unique_lock lock(trigger_lock);
		trigger.wait_for(lock, std::chrono::seconds(2));
	}

	return get_results();
}


DarkHelp::NN * DarkHelp::DHThreads::get_nn(const size_t idx)
{
	if (idx < networks_loaded())
	{
		return networks[idx];
	}

	throw std::invalid_argument("index " + std::to_string(idx) + " is not valid");
}


DarkHelp::DHThreads::ResultsMap DarkHelp::DHThreads::get_results()
{
	ResultsMap results;

	std::scoped_lock lock(results_lock);
	results.swap(all_results);

	return results;
}


void DarkHelp::DHThreads::run(const size_t id)
{
	try
	{
		DarkHelp::NN nn(cfg);
		networks[id] = &nn;

		threads_ready ++;

		while (not stop_requested)
		{
			if (input_files.empty())
			{
				std::unique_lock lock(trigger_lock);
				trigger.wait_for(lock, std::chrono::seconds(2));
			}

			if (input_files.size() == 0)
			{
				continue;
			}

			std::string fn;
			if (not stop_requested)
			{
				// see if we can get a file to process from the input set
				std::scoped_lock lock(input_files_lock);

				if (not input_files.empty())
				{
					fn = input_files.front();
					input_files.pop_front();
					files_processing ++;

					if (not input_files.empty())
					{
						// let another thread know there are still some input files that remain
						trigger.notify_all();
					}
				}
			}

			if (not fn.empty())
			{
//				std::cout << id << ": processing " << fn << std::endl;
				const auto results = nn.predict(fn);
//				std::cout << id << ": " << results << std::endl;

				if (annotate_output_images)
				{
					const auto annotated_image_fn = output_dir / (std::filesystem::path(fn).stem().string() + ".jpg");
					cv::imwrite(annotated_image_fn, nn.annotate(), {cv::IMWRITE_JPEG_QUALITY, 75});
				}

				if (detele_input_file_after_processing)
				{
					std::filesystem::remove(fn);
				}

				if (true)
				{
					std::scoped_lock lock(results_lock);
					all_results[fn] = results;
				}

				files_processing --;

				// in case wait() has been called, we want to notify so it can return once all images are done
				trigger.notify_all();
			}
		}
	}
	catch (const std::exception & e)
	{
		std::cout << id << ": caught exception: " << e.what() << std::endl;
	}

	std::cout << id << ": ending thread" << std::endl;
	networks[id] = nullptr;
	threads_ready --;

	return;
}
