# Sample Encoder

The sample encoder implements the encoding process specified in the VC-5 document suite
and includes a minimal image unpacking process for reading some common image file formats.

The sample encoder uses compile-time and run-time switches to control what parts of the
VC-5 specification are included in the build. The compile-time switches are set in the common
[configuration file](../common/include/config.h). The run-time switches are set on the command
line (see the documentation for details).

The sample encoder has been built on macOS and Linux using both make and CMake and on
Windows using CMake to create a Visual Studio project. CMake can be used to build the
sample encoder from the command line on Windows. See the [build documentation](../notes/install.html) for details.

## Directory Structure

Description of the important files and folders in the encoder directory:

[Archive](./Archive)
> Old files that will be removed from the code base.

[Makefile](./Makefile)
> Make file for building the sample encoder using make of CMake.

[include](./include)
> Header files used only by the sample encoder

[src](./src)
> Source files for the sample encoder

[build](./build)
> Directory tree for the intermediate build files and the encoder executable.
The encoder can be built using make or CMake depending on which tools are available
on the platform.

> Windows does not have a good implementation of make, so CMake is the
only method for building the sample encoder on Windows. CMake creates solution and
project files for editing and building the sample encoder using Visual Studio.

[metadata](./metadata)
> Metadata files output by the encoder for testing conformance. The files are in the
[media](../media/metadata/) directory included in the software distribution.

[test-encoder.sh](./test-encoder.sh)
> Bash script for testing the encoder.

[verify-metadata.sh](./verify-metadata.sh)
> Bash script for testing the encoding of metadata into the bitstream.

[encoder.doxy](./encoder.doxy)
> Doxygen configuration file for creating documentation for the sample encoder.

[encoder.sublime-project](./encoder.sublime-project)
> Sublime Text project for editing the sample encoder.

## Build Status

Status of builds for the VC-5 encoder.

| Platform |     Make      |     CMake     |
| -------- | :-----------: | :-----------: |
| macOS    | Build and Run | Build and Run |
| Windows  |      N/A      | Build and Run |
| Ubuntu   | Build and Run | Build and Run |



Build: Can compile and link the encoder (debug and release) using the specified build method.

Run: Can build and execute the encoder (debug and release) without command-line arguments to output the help message.

Test: Can build the encoder (debug and release) and run the test suite.

