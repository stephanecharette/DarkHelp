#!/usr/bin/env python3

import DarkHelp

# Sample code to show how to use the DarkHelp C API in Python.

print("DarkHelp v" + DarkHelp.DarkHelpVersion().decode("utf-8"))
print("Darknet v" + DarkHelp.DarknetVersion().decode("utf-8"))

# The "Rolodex" example dataset can be downloaded from https://www.ccoderun.ca/programming/2023-11-06_Rolodex/
cfg_filename = "Rolodex.cfg".encode("utf-8")
names_filename = "Rolodex.names".encode("utf-8")
weights_filename = "Rolodex_best.weights".encode("utf-8")

# By default CreateDarknetNN() will toggle the output redirection to hide the
# output from Darknet, so call ToggleOutputRediretion() just prior to that if
# you do want to see the neural network being loaded.
#DarkHelp.ToggleOutputRedirection()

# The order in which the .cfg, .names, and .weights file are specified does
# not matter.  DarkHelp will automatically figure out which file is which.
dh = DarkHelp.CreateDarkHelpNN(cfg_filename, names_filename, weights_filename)
if not dh:
    print("""
Failed to allocate a DarkHelp object.  Possible problems include:

  1) missing neural network files, or files are in a different directory
  2) libraries needed by DarkHelp or Darknet have not been installed
  3) errors in DarkHelp or Darknet libraries
""")
    quit(1)

# If you toggled the output redirection above, remember to restore it once the
# network has been loaded, otherwise all your output will go to /dev/null.
#DarkHelp.ToggleOutputRedirection()

# Configure DarkHelp to ensure it behaves like we want.
DarkHelp.SetThreshold(dh, 0.35)
DarkHelp.EnableTiles(dh, False)
DarkHelp.EnableSnapping(dh, True)
DarkHelp.EnableUseFastImageResize(dh, False)
DarkHelp.EnableNamesIncludePercentage(dh, True)
DarkHelp.EnableAnnotationAutoHideLabels(dh, False)
DarkHelp.EnableAnnotationIncludeDuration(dh, False)
DarkHelp.EnableAnnotationIncludeTimestamp(dh, False)
DarkHelp.SetAnnotationLineThickness(dh, 1)

# Now we run all our images through DarkHelp, generating both an
# output image and the JSON string with all of the predicitions.
# This is the code you'd run in a loop if you have many images.
DarkHelp.PredictFN(dh, "page_1.png".encode("utf-8"))
DarkHelp.Annotate(dh, "output.jpg".encode("utf-8"))
json = DarkHelp.GetPredictionResults(dh)
print(json.decode())

# When done with DarkHelp, destroy the object to release memory.
DarkHelp.DestroyDarkHelpNN(dh)
