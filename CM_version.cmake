# DarkHelp - C++ helper class for Darknet's C API.
# Copyright 2019-2021 Stephane Charette <stephanecharette@gmail.com>
# MIT license applies.  See "license.txt" for details.


FILE ( READ version.txt VERSION_TXT )
STRING ( STRIP "${VERSION_TXT}" VERSION_TXT )
STRING ( REGEX MATCHALL "^([0-9]+)\\.([0-9]+)\\.([0-9]+)-([0-9]+)$" OUTPUT ${VERSION_TXT} )

SET ( DH_VER_MAJOR	${CMAKE_MATCH_1} )
SET ( DH_VER_MINOR	${CMAKE_MATCH_2} )
SET ( DH_VER_PATCH	${CMAKE_MATCH_3} )
SET ( DH_VER_COMMIT	${CMAKE_MATCH_4} )

SET ( DH_VERSION ${DH_VER_MAJOR}.${DH_VER_MINOR}.${DH_VER_PATCH}-${DH_VER_COMMIT} )
MESSAGE ( "Building ver: ${DH_VERSION}" )

ADD_DEFINITIONS ( -DDH_VERSION="${DH_VERSION}" )
