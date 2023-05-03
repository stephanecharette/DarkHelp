#pragma once

#include <DarkHelp.hpp>
#include <fstream>


namespace DarkHelp
{
	/** This class attempts to do very simple object tracking based on the position of the object.  It assumes that objects
	 * move a small amount between frames, and don't randomly appear and disapear from view.
	 *
	 * @li This tracker works best when the camera frame rate is high enough to @ref DarkHelp::PositionTracker::maximum_distance_to_consider "minimize the distance" an object moves.
	 * @li This tracker makes no attempt to re-identify objects that move off-screen and then come back into view.  A new ID will be assigned to the object.
	 *
	 * @image html tracking_cars.jpg
	 *
	 * @see @ref DarkHelp::PositionTracker::PositionTracker()
	 *
	 * @since May 2023
	 */
	class PositionTracker final
	{
		public:

			/** The position tracker uses @p Obj to keep information on objects that are being tracked.
			 * This includes the frame IDs and the bounding box rectangles for the object on each of those frames.
			 * If needed (for example to draw a tail showing where the object has been) these object structures
			 * can be obtained via @ref PositionTracker::get().
			 *
			 * @since May 2023
			 */
			struct Obj final
			{
				/// A unique object ID assigned to this object.
				size_t oid;

				/** Store an entry for every frame where this object was detected.
				 * The key is the frame ID, and the value is the rectangle on that frame.
				 */
				std::map<size_t, cv::Rect> fids_and_rects;

				/// Every class detected with a threshold > 0.2.  This is used to find a match in new frames.
				std::set<size_t> classes;

				/// Constructor.
				Obj() { clear(); }

				/// Destructor.
				~Obj() { return; }

				/// Reset this object.  Sets the @ref oid to zero and removes any frames, rectangles, and classes.
				Obj & clear();

				/// Returns @p true if this object has no object ID or frame information.
				bool empty() const;

				/// Returns the frame where this object first appeared.
				size_t first_seen_frame_id() const;

				/// Returns the most recent frame.
				size_t last_seen_frame_id() const;

				/// Returns the rectangle from the most recent frame.
				cv::Rect rect() const;

				/// The central point of the object.  This uses the most recent frame.
				cv::Point center() const;

				/// The size of the object.  This uses the most recent frame.
				cv::Size size() const;
			};

			/// A @p std::list type definition of objects.  @see @ref DarkHelp::PositionTracker::objects
			using Objects = std::list<Obj>;

			/** Constructor.  Instantiate one of these trackers (on the stack, or as a member of another class for example) and
			 * every time you call @ref DarkHelp::NN::predict() you need to pass the results into the tracker.  For example:
			 *
			 * ~~~~
			 * DarkHelp::NN nn("pigs.cfg", "pigs.names", "pigs_best.weights");
			 * DarkHelp::PositionTracker tracker;
			 *
			 * cv::VideoCapture cap("pigs.mp4");
			 *
			 * while (cap.isOpened())
			 * {
			 * 	cv::Mat frame;
			 * 	cap >> frame;
			 * 	if (frame.empty())
			 * 	{
			 * 		break;
			 * 	}
			 *
			 * 	auto results = nn.predict(mat);
			 * 	tracker.add(results);
			 *
			 * 	std::cout << results << std::endl;
			 * 	std::cout << tracker << std::endl;
			 *
			 * 	for (const auto & prediction : results)
			 * 	{
			 * 		// get the tracking details for this specific prediction
			 * 		const auto & obj = tracker.get(prediction.object_id);
			 * 		cv::putText(mat, std::to_string(obj.oid, obj.center(), cv::FONT_HERSHEY_SIMPLEX, 0.75, {0, 0, 0}, 1, cv::LINE_AA);
			 *
			 * 		// use the many rectangles stored in obj.fids_and_rects to draw the tail
			 * 		// ...etc...
			 * 	}
			 *
			 * 	cv::imshow("output", mat);
			 * 	cv::waitKey();
			 * }
			 * ~~~~
			 *
			 * Once the call into @ref PositionTracker::add() returns, the @p results will have been modified with the unique
			 * tracking object IDs (@ref DarkHelp::PredictionResult::object_id).
			 *
			 * %DarkHelp itself doesn't draw the tracking details on images.  But it is trivial to use standard OpenCV calls like
			 * @p cv::putText() and @p cv::circle() to add object IDs and tails to images:
			 *
			 * @image html tracking_pigs.jpg
			 *
			 * @see @ref DarkHelp::PositionTracker::add()
			 * @see @ref DarkHelp::PositionTracker::get()
			 * @see @ref DarkHelp::PositionTracker::age_of_objects_before_deletion
			 * @see @ref DarkHelp::PositionTracker::maximum_number_of_frames_per_object
			 * @see @ref DarkHelp::PositionTracker::maximum_distance_to_consider
			 */
			PositionTracker();

			/// Destructor.
			~PositionTracker();

			/// Returns @p true if zero objects are being tracked, otherwise returns @p false.
			bool empty() const { return objects.empty(); }

			/// Returns the total number of objects currently being tracked.
			size_t size() const { return objects.size(); }

			/** Removes all objects being tracked and resets the frame ID and object ID variables.  This will also reset all the
			 * configuration variables such as @ref DarkHelp::PositionTracker::maximum_distance_to_consider and
			 * @ref DarkHelp::PositionTracker::maximum_number_of_frames_per_object to their default values.
			 */
			PositionTracker & clear();

			/** Add the %DarkHelp results to the tracker so it can start assinging OIDs.  This automatically increments the frame
			 * ID counter and calls @ref process() internally to manage matching up the predictions with all known tracked objects,
			 * as well as creating new entries for objects that have not yet been seen.
			 *
			 * @note You must remember to call @ref add() for every video frame to ensure objects are properly tracked.  See the
			 * example code in @ref DarkHelp::PositionTracker::PositionTracker().
			 */
			PositionTracker & add(DarkHelp::PredictionResults & results);

			/** Get a reference to the @ref DarkHelp::PositionTracker::Obj "object" that matches the given OID.  This will throw
			 * if the requested OID does not exist.  The object will provide you with the frame ID where the object first appeared,
			 * the frame ID when it was last seen, and the corresponding bounding box rectangles for many of the previous frames.
			 *
			 * The number of items stored and retrieved for each object is determined by
			 * @ref DarkHelp::PositionTracker::maximum_number_of_frames_per_object.
			 */
			const Obj & get(const size_t oid) const;

			/** The most recent object ID that was added to the tracker.  This is automatically incremented when calling
			 * @ref add().  The special value @p zero is reserved to indicate no object ID.  This means objects are numbered
			 * sequentially starting with @p 1.
			 *
			 * Under normal circumstances you shouldn't need to access nor modify this value.
			 */
			size_t most_recent_object_id;

			/** The most recent frame ID that was added to the tracker.  This is automatically incremented when calling
			 * @ref add().  The special value @p zero is resrved to indicate no frame ID.  This means frames are numbered
			 * sequentially starting with @p 1.
			 *
			 * Under normal circumstances you shouldn't need to access nor modify this value.
			 */
			size_t most_recent_frame_id;

			/** A "database" of all the currently tracked objects.
			 * This is regularly prunned by automatic calls to @ref remove_old_objects().
			 *
			 * Under normal circumstances you shouldn't need to access nor modify this value.
			 * Instead, use @ref DarkHelp::PositionTracker::get().
			 *
			 * @see @ref add()
			 * @see @ref get()
			 */
			Objects	objects;

			/** When an object hasn't been seen for several frames, it is removed from the tracker and all associated data is
			 * deleted.  This includes frame references, bounding rectangles, and object ID.  The length of time it takes for
			 * this to happen is measured in frames.  If you set it for too long, then the object ID might end up being re-used,
			 * but if it is set too short then an object may jump to a new object ID if detection misses it in a frame.
			 *
			 * The right value to use depends on how quickly the objects in your video move, how successful detection is on
			 * every frame, whether your objects may be momentarily obscured from view, and the FPS for your input video.
			 *
			 * This value is the number of consecutive frames that must elapse before an object is removed.  Set to @p zero
			 * to keep all objects.  Default value is @p 10.
			 *
			 * @since May 2023
			 */
			size_t age_of_objects_before_deletion;

			/** This is used to limit the number of frames and rectangles stored for each tracked object.  The first and last
			 * few frames and rectangles are always kept, but those in the middle can be pruned to limit the amount of memory
			 * that object tracking consumes.  Set to @p zero to keep all frames and rectangles.  Default value is 90, which
			 * is 3 seconds of data with a typical 30 FPS video.
			 *
			 * @since May 2023
			 */
			size_t maximum_number_of_frames_per_object;

			/** The maximum distance the tracker will consider when attempting to find a match between an object on a previous
			 * frame and one on a new frame.  If the distance between the old position and the new position is greater than this
			 * value, then a match will not be made and the object will instead be assigned a new ID.  The value you use for this
			 * will depend on:
			 *
			 * @li your image size,
			 * @li your network dimensions,
			 * @li and most importantly, the rate at which objects are moving across the image.
			 *
			 * This value is the distance <b>in pixels</b> between the old position and the new position.  The reason it is a
			 * @p double and not an @p int is because the distance is calculated using @p cv::norm() (aka pythagoras).
			 *
			 * Default value is @p 100.0.
			 *
			 * @since May 2023
			 */
			double maximum_distance_to_consider;

		protected:

			/// This is called internally by @ref DarkHelp::PositionTracker::add().
			PositionTracker & process(const size_t frame_id, DarkHelp::PredictionResults & results);

			/// This is called internally by @ref DarkHelp::PositionTracker::add().
			PositionTracker & remove_old_objects();
	};

	/** Convenience function to stream a single tracked object as a line of text.
	 * Mostly intended for debug or logging purposes.
	 */
	std::ostream & operator<<(std::ostream & os, const DarkHelp::PositionTracker::Obj & obj);

	/** Convenience function to stream the entire object tracker as text.
	 * Mostly intended for debug or logging purposes.
	 *
	 * Example:
	 * ~~~~
	 * auto results = nn.predict("traffic.jpg");
	 * tracker.add(results);
	 * std::cout << tracker << std::endl;
	 * ~~~~
	 */
	std::ostream & operator<<(std::ostream & os, const DarkHelp::PositionTracker & tracker);
}
