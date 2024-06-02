# Reference Decoder

The reference decoder implements the decoding process specified in the VC-5 document suite
and includes a minimal image repacking process for creating a displayable image.

The reference decoder uses compile-time and run-time switches to control what parts of the
VC-5 specification are included in the build. The compile-time switches are set in the common
[configuration file](../common/include/config.h). The run-time switches are set on the command
line (see the documentation for details).

The reference decoder has been built on macOS and Linux using both make and CMake and on
Windows using CMake to create a Visual Studio project. CMake can be used to build the
reference decoder from the command line on Windows. See the [build documentation](../notes/install.html) for details.

## Directory Structure

Description of the important files and folders in the decoder directory:

[Archive](./Archive)
> Old files that will be removed from the code base.

[Makefile](./Makefile)
> Make file for building the reference decoder using make of CMake.

[include](./include)
> Header files used only by the reference decoder

[src](./src)
> Source files for the reference decoder

[build](./build)
> Directory tree for the intermediate build files and the decoder executable.
The decoder can be built using make or CMake depending on which tools are available
on the platform.

> Windows does not have a good implementation of make, so CMake is the
only method for building the reference decoder on Windows. CMake creates solution and
project files for editing and building the reference decoder using Visual Studio.

[metadata](./metadata)
> Metadata files output by the decoder for testing conformance. The files are in the
[media](../media/metadata/) directory included in the software distribution.


[test-decoder.sh](./test-decoder.sh)
> Bash script for testing the decoder.

[decoder.doxy](./decoder.doxy)
> Doxygen configuration file for creating documentation for the reference decoder.

[decoder.sublime-project](./decoder.sublime-project)
> Sublime Text project for editing the reference decoder.

## Build Status

Status of builds for the VC-5 decoder.

| Platform |       Make        |       CMake       |
| -------- | :---------------: | :---------------: |
| macOS    | Build and Run [1] | Build and Run [1] |
| Windows  |                   |   Build and Run   |
| Ubuntu   |   Build and Run   |   Build and Run   |



Build: Can compile and link the decoder (debug and release) using the specified build method.

Run: Can build and execute the decoder (debug and release) without command-line arguments to output the help message.

Test: Can build the decoder (debug and release) and run the test suite.

[1] Forced use of Mini-XML static library after many attempts to get dyanamic library to link properly.

