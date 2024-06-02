---
title: |
	VC-5 Test Materials \
	Install and Build Instructions \
	Version {{version.major}}.{{version.minor}}.{{version.revision}}
mustache: ../version.yaml
---


# Introduction

The VC-5 test materials include a sample encoder, reference decoder, and utility programs and scripts to assist in testing the VC-5 codec.
This document provides detailed information on installing, building, and running the software, including tools and libraries
required to run the software and the method for downloading test media.

Support for embedded metadata as described in ST 2073-7 is described in a [separate document](../metadata/notes/install.md).


# Restrictions

This release is provided for the sole purpose of evaluating the source code prior to purchase of the VC-5 standards
and source code from SMPTE.

All source code and other materials included in this release is provided as is
and is subject to the following:

&#40;c) 2013-21 Society of Motion Picture & Television Engineers LLC and Woodman Labs, Inc.<br>
All rights reserved--use subject to compliance with end user license agreement.


# Software Prerequisites

The following software tools are required to build and run the test materials:

C compiler and linker
: Required to build the sample encoder, reference decoder, and utility programs.

Tcl
: The codec test script is written in Tcl.
Some Tcl distributions do not include the cmdline package by default.
This package is required by the codec test script and is included in [Tcllib](http://wiki.tcl.tk/1246).

Python 3.8 or later
: Various scripts such as the program for downloading media from Amazon S3 are written in Python.

Git
: Used to obtain the software and updates from the SMPTE GitHub repository.

Make
: Used to build the sample encoder, reference decoder, and utility programs.

AWS Command Line Interface
: Used to configure credentials that grant access to the VC-5 media bucket on Amazon S3.


Optional software:

CMake
: Used to create build scripts for other build tools besides make (the default tool).
CMake is required to build the software on Windows since Visual Studio projects are no longer
distributed. CMake can build a Visual Studio solution file and projects that can be used to
build and run the software on Windows.


Doxygen
: Used to generate the software documentation.

Software documentation created using Doxygen is provided in the software distribution, so Doxygen is only required to
create new documentation if the distributed software is substantially modified.


## MacOS

Xcode includes the compiler, linker, make, and git and can be downloaded for free from the Apple developer web site.

The software can be built using the command-line tools, Xcode is not required but may be useful for editing and debugging
on macOS.

It is strongly recommended that Python, Doxygen, CMake and other command-line tools be installed using
[Homebrew](https://brew.sh/) whenever the software is available through Homebrew.
Tcl can be installed using the free community distribution provided by ActiveState:
[ActiveTcl](https://www.activestate.com/activetcl).

The Python packages by this software distribution are listed
in the `requirements.txt` file included in this distribution in the `$(ROOT)/scripts` directory.
Use the following command to install all Python packages required by this software distribution:
```bash
pip install -r requirements.txt
```


## Windows

Visual Studio Community is available for free from Microsoft.

Tcl and Python can be installed using the community editions provided by ActiveState:
[ActiveTcl](https://www.activestate.com/activetcl)
and
[ActivePython](https://www.activestate.com/activepython).
These are full-featured distributions, so it should not be necessary to install additional Python or Tcl packages.

CMake can be installed from the web site: <https://cmake.org/download/>


## Linux

Build tools can be installed using the preferred package manager for the specific Linux distribution.


# Software Distribution

The source code distribution includes the following files and directory structure:

common/
: Source code that is common to both the sample encoder and reference decoder.

encoder/
: Source code for the sample encoder.

decoder/
: Source code for the reference decoder.

tables/
: Codebook used by the sample encoder and reference decoder.

external/
: Software developed by third parties that is used by the test materials.

converter/
: Source code for a C language program for converting between image file formats.

comparer/
: Source code for a C language program that can be used to compare the decoded image with
the image that was input to the encoder.  The comparer program is not currently used by
the test scripts.

scripts/
: Scripts written in Tcl for testing the sample encoder and reference decoder.

media/
: Sample images and reference bitstreams that are compliant with the VC-5 suite of standards.

Makefile
: Make file for building the software, including targets to download media and run the comprehensive test suite.

install.md
: Instructions for building the source code provided with the test materials in markdown format
that can be converted to HTML or PDF.

release.md
: Release notes in markdown format listing the changes in each version of the test materials.
The release notes can be converted to HTML or PDF.

In this document, the variable `ROOT` is the location of the installed software with the
directory structure as describe above.

In previous software releases, the external sub-directory redistributed third-party code for `getopt`,
which is used for parsing the command-line arguments,
and Standard C headers files for Boolean and integer data types that are omitted on some platforms.

In the current software release, the external directory contains scripts that clone the third-party
software packages into the external directory. The scripts build the cloned software if possible.
On macOS and linux systems, use the script `install.sh` and on Windows use the script `install.ps1`.
The file `pathtools.ps1` contains PowerShell functions for adding dynamic libraries to the executable path.


# Amazon Credentials

Prior to this release, the test media files are stored on Amazon S3 and downloaded using a script (see [Test Media](#Test-Media)).
The test media file are still available on Amazon S3 but been checked into the GitHub repository using Git LFS and will be
downloaded automatically with the software distribution is cloned from GitHub.

Credentials are required to access the media files in the VC-5 bucket on Amazon S3.
A guest user account has been created on the Amazon account that hosts the VC-5 media bucket.
The guest user has read-only access sufficient to allow the media to be downloaded from S3.

The following steps are required to obtain access to the VC-5 media bucket:

1. Install the AWS Command Line Interface:
http://docs.aws.amazon.com/cli/latest/userguide/installing.html

2. Email the VC-5 Amazon account [administrator](mailto:admin@vc5codec.org) to obtain the guest credentials.
The administrator will reply with a CSV file that contains the credentials.

3. Run the following command:

```bash
aws configure --profile vc5codec-guest
AWS Access Key ID [None]: <Access key ID>
AWS Secret Access Key [None]: <Secret access key>
Default region name [None]:
Default output format [None]:
```

Replace `<Access key ID>` and `<Secret access key>` with the corresponding fields from the CSV file provided
by the VC-5 Amazon account administrator.

This command will create a directory tree in the user home directory granting guest access to the VC-5 media bucket.
It is important to install the credentials for the `vc5codec-guest` profile because the script for downloading
media files uses this profile by name to access Amazon S3.

For further information about credentials for Amazon Web Services see the web page on
[credentials](http://docs.aws.amazon.com/cli/latest/userguide/cli-chap-getting-started.html).


# Test Media

The test images and bitstreams used for conformance testing are included in the software distribution
or can be downloaded separately from an Amazon S3 bucket.  The `getmedia.py`script is provided in the software
distribution to handle all of the details for downloading the test media from the S3 bucket.
The script checks whether a file exists on the local hard drive already and if the checksums of the local
and remote files are identical, then the file is not downloaded again.

To run the script for downloading media, the user must obtain Amazon credentials from the VC-5 Amazon account administrator
(see [Amazon Credentials](#Amazon-Credentials)).

To install the test media, invoke the following command from the root directory of the software
distribution:
```bash
make media
```

The preferred method for obtaining the test media is to clone the software distribution from GitHub.
By default, the test media will be downloaded when the repository is cloned.


# Documentation

Doxygen can be used to create documentation for the encoder and decoder by running the command
```bash
make docs
```
in the root directory of the software installation.

The documentation is in HTML format and is located in the `$(ROOT)/encoder/docs/html` and
`$(ROOT)/decoder/docs/html` sub-directories.  The root of the document tree is `index.html`
in the respective sub-directories.


# Modifying the Source Code


## Key Parameters

Key compile-time parameters are in the file `$(ROOT)/common/include/config.h`, including:

- Limits that control the maximum sizes of data structures
- VC-5 parts that are enabled at compile-time by default


## Implicitly Defined Parts

Some parts of the VC-5 standard are enabled implicity if other parts are enabled.
For example, VC-5 Part 3 Image Formats is enabled if VC-5 Part 6 Sections is enabled.
This behavior can be modified by changing the table `enabled_parts_list` in the routine
`CheckEnabledParts()` in the file `$(ROOT)/common/src/utilities.c` in the codec software
distribution.


# Building the Encoder and Decoder

The encoder and decoder have been built and tested on macOS Catalina using both make files and CMake,
with the Xcode command-line tools.


## MacOS

The Xcode projects are not currently included in the software distribution.
The CMake build script can be used to create Xcode projects for building the encoder and decoder.
See [Xcode](#Xcode).

The encoder and decoder can be built by executing either of the following commands:
```bash
make TOOL=make
```
or
```bash
make TOOL=cmake
```
in the root directory of the installed software.

It may be necessary to run `cmake` explicitly to generate build files the first time that CMake is used.
For example, to run `cmake` for preparing to build the encoder:
```bash
cd $(ROOT)/encoder/build/cmake
mkdir build
cd build
cmake ..
```

It is good practice to create CMake build files in a subdirectory since CMake creates a large number
of files and directories and it is easy to remove all CMake files by deleting the `build` subdirector.

The encoder build files are placed in the `make` or `cmake` subdirectories of `$(ROOT)/encoder/build`,
depending on which tool was used to build the executable.
Likewise, the decoder build files are in the `make` or `cmake` subdirectories of `$(ROOT)/decoder/build`,
depending on which tool was used to build the executable.

The default tool is `make`, so the encoder and decoder can be built on macOS or Linux using the simpler command:
```bash
make
```
with no command-line arguments required.


Build products can be removed by executing the following command in the root directory:
```bash
make clean
```
or
```bash
make clean-all
```
to remove all build products.

The command
```bash
make TOOL=cmake clean
```
will remove the build products created by running the make files created by CMake.

The command
```bash
make TOOL=cmake clean-all
```
will remove the build products created by running the make files created by CMake
and all of the build files created by running CMake.


## Linux

Although Linux is not used routinely for building and testing the software, either of the methods
described above (make files or CMake) should work on Linux and other Unix systems.


## Windows

The software is not built and tested on Windows routinely, but it should be possible
to build the software using Visual Studio solution and project files created by CMake (see [CMake](#CMake)).


## CMake

CMake has been used to build the encoder and decoder using make (the default CMake generator).

It is expected that CMake will correctly generate build files on all platforms and for all build tools
supported by CMake such as Xcode, Visual Studio, and Eclipse, but not all platforms and tools
have been tested.
Submit a bug report for any combination of platform or build tool that CMake should support,
but the build fails (see [Bugs](#Bugs)).

In the future, CMake will be used to create build projects for various tools and platforms.
Project files for specific platforms and tools (for example, Visual Studio project and solution files)
will not be included in the software distribution.

The recommended practice is to create a sub-directory in the cmake directory for a specific build tool
and create build files in that directory.
For example, to create Xcode projects for building the encoder:
```bash
cd $(ROOT)/encoder/build/cmake
mkdir xcode
cd xcode
cmake -G Xcode ..
```

See the CMake [documentation](https://cmake.org/cmake/help/v3.0/manual/cmake-generators.7.html) for information
about determining the generators that are available on a specific platform.


## Xcode

Xcode projects for the encoder and decoder can be created for macOS using CMake.
For example, to create an Xcode project for the decoder:
```bash
cd $(ROOT)/decoder/build
mkdir xcode
cd xcode
cp ../cmake/CMakeLists.txt .
cmake -G Xcode .
```

This will create an Xcode project that can be used to build and debug the decoder.


# Testing the Software

The encoder and decoder can be invoked from the command line with the arguments described in VC-5 Part 2,
but it may be more convenient to use the `testcodec.tcl` test script that is included in the distribution.

As described in VC-5 Part 2, the codec test script relies on a specific directory structure and filename
conventions.

All regression tests can be performed by executing the command
```bash
testcodec.tcl -a
```
in the directory `$(ROOT)/scripts`.
By default, the codec test script uses the release configuration of the encoder and decoder built using
the make tool.

For every image file in the `$(ROOT)/media` directory tree, the image file is encoded into a bitstream which
is then decoded into an image file.  The bitstream and decoded image are compared against the corresponding
master files using a simple binary file comparison.  Any difference in file size or content is reported as
a failure.

The codec test script accepts several command-line arguments that use different build products or limit
testing to a specific part of the VC-5 standards.  For example, the command
```bash
testcodec.tcl -p 3 -b debug -v
```
runs only the tests for VC-5 part 3 using the debug configuration of the encoder and decoder built using
the make tool and prints information about the tests to the terminal window.

The command
```bash
testcodec.tcl -p 5 -c cmake -l testcodec.log -v
```
runs only tests for VC-5 part 5 using the release configuration of the encoder and decoder built using
the cmake build tool and outputs information to both the terminal window and a logfile.

For convenience, the comprehensive codec test script can be run from the root directory using make:
```bash
make test
```

# Utility Programs

The converter and comparer utility programs, described in VC-5 Part 2, are included in the software distribution,
but have not been used or tested in some time.  It is expected that the utility programs will still work, as the
code as not been changed in some time.

In the future, CMake scripts will be provided to allow the utility programs to be built on any platform and
enable creation of projects for specific tools such as Xcode and Visual Studio, for example.


## Converter

The converter program was used to create image files in byr4 pixel format from DPX files.

The converter can be built using make files or CMake.
Buy default, CMake creates a project that uses the make tool, but CMake can be used to create other types
of projects.
For example, to create an Xcode project for the converter:
```bash
cd $(ROOT)/converter/build
mkdir xcode
cd xcode
cp ../cmake/CMakeLists.txt .
cmake -G Xcode .
```


## Comparer

The comparer program was intended to be used to compare decoded images with the images input to the encoder,
but this functionality has been superceded by the codec test script that performs a binary compare of
encoded bitstreams and decoded images with master files that have been checked for correctness.

The converter program was used to create image files in byr4 pixel format from DPX files.

The converter can be built using make files or CMake.
Buy default, CMake creates a project that uses the make tool, but CMake can be used to create other types
of projects.
For example, to create an Xcode project for the comparer:
```bash
cd $(ROOT)/comparer/build
mkdir xcode
cd xcode
cp ../cmake/CMakeLists.txt .
cmake -G Xcode .
```


# Known Problems


## Too Many Open Files

The encoder and decoder are passing all regression tests performed by running:
```bash
testcodec.tcl -a
```
but the user may see an error message about too many pipes.
To solve this problem, increase the limit on the number of open files:
```bash
ulimit -n 2048
```

# Contact Information

Please report bugs or suggestions for improvements by sending email to <mailto:bugs@vc5codec.org>.

Include in the bug report step by step instructions for reproducing the problem and attach any files
needed to reproduce the problem.  Describe the expected behavior and the behavior that was observed.

