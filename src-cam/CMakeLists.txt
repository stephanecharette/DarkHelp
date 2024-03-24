# DarkHelp - C++ helper class for Darknet's C API.
# Copyright 2019-2024 Stephane Charette <stephanecharette@gmail.com>
# MIT license applies.  See "license.txt" for details.


FILE (GLOB SRC_CAM *.cpp)
LIST (SORT SRC_CAM)

ADD_EXECUTABLE (DarkHelp_cam ${SRC_CAM})
TARGET_LINK_LIBRARIES (DarkHelp_cam PRIVATE Threads::Threads dh ${Darknet} ${OpenCV_LIBS})
INSTALL (TARGETS DarkHelp_cam DESTINATION bin)
