/* DarkHelp - C++ helper class for Darknet's C API.
 * Copyright 2019-2024 Stephane Charette <stephanecharette@gmail.com>
 * MIT license applies.  See "license.txt" for details.
 */

#include <filesystem>
#include <map>
#include <set>
#include <sstream>
#include <string>

#include <opencv2/opencv.hpp>


/** @file
 * This simple tool looks for classes named "TL", "TR", "BL", and "BR".  This is typically used to indicate the
 * corners of objects, where "TL" is "top-left", "BR is "bottom-right", etc.  If it finds any of these classes, the
 * tool will then read through all of the existing annotations, resize the corners to a specific size, and re-write
 * the annotation files with the new sizes.
 */


/// Size to use for corner rectangles.
const int corner_size = 16;


/// Annotation indexes, such as "tl" -> 0, "tr" -> 1, ...
std::map<std::string, int> indexes;


/// Annotation indexes, such as 0 -> "tl", 1 -> "tr", ...
std::map<int, std::string> corners;


/// These are all of the images with annotations that we need to process.  @see @ref find_all_images()
std::vector<std::string> annotated_image_filenames;


std::string lowercase(const std::string & raw)
{
	std::string str = raw;

	std::transform(str.begin(), str.end(), str.begin(),
			[](unsigned char c)
			{
				return std::tolower(c);
			});

	return str;
}


/** Read the .names file and find all of the corner indexes we need to resize.  These classes will have names such as
 * @p TL, @p TR, @p BR, and @p BL.  The search for names is case-insensitive.
 */
void parse_names_file(const std::filesystem::path & names_file)
{
	std::cout << "Input .names file .. " << names_file.string() << std::endl;

	std::ifstream ifs(names_file);
	std::string line;

	int idx = 0;
	while (std::getline(ifs, line))
	{
		line = lowercase(line);

		if (line.find("tl") == 0 or
			line.find("tr") == 0 or
			line.find("br") == 0 or
			line.find("bl") == 0)
		{
			// this annotation seems to be a corner we need to resize
			indexes[line] = idx;
			corners[idx] = line;
			std::cout << "-> #" << idx << " = " << line << std::endl;
		}

		idx ++;
	}

	/* We typically have either 2 classes such as TL and TR, or we have the full 4 classes including BR and BL.
	 * This is not a hard requirement, but I've not yet run into a case where we have just 1 or 3.  If this
	 * situation ever comes up then we can get rid of this check.
	 */
	if (indexes.size() != 2 and indexes.size() != 4)
	{
		throw std::logic_error("expected either 2 or 4 corner type indexes, but found " + std::to_string(indexes.size()));
	}

	return;
}


/** Perform a recursive directory search to find all the images.  Then we exclude anything in DarkMark's image cache
 * or which doesn't have an annotation file.  Results are stored in the global variable @ref annotated_image_filenames.
 */
void find_all_images(const std::filesystem::path & root_directory)
{
	std::cout << "Search directory ... " << root_directory.string() << std::endl;

	std::vector<std::string> all_images;

	for (const auto & entry : std::filesystem::recursive_directory_iterator(root_directory))
	{
		if (entry.path().string().find("darkmark_image_cache") != std::string::npos)
		{
			continue;
		}

		const auto ext = lowercase(entry.path().extension().string());

		// might need to expand the set of extensions we look for (only need lowercase)
		if (ext == ".png" or
			ext == ".jpg" or
			ext == ".jpeg")
		{
			all_images.push_back(entry.path().string());
		}
	}

	// only keep the images that have annotations
	annotated_image_filenames.clear();
	size_t negative_samples = 0;
	for (const auto & fn : all_images)
	{
		const auto annotation_filename = std::filesystem::path(fn).replace_extension(".txt");
		if (std::filesystem::exists(annotation_filename))
		{
			if (std::filesystem::file_size(annotation_filename) > 0)
			{
				annotated_image_filenames.push_back(fn);
			}
			else
			{
				negative_samples ++;
			}
		}
	}

	std::cout
		<< "Total images ....... " << all_images.size()					<< std::endl
		<< "Negative samples ... " << negative_samples					<< std::endl
		<< "Annotated images ... " << annotated_image_filenames.size()	<< std::endl;

	std::sort(annotated_image_filenames.begin(), annotated_image_filenames.end());

	return;
}


/// Loop through all of the images and resize the corner annotations.
void process_images()
{
	std::cout << "Resize corners to .. " << corner_size << " x " << corner_size << std::endl;

	size_t rewritten_files	= 0;
	size_t unmodified_files	= 0;

	const float images_to_process = annotated_image_filenames.size();
	float images_processed = 0.0f;

	// keep track of the corners that we end up modifying
	std::map<std::string, size_t> count_modified_corners;
	for (auto iter : corners)
	{
		count_modified_corners[iter.second] = 0;
	}

	for (const auto & fn : annotated_image_filenames)
	{
		std::cout
			<< "\rProcessing images .. "
			<< static_cast<int>(std::round(images_processed * 100.0f / images_to_process))
			<< "% " << std::flush;

		const auto annotation_filename = std::filesystem::path(fn).replace_extension(".txt");

		cv::Mat mat = cv::imread(fn);
		if (mat.empty())
		{
			throw std::runtime_error("failed to read the image " + fn);
		}
		const double width	= mat.cols;
		const double height	= mat.rows;

		std::stringstream ss;
		ss << std::fixed << std::setprecision(9);

		bool modified = false;
		std::ifstream ifs(annotation_filename);
		while (ifs.good())
		{
			int idx		= -1;
			double cx	= -1.0;
			double cy	= -1.0;
			double w	= -1.0;
			double h	= -1.0;

			ifs >> idx >> cx >> cy >> w >> h;
			if (not ifs.good())
			{
				break;
			}

			if (corners.count(idx)	and
				cx	> 0.0			and
				cy	> 0.0			and
				w	> 0.0			and
				h	> 0.0)
			{
				int im_x = std::round(width		* (cx - w / 2.0));
				int im_y = std::round(height	* (cy - h / 2.0));
				int im_w = std::round(width		* w);
				int im_h = std::round(height	* h);

				if (im_w != corner_size or im_h != corner_size)
				{
					if (corners[idx] == "tl")
					{
						// leave the X and Y coordinates unchanged
						im_w = corner_size;
						im_h = corner_size;
						count_modified_corners["tl"] ++;
					}
					else if (corners[idx] == "tr")
					{
						// move the X, leave the Y unchanged
						im_x += (im_w - corner_size);
						im_w = corner_size;
						im_h = corner_size;
						count_modified_corners["tr"] ++;
					}
					else if (corners[idx] == "br")
					{
						// move both X and Y
						im_x += (im_w - corner_size);
						im_y += (im_h - corner_size);
						im_w = corner_size;
						im_h = corner_size;
						count_modified_corners["br"] ++;
					}
					else if (corners[idx] == "bl")
					{
						// move the Y, leave the X unchanged
						im_y += (im_h - corner_size);
						im_w = corner_size;
						im_h = corner_size;
						count_modified_corners["bl"] ++;
					}
					else
					{
						throw std::logic_error("corner type \"" + corners[idx] + "\" is unknown");
					}

					// now that we know the new image coordinates, calculate the new normalized coordinates
					w	= im_w / width;
					h	= im_h / height;
					cx	= (im_x + (im_w / 2.0)) / width;
					cy	= (im_y + (im_h / 2.0)) / height;
					modified = true;
				}
			}

			ss << idx << " " << cx << " " << cy << " " << w << " " << h << std::endl;
		}
		ifs.close();

		if (not modified)
		{
			unmodified_files ++;
		}
		else
		{
			rewritten_files ++;
			std::ofstream ofs(annotation_filename);
			ofs << ss.str();

			// if this file also has a DarkMark .json file associated with it,
			// then it must be deleted to force DarkMark to re-import the .txt file
			const auto json_filename = std::filesystem::path(fn).replace_extension(".json");
			if (std::filesystem::exists(json_filename))
			{
				std::filesystem::remove(json_filename);
			}
		}

		images_processed ++;
	}

	std::cout
		<< ""											<< std::endl
		<< "Unmodified files ... " << unmodified_files	<< std::endl
		<< "Re-written files ... " << rewritten_files	<< std::endl;

	for (const auto & [key, val] : count_modified_corners)
	{
		std::cout << "-> " << key << ": " << val << std::endl;
	}

	return;
}


int main(int argc, char * argv[])
{
	int rc = 1;

	try
	{
		std::cout << "Resize Darknet/YOLO Corner Annotations (TL, TR, BL, BR)" << std::endl << std::endl;

		if (argc != 2)
		{
			std::cout
				<< "Usage:"													<< std::endl
				<< ""														<< std::endl
				<< "\t" << argv[0] << " <filename>"							<< std::endl
				<< ""														<< std::endl
				<< "Specify the .names file of the Darknet/YOLO project."	<< std::endl
				<< ""														<< std::endl
				<< "WARNING:"												<< std::endl
				<< ""														<< std::endl
				<< "This tool will re-write your annotations!  Make sure"	<< std::endl
				<< "you have a backup of your data before you run it."		<< std::endl
				<< ""														<< std::endl;

			throw std::invalid_argument("invalid parameter");
		}

		std::filesystem::path names_file(argv[1]);
		if (not std::filesystem::exists(names_file))
		{
			throw std::invalid_argument("file does not exist: " + names_file.string());
		}
		names_file = std::filesystem::canonical(names_file);
		if (not std::filesystem::is_regular_file(names_file))
		{
			throw std::invalid_argument("was expecting the .names file to be a regular file: " + names_file.string());
		}

		parse_names_file(names_file);

		std::filesystem::path root_directory = names_file.parent_path();
		find_all_images(root_directory);

		process_images();

		std::cout << "Done!" << std::endl;

		rc = 0;
	}
	catch (const std::exception & e)
	{
		std::cout << "ERROR: " << e.what() << std::endl;
		rc = 2;
	}

	return rc;
}
