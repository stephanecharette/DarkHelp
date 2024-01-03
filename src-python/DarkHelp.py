#!/usr/bin/env python3

"""
Python 3 wrapper for the DarkHelp library.  See "example.py" in the same directory as this file.

The DarkHelp repo where the most recent version of this file can be downloaded is here:

https://github.com/stephanecharette/DarkHelp#what-is-the-darkhelp-c-api
"""

from ctypes import *
import os

if os.name == "posix":
    libpath = "/usr/lib/libdarkhelp.so"
elif os.name == "nt":
    libpath = "C:/Program Files/darknet/bin/darkhelp.dll"
else:
    print("unknown OS")
    exit
lib = CDLL(libpath, RTLD_GLOBAL)


"""
Get the DarkHelp version number from DarkHelp::version.
@see @ref DarknetVersion
@see @ref DarkHelpVersion()
@see @ref DarkHelp::version()
"""
DarkHelpVersion = lib.DarkHelpVersion
DarkHelpVersion.argtypes = None
DarkHelpVersion.restype = c_char_p

"""
Get the Darknet version number.
@see @ref DarkHelpVersion
@see @p DARKNET_VERSION_SHORT in Darknet
"""
DarknetVersion = lib.DarknetVersion
DarknetVersion.argtypes = None
DarknetVersion.restype = c_char_p

"""
Hide or show output on @p STDOUT and @p STDERR.
@see @ref ToggleOutputRedirection()
@see @ref DarkHelp::toggle_output_redirection()
"""
ToggleOutputRedirection = lib.ToggleOutputRedirection
ToggleOutputRedirection.argtypes = None
ToggleOutputRedirection.restype = None

"""
Create a %DarkHelp object.  Remember to call @ref DestroyDarkHelpNN() when done.
@see @ref DarkHelp::NN::NN()
@see @ref DestroyDarkHelpNN()
"""
CreateDarkHelpNN = lib.CreateDarkHelpNN
CreateDarkHelpNN.argtypes = (c_char_p, c_char_p, c_char_p)
CreateDarkHelpNN.restype = c_void_p

"""
Destroy a %DarkHelp object created using @ref CreateDarkHelpNN().
@see @ref DarkHelp::NN::~NN()
"""
DestroyDarkHelpNN = lib.DestroyDarkHelpNN
DestroyDarkHelpNN.argtypes = [c_void_p]
DestroyDarkHelpNN.restype = None

"""
Calls DarkHelp's @p predict() with the given image.  This automatically does
tiling and puts the results back together if tiling has been enabled.
@see @ref Predict()
@see @ref DarkHelp::NN::predict()
"""
PredictFN = lib.PredictFN
PredictFN.argtypes = [c_void_p, c_char_p]
PredictFN.restype = c_int

"""
Calls DarkHelp's @p predict() with the given image.  This automatically does
tiling and puts the results back together if tiling has been enabled.
@see @ref PredictFN()
@see @ref DarkHelp::NN::predict()
"""
Predict = lib.Predict
Predict.argtypes = [c_void_p, c_int, c_int, c_char_p, c_int]
Predict.restype = c_int

"""
Get the prediction results as a JSON string.
"""
GetPredictionResults = lib.GetPredictionResults
GetPredictionResults.argtypes = [c_void_p]
GetPredictionResults.restype = c_char_p

"""
Calls DarkHelp's @p annotate() with the last image to be processed by
DarkHelp's @p predict().
@see @ref DarkHelp::NN::annotate()
"""
Annotate = lib.Annotate
Annotate.argtypes = [c_void_p, c_char_p]
Annotate.restype = None

"""
Set the minimum detection threshold which will be used the next time
@ref DarkHelp::NN::predict() is called.
@see @ref DarkHelp::NN::Config::threshold
"""
SetThreshold = lib.SetThreshold
SetThreshold.argtypes = [c_void_p, c_float]
SetThreshold.restype = c_float

"""
Set the NMS threshold which will be used the next time
@ref DarkHelp::NN::predict() is called.
@see @ref DarkHelp::NN::Config::non_maximal_suppression_threshold
"""
SetNonMaximalSuppression = lib.SetNonMaximalSuppression
SetNonMaximalSuppression.argtypes = [c_void_p, c_float]
SetNonMaximalSuppression.restype = c_float

"""
Toggles whether the percentage is shown in the label.
@see @ref DarkHelp::Config::names_include_percentage
"""
EnableNamesIncludePercentage = lib.EnableNamesIncludePercentage
EnableNamesIncludePercentage.argtypes = [c_void_p, c_bool]
EnableNamesIncludePercentage.restype = c_bool

"""
@see @ref DarkHelp::Config::annotation_auto_hide_labels
"""
EnableAnnotationAutoHideLabels = lib.EnableAnnotationAutoHideLabels
EnableAnnotationAutoHideLabels.argtypes = [c_void_p, c_bool]
EnableAnnotationAutoHideLabels.restype = c_bool

"""
@see @ref DarkHelp::Config::annotation_suppress_all_labels
"""
EnableAnnotationSuppressAllLabels = lib.EnableAnnotationSuppressAllLabels
EnableAnnotationSuppressAllLabels.argtypes = [c_void_p, c_bool]
EnableAnnotationSuppressAllLabels.restype = c_bool

"""
@see @ref DarkHelp::Config::annotation_shade_predictions
"""
SetAnnotationShadePredictions = lib.SetAnnotationShadePredictions
SetAnnotationShadePredictions.argtypes = [c_void_p, c_float]
SetAnnotationShadePredictions.restype = c_float

"""
@see @ref DarkHelp::Config::include_all_names
"""
EnableIncludeAllNames = lib.EnableIncludeAllNames
EnableIncludeAllNames.argtypes = [c_void_p, c_bool]
EnableIncludeAllNames.restype = c_bool

"""
@see @ref DarkHelp::Config::annotation_font_scale
"""
SetAnnotationFontScale = lib.SetAnnotationFontScale
SetAnnotationFontScale.argtypes = [c_void_p, c_double]
SetAnnotationFontScale.restype = c_double

"""
@see @ref DarkHelp::Config::annotation_font_thickness
"""
SetAnnotationFontThickness = lib.SetAnnotationFontThickness
SetAnnotationFontThickness.argtypes = [c_void_p, c_int]
SetAnnotationFontThickness.restype = c_int

"""
@see @ref DarkHelp::Config::annotation_line_thickness
"""
SetAnnotationLineThickness = lib.SetAnnotationLineThickness
SetAnnotationLineThickness.argtypes = [c_void_p, c_int]
SetAnnotationLineThickness.restype = c_int

"""
@see @ref DarkHelp::Config::annotation_include_duration
"""
EnableAnnotationIncludeDuration = lib.EnableAnnotationIncludeDuration
EnableAnnotationIncludeDuration.argtypes = [c_void_p, c_bool]
EnableAnnotationIncludeDuration.restype = c_bool

"""
@see @ref DarkHelp::Config::annotation_include_timestamp
"""
EnableAnnotationIncludeTimestamp = lib.EnableAnnotationIncludeTimestamp
EnableAnnotationIncludeTimestamp.argtypes = [c_void_p, c_bool]
EnableAnnotationIncludeTimestamp.restype = c_bool

"""
@see @ref DarkHelp::Config::annotation_pixelate_enabled
"""
EnableAnnotationPixelate = lib.EnableAnnotationPixelate
EnableAnnotationPixelate.argtypes = [c_void_p, c_bool]
EnableAnnotationPixelate.restype = c_bool

"""
@see @ref DarkHelp::Config::annotation_pixelate_size
"""
SetAnnotationPixelateSize = lib.SetAnnotationPixelateSize
SetAnnotationPixelateSize.argtypes = [c_void_p, c_int]
SetAnnotationPixelateSize.restype = c_int

"""
@see @ref DarkHelp::Config::enable_tiles
"""
EnableTiles = lib.EnableTiles
EnableTiles.argtypes = [c_void_p, c_bool]
EnableTiles.restype = c_bool

"""
@see @ref DarkHelp::Config::combine_tile_predictions
"""
EnableCombineTilePredictions = lib.EnableCombineTilePredictions
EnableCombineTilePredictions.argtypes = [c_void_p, c_bool]
EnableCombineTilePredictions.restype = c_bool

"""
@see @ref DarkHelp::Config::only_combine_similar_predictions
"""
EnableOnlyCombineSimilarPredictions = lib.EnableOnlyCombineSimilarPredictions
EnableOnlyCombineSimilarPredictions.argtypes = [c_void_p, c_bool]
EnableOnlyCombineSimilarPredictions.restype = c_bool

"""
@see @ref DarkHelp::Config::tile_edge_factor
"""
SetTileEdgeFactor = lib.SetTileEdgeFactor
SetTileEdgeFactor.argtypes = [c_void_p, c_float]
SetTileEdgeFactor.restype = c_float

"""
@see @ref DarkHelp::Config::tile_rect_factor
"""
SetTileRectFactor = lib.SetTileRectFactor
SetTileRectFactor.argtypes = [c_void_p, c_float]
SetTileRectFactor.restype = c_float

"""
@see @ref DarkHelp::Config::snapping_enabled
"""
EnableSnapping = lib.EnableSnapping
EnableSnapping.argtypes = [c_void_p, c_bool]
EnableSnapping.restype = c_bool

"""
@see @ref DarkHelp::Config::binary_threshold_block_size
"""
SetBinaryThresholdBlockSize = lib.SetBinaryThresholdBlockSize
SetBinaryThresholdBlockSize.argtypes = [c_void_p, c_int]
SetBinaryThresholdBlockSize.restype = c_int

"""
@see @ref DarkHelp::Config::binary_threshold_constant
"""
SetBinaryThresholdConstant = lib.SetBinaryThresholdConstant
SetBinaryThresholdConstant.argtypes = [c_void_p, c_double]
SetBinaryThresholdConstant.restype = c_double

"""
@see @ref DarkHelp::Config::snapping_horizontal_tolerance
"""
SetSnappingHorizontalTolerance = lib.SetSnappingHorizontalTolerance
SetSnappingHorizontalTolerance.argtypes = [c_void_p, c_int]
SetSnappingHorizontalTolerance.restype = c_int

"""
@see @ref DarkHelp::Config::snapping_vertical_tolerance
"""
SetSnappingVerticalTolerance = lib.SetSnappingVerticalTolerance
SetSnappingVerticalTolerance.argtypes = [c_void_p, c_int]
SetSnappingVerticalTolerance.restype = c_int

"""
@see @ref DarkHelp::Config::snapping_limit_shrink
"""
SetSnappingLimitShrink = lib.SetSnappingLimitShrink
SetSnappingLimitShrink.argtypes = [c_void_p, c_float]
SetSnappingLimitShrink.restype = c_float

"""
@see @ref DarkHelp::Config::snapping_limit_grow
"""
SetSnappingLimitGrow = lib.SetSnappingLimitGrow
SetSnappingLimitGrow.argtypes = [c_void_p, c_float]
SetSnappingLimitGrow.restype = c_float

"""
@see @ref DarkHelp::Config::use_fast_image_resize
"""
EnableUseFastImageResize = lib.EnableUseFastImageResize
EnableUseFastImageResize.argtypes = [c_void_p, c_bool]
EnableUseFastImageResize.restype = c_bool
