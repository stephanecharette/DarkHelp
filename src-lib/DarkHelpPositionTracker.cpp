/* DarkHelp - C++ helper class for Darknet's C API.
 * Copyright 2019-2024 Stephane Charette <stephanecharette@gmail.com>
 * MIT license applies.  See "license.txt" for details.
 */

#include "DarkHelpPositionTracker.hpp"


DarkHelp::PositionTracker::Obj & DarkHelp::PositionTracker::Obj::clear()
{
	oid				= 0;
	fids_and_rects	.clear();
	classes			.clear();

	return *this;
}


bool DarkHelp::PositionTracker::Obj::empty() const
{
	return oid == 0 or fids_and_rects.empty();
}


size_t DarkHelp::PositionTracker::Obj::first_seen_frame_id() const
{
	if (fids_and_rects.empty())
	{
		throw std::logic_error("cannot get the first frame ID since the tracking map for this object is empty");
	}

	return fids_and_rects.begin()->first;
}


size_t DarkHelp::PositionTracker::Obj::last_seen_frame_id() const
{
	if (fids_and_rects.empty())
	{
		throw std::logic_error("cannot get the last frame ID since the tracking map for this object is empty");
	}

	return fids_and_rects.rbegin()->first;
}


cv::Rect DarkHelp::PositionTracker::Obj::rect() const
{
	if (fids_and_rects.empty())
	{
		throw std::logic_error("cannot get the recentagle since the tracking map for this object is empty");
	}

	return fids_and_rects.rbegin()->second;
}


cv::Point DarkHelp::PositionTracker::Obj::center() const
{
	const auto r = rect();
	const cv::Point p(r.x + r.width / 2, r.y + r.height / 2);

	return p;
}


cv::Size DarkHelp::PositionTracker::Obj::size() const
{
	return rect().size();
}


DarkHelp::PositionTracker::PositionTracker() :
	most_recent_object_id(0),
	most_recent_frame_id(0),
	age_of_objects_before_deletion(10),
	maximum_number_of_frames_per_object(90),
	maximum_distance_to_consider(100.0)
{
	clear();

	return;
}


DarkHelp::PositionTracker::~PositionTracker()
{
	return;
}


DarkHelp::PositionTracker & DarkHelp::PositionTracker::clear()
{
	maximum_distance_to_consider		= 100.0;
	maximum_number_of_frames_per_object	= 90;
	age_of_objects_before_deletion		= 10;
	most_recent_object_id				= 0;
	most_recent_frame_id				= 0;

	objects.clear();

	return *this;
}


DarkHelp::PositionTracker & DarkHelp::PositionTracker::add(DarkHelp::PredictionResults & results)
{
	most_recent_frame_id ++;

	if (results.size() > 0)
	{
		process(most_recent_frame_id, results);
	}

	remove_old_objects();

	return *this;
}


const DarkHelp::PositionTracker::Obj & DarkHelp::PositionTracker::get(const size_t oid) const
{
	for (const auto & obj : objects)
	{
		if (obj.oid == oid)
		{
			return obj;
		}
	}

	throw std::invalid_argument("object #" + std::to_string(oid) + " not found");
}


DarkHelp::PositionTracker & DarkHelp::PositionTracker::process(const size_t frame_id, DarkHelp::PredictionResults & results)
{
	std::set<size_t> previous_oids_we_already_matched;

	for (auto & prediction : results)
	{
		Obj new_obj;
		new_obj.fids_and_rects[frame_id] = prediction.rect;

		for (auto iter = prediction.all_probabilities.begin(); iter != prediction.all_probabilities.end(); iter ++)
		{
			// where the key is the class, and val is the probability from 0.0 to 1.0
			const auto & key = iter->first;
			const auto & val = iter->second;

			if (val >= 0.2)
			{
				new_obj.classes.insert(key);
			}
		}

		// compare the centroid of every object we're tracking against this one to see if we have a match
		std::map<double, Objects::iterator> distances;

		for (auto iter = objects.begin(); iter != objects.end(); iter ++)
		{
			const auto & old_obj = *iter;
			const auto & old_oid = old_obj.oid;

			if (previous_oids_we_already_matched.count(old_oid))
			{
				// skip this one since we already found a match for it
				continue;
			}

			// remember the distance between the new object and this old one
			const auto distance = cv::norm(new_obj.center() - old_obj.center());
			distances[distance] = iter;
		}

		// start with the smallest distances and see if we can find a match
		for (auto iter = distances.begin(); iter != distances.end(); iter ++)
		{
			const auto distance = iter->first;
			if (distance > maximum_distance_to_consider)
			{
				// object is too far, stop looking for matches
				break;
			}

			auto & old_obj = *iter->second;
			if (old_obj.classes.count(prediction.best_class) == 0)
			{
				// object is not the correct class, keep looking
				continue;
			}

			new_obj.oid = old_obj.oid;
			old_obj.fids_and_rects[frame_id] = prediction.rect;
			old_obj.classes.insert(new_obj.classes.begin(), new_obj.classes.end());
//			std::cout << "near match: oid=" << new_obj.oid << " center=" << new_obj.center() << " distance=" << distance << std::endl;
			break;
		}

		// anything that remains without a OID is a new object
		if (new_obj.oid == 0)
		{
			new_obj.oid = ++ most_recent_object_id;
			objects.push_back(new_obj);
//			std::cout << "NEW OID: oid=" << new_obj.oid << " center=" << new_obj.center() << std::endl;
		}

		// update the OID in the DarkHelp prediction results
		prediction.object_id = new_obj.oid;
		previous_oids_we_already_matched.insert(new_obj.oid);
	}

	return *this;
}


DarkHelp::PositionTracker & DarkHelp::PositionTracker::remove_old_objects()
{
	// delete old entries that have not been updated in a while

	if (age_of_objects_before_deletion > 0 and most_recent_frame_id > age_of_objects_before_deletion)
	{
		auto iter = objects.begin();
		while (iter != objects.end())
		{
			const auto & obj = *iter;
			const auto & last_seen = obj.last_seen_frame_id();
			if (most_recent_frame_id - last_seen > age_of_objects_before_deletion)
			{
				iter = objects.erase(iter);
				continue;
			}

			iter ++;
		}
	}

	if (maximum_number_of_frames_per_object >= 10)
	{
		for (auto & obj : objects)
		{
			if (obj.fids_and_rects.size() > maximum_number_of_frames_per_object)
			{
				// do not delete the very first entry, so we know exactly where an object appeared
				auto iter1 = obj.fids_and_rects.upper_bound(obj.first_seen_frame_id());

				// keep the last 40 or so frames in case the user wants to draw a tail
				auto iter2 = obj.fids_and_rects.lower_bound(most_recent_frame_id - maximum_number_of_frames_per_object / 2);

				obj.fids_and_rects.erase(iter1, iter2);
			}
		}
	}

	return *this;
}


std::ostream & DarkHelp::operator<<(std::ostream & os, const DarkHelp::PositionTracker::Obj & obj)
{
	const size_t first_fid	= obj.first_seen_frame_id();
	const size_t last_fid	= obj.last_seen_frame_id();

	os	<< "oid="		<< obj.oid
		<< " frames="	<< obj.fids_and_rects.size()
		<< " first="	<< first_fid
		<< " last="		<< last_fid
		<< " missing="	<< (1 + last_fid - first_fid - obj.fids_and_rects.size())
		<< " center="	<< obj.center()
		<< " size="		<< obj.size()
		;

	return os;
}


std::ostream & DarkHelp::operator<<(std::ostream & os, const DarkHelp::PositionTracker & tracker)
{
	os	<< "Position Tracker:"												<< std::endl
		<< "-> most recent frame .... " << tracker.most_recent_frame_id		<< std::endl
		<< "-> most recent object ... " << tracker.most_recent_object_id	<< std::endl
		<< "-> tracked objects ...... " << tracker.objects.size();

	for (const auto & obj : tracker.objects)
	{
		os << std::endl << "-> " << obj;
	}

	return os;
}
