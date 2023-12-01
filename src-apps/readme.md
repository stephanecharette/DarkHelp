# What is the src-apps directory?

These are a series of very simple single-purpose apps to show how to use the DarkHelp C++ API.  These are meant to be used as a starting point for people who want to use Darknet and DarkHelp.  They are not meant to be final production or commercial-ready applications.

Find the one that seems to do what you want, and modify it as needed.  The examples as shown below are typically done with [the Rolodex example dataset](https://www.ccoderun.ca/programming/2023-11-06_Rolodex/).

# App Descriptions

* `process_single_image.cpp`  Loads a neural network, call `predict()` on a single image, and display the results on the console.  Should look similar to this:
	* Run:  `src-apps/process_single_image Rolodex.cfg Rolodex.names Rolodex_best.weights page_70.png`
	* Output should look similar to this:
```bash
page_70.png prediction results: 10
-> 1/10: "user level 2 100%" #7 prob=0.996683 x=236 y=421 w=15 h=23 oid=0 tile=0 entries=1
-> 2/10: "date of birth 100%" #2 prob=0.999682 x=143 y=331 w=207 h=23 oid=0 tile=0 entries=1
-> 3/10: "female 100%" #4 prob=0.999683 x=535 y=331 w=13 h=22 oid=0 tile=0 entries=1
-> 4/10: "marital status 100%" #5 prob=0.999713 x=299 y=371 w=16 h=23 oid=0 tile=0 entries=1
-> 5/10: "serial number 100%" #12 prob=0.999727 x=423 y=461 w=219 h=23 oid=0 tile=0 entries=1
-> 6/10: "address 100%" #1 prob=0.999777 x=50 y=240 w=437 h=67 oid=0 tile=0 entries=1
-> 7/10: "avt 100%" #11 prob=0.999786 x=533 y=420 w=208 h=24 oid=0 tile=0 entries=1
-> 8/10: "barcode 100%" #14 prob=0.999866 x=548 y=523 w=186 h=92 oid=0 tile=0 entries=1
-> 9/10: "name 100%" #0 prob=0.999887 x=60 y=176 w=430 h=42 oid=0 tile=0 entries=1
-> 10/10: "qr code 100%" #13 prob=0.999931 x=98 y=522 w=101 h=100 oid=0 tile=0 entries=1
```

* `display_single_image.cpp`  Loads a neural network, call `predict()` on a single image, annotate the image with the results, and display using a GUI window.
	* Run:  `src-apps/display_single_image Rolodex.cfg Rolodex.names Rolodex_best.weights page_70.png`

* `display_single_image_snapping.cpp`  Similar to the previous app, but enables [the "snapping" feature](https://www.ccoderun.ca/darkhelp/api/classDarkHelp_1_1Config.html#af5a408e8347469584373338271007ede).
	* Run:   `src-apps/display_single_image_snapping Rolodex.cfg Rolodex.names Rolodex_best.weights page_70.png`

* `display_single_image_custom_settings.cpp`  Similiar to the previous app, shows how to customize a number of the DarkHelp and Darknet settings.
	* Run:  `src-apps/display_single_image_custom_settings Rolodex.cfg Rolodex.names Rolodex_best.weights page_70.png`

* `process_video_webcam.cpp`  Loads a neural network, open the first webcam, and continuously apply the network to every video frame.  Press `ESC` to exit.
	* If you have a webcam, try and run the MSCOCO pre-trained weights with this one.  Download [the MSCOCO YOLOv4-tiny weights](https://github.com/hank-ai/darknet#mscoco-pre-trained-weights).
	* Run:  `src-apps/process_video_webcam yolov4-tiny.cfg coco.names yolov4-tiny.weights`

* `save_webcam_to_video.cpp` Similar to the previous app, but the output is saved back out to disk, named `out.mp4`.  Press `ESC` to stop recording and exit.
	* Run:  `src-apps/save_webcam_to_video yolov4-tiny.cfg coco.names yolov4-tiny.weights`

* `rotate_images.cpp` Detects image rotation, rotates the images back to level, and then re-runs the neural network to get more accurate detections.
	* See:  <https://www.ccoderun.ca/programming/2023-11-26_YOLO_and_image_rotation/>
	* Run:  `src-apps/rotate_image Perfs.cfg Perfs.names Perfs_best.weights ~/nn/Perfs/set_01/film*.jpg`

* `video_display_realtime.cpp` Loads a video file and displays it at the exact speed it was recorded, running the neural network on each frame.
	* Run:  `src-apps/video_display_realtime animals.cfg animals.names animals_best.weights zoo.m4v`

* `video_object_counter.cpp` Combines predictions and object tracker to count the number of objects.
	* See:  output should be similar to this:  <https://youtu.be/2biQpVRFhbk>
	* Run:  `src-apps/video_object_counter pigs.cfg pigs.names pigs_best.weights farm.m4v`
