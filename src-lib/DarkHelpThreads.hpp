/* DarkHelpOnThreads - C++ threads helper class for DarkHelp and Darknet.
 * Copyright 2024 Stephane Charette <stephanecharette@gmail.com>
 * MIT license applies.  See "license.txt" for details.
 */

#include "DarkHelp.hpp"

#include <atomic>
#include <condition_variable>
#include <filesystem>
#include <mutex>
#include <thread>


/** @file
 * %DarkHelp's class to load multiple identical neural networks.
 */

namespace DarkHelp
{
	/** This class allows you to easily run multiple identical copies of a neural network to process many files at once.
	 *
	 * If you have enough vram, or if you're running the CPU-only version of Darknet on a device with many cores, then you
	 * can use this class to load multiple copies of a neural network.  Each copy will run on a different thread, and can
	 * process images independently of each other.  This works especially well when the network is loaded once and re-used
	 * many times to process a large set of images.
	 *
	 * For example, on a RTX 3050 with 4 GiB of vram, a YOLOv4-tiny network measuring 768x576 takes 29.4 seconds to process
	 * 1540 image files.
	 *
	 * With this class, the same set of 1540 images only takes 8.8 seconds to process using 13 identical instances of the
	 * network loaded on the GPU.
	 *
	 * (Each instance of the network consumes 289 MiB of vram, which is why 13 copies can be loaded at once on a GPU with
	 * 4 GiB of vram.)
	 *
	 * Note this header file is not included by @p DarkHelp.hpp.  To use this functionality you'll need to explicitely
	 * include this header file.
	 */
	class DHThreads final
	{
		public:

			/// The key is the input image filename, while the value contains a copy of the prediction results.
			using ResultsMap = std::map<std::string, DarkHelp::PredictionResults>;

			/// Constructor.  No worker threads are started with this constructor.  You'll need to manually call @ref init().
			DHThreads();

			/** Constructor.
			 *
			 * @param [in] c The %DarkHelp configuration that each thread should use to load the neural network.
			 * @param [in] workers The number of worker threads to create.  Each of these threads will also load a copy of the
			 * neural network, so the number you use here should depend on the amount of vram available and the neural network
			 * dimensions.
			 * @param [in] output_directory The directory where the output files will be saved when @ref annotate_output_images
			 * is enabled.  If the directory does not exist, then it will be created.  If it already exists, then it is left
			 * as-is, meaning existing files will remain.
			 *
			 * The @p %DHThreads constructor will automatically call @ref init() to ensure all the threads and the neural networks
			 * are running.
			 *
			 * Call @ref add_images() to get the threads to start processing images.  Then call @ref wait_for_results() or
			 * @ref get_results().
			 */
			DHThreads(const DarkHelp::Config & c, const size_t workers, const std::filesystem::path output_directory = ".");

			/// Destructor.
			~DHThreads();

			/// Mostly for use with the default constructor.  Parameters are the same as the other constructor.
			DHThreads & init(const DarkHelp::Config & c, const size_t workers, const std::filesystem::path output_directory = ".");

			/** Starts all of the processing threads.  This is automatically called by @ref init(), but may also be called
			 * manually if @ref stop() was called.  Calling @p restart() when the threads are already running will cause the
			 * existing threads to stop, input files to be cleared, and existing results to be reset.  @see @ref init()
			 */
			DHThreads & restart();

			/** Causes the threads to stop processing and exit.  All results and input files are cleared.  Does not return until
			 * all threads have joined.  Worker threads can be restarted by calling either @ref init() or @ref restart().
			 * @see @ref purge()
			 */
			DHThreads & stop();

			/** Can be used to add a single image, or a subdirectory.  If a subdirectory, then recurse looking for all images.
			 * Call this as many times as necessary until all images have been added.  Image processing by the worker threads
			 * will start immediately.  Additional images can be added at any time, even while the worker threads have already
			 * started processing the first set of images.
			 */
			DHThreads & add_images(const std::filesystem::path & dir);

			/** Removes all input files, waits for all worker threads to finish processing, and clears out any results.
			 * Unlike the call to @ref stop() this does not terminate the worker threads or unload the neural networks,
			 * so you can again call @ref add_images() immediately afterwards without needing to @ref restart().
			 */
			DHThreads & purge();

			/** Get the number of files that have not yet been processed.  This is the number of unprocessed files plus the
			 * files which are in the middle of being processed.
			 */
			size_t files_remaining() const
			{
				return input_files.size() + files_processing;
			}

			/// Get the number of worker threads which have loaded a copy of the neural network.
			size_t networks_loaded() const
			{
				return threads_ready;
			}

			/** Returns when there are zero input files remaining and all worker threads have finished processing images.
			 * Note this will clear out the results since it internally calls @ref get_results().
			 *
			 * The difference between @ref wait_for_results() and @ref get_results() is that @p wait_for_results() will
			 * wait until @em all the results are available, while @p get_results() will immediately return with whatever
			 * results are available at this point in time.
			 */
			ResultsMap wait_for_results();

			/** Gain access to the neural network for the given worker thread.  This will return @p nullptr if the given worker
			 * thread has not loaded a neural network.
			 */
			DarkHelp::NN * get_nn(const size_t idx);

			/** Get all of the available prediction results.  You may call this as often you like to get intermediate results
			 * (if any are available), or you can indirectly call this via @ref wait_for_results().  Note that once the results
			 * are read and returned, the structure in which the results are stored internally is cleared.
			 *
			 * The difference between @ref wait_for_results() and @ref get_results() is that @p wait_for_results() will
			 * wait until @em all the results are available, while @p get_results() will immediately return with whatever
			 * results are available at this point in time.
			 */
			ResultsMap get_results();

			/** A copy of the configuration to use when instantiating each of the DarkHelp objects.  This is only referenced by
			 * @ref restart().  Meaning if you change @p cfg after the @ref DHThreads() consructor or @ref init(), you'll need to
			 * call @ref stop() and @ref restart().
			 */
			DarkHelp::Config cfg;

			/// Determines whether the input images are deleted once they are processed.  Default value is @p false, meaning files are not deleted.
			std::atomic<bool> detele_input_file_after_processing;

			/// Determines if annotated output images are created.  Default value is @p false.
			std::atomic<bool> annotate_output_images;

		private:

			/// The method that each worker thread runs to process images.  @see @ref restart()
			void run(const size_t id);

			/// If the threads need to stop, set this variable to @p true.  @see @ref stop()
			std::atomic<bool> stop_requested;

			/// The number of threads to start.
			size_t worker_threads_to_start;

			/// The directory where the output will be saved.
			std::filesystem::path output_dir;

			/// The threads that were started.  @see @ref worker_threads_to_start
			std::vector<std::thread> threads;

			/// Address to the worker thread neural networks.  @see @ref get_nn()
			std::vector<DarkHelp::NN *> networks;

			/// @{ Used to signal the worker threads when more work becomes available.
			std::condition_variable trigger;
			std::mutex trigger_lock;
			/// @}

			/// @{ Used to keep track of all the input files remaining to be processed.  @see @ref add_images()
			std::deque<std::string> input_files;
			std::mutex input_files_lock;
			/// @}

			/// @{ The prediction results for all the image file which have been processed.  @see @ref get_results()
			ResultsMap all_results;
			std::mutex results_lock;
			/// @}

			/// Track the number of worker threads which have loaded the neural network.
			std::atomic<size_t> threads_ready;

			/// The number of worker threads which are currently processing an image.
			std::atomic<size_t> files_processing;
	};
}
