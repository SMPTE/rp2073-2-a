# Scripts for Testing and Managing the VC-5 Software


## testcodec.tcl

The original codec test script that is still used for testing the sample encoder and
reference decoder by comparing the output files with the master bitstreams and decoded images
in the media directory.

This script is only used for testing VC-5 parts 1-6.

The new test script `testcodec.py` is used for testing the conformance of the sample encoder and
reference decoder to VC-5 part 7 and will eventually replace `testcodec.tcl` as the Tcl code has
become large and difficult to maintain and Tcl is no longer widely used.

The files `testcodec-unix.tcl` and `testcodec-windows.tcl` contain platform-specific code and
settings used by the `testcodec.tcl` script.

The file `testcode-suite.tcl` contains code used to build the list of test cases run by the
`testcodec.tcl` script.


## testcodec.py

New script for testing the sample encoder and reference decoder.


## display.py

Python script to display unformatted image files.


## dpxcurve.py

Script used to calculate the DPX encoding curve in ST 2073-7, section B.9.5.


## dumper.py

Script to output a human-readable representation of a VC-5 bitstream.


## getmedia.py

Script to download test materials (media files) from the S3 bucket used for the VC-5 codec.

The media files are now stored in Git LFS which is more convenient since the media files are
(by default) downloaded when the repository is cloned.

The media files are still in Amazon S3 but even with the `getmedia.py` script, it is not
convenient to obtain the media files from this source due to access restrictions.

The `getmedia.py` script optionally uses the file `testmedia.json` to determine which media files to download.

## listfiles.py

Script to list specified information for all files in a directory tree.


## requirements.txt

List of packages and version numbers used by the Python scripts.


