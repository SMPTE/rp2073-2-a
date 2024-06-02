# Platform-specific definitions file for Windows
#
# (c) 2013-2017 Society of Motion Picture & Television Engineers LLC and Woodman Labs, Inc.
# All rights reserved--use subject to compliance with end user license agreement.

# Set the location of the root directory for the test cases
set media [file normalize "C:/Media/VC-5"]

# Set the directory that contains the build results
#set compiler VS2005
set build cmake

# Set the filenames of the executable programs
set encoder encoder.exe
set decoder decoder.exe
