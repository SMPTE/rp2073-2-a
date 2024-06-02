---
title: |
	VC-5 Test Materials \
	Metadata Install and Build Instructions \
	Version {{version.major}}.{{version.minor}}.{{version.revision}}
mustache: ../../version.yaml
---


# Introduction

This document describes how to install, build, and test the metadata software that supports
ST 2073-7. The metadata software is itself a large software package so new documentation was
started.


# Terminology

In this document, the symbol `$(ROOT)` is the root directory of the VC-5 software installation.


# Dependencies

The metadata software makes use of following categories of prerequisites:
* Python
* Bash (on macOS and Linux)
* PowerShell (on Windows)
* Xcode commmand-line tools (on macOS)
* Visual Studio 2019 (on Windows)
* Gcc tool chain (on Linux)
* External software packages


## Python

All Python scripts are written for Python3.

Python is used heavily in the workflow for generating the metadata test cases.


## Bash

Many scripts for building and testing the metadata software on macOS and Linux.


## PowerShell

Many of the Bash scripts have corresponding PowerShell scripts for use on Windows.


## Build Tools


### Xcode Command-line Tools

Most of the software development on macOS was done using the Xcode command-line tools
and the Sublime Text editor. Make files are used to perform most builds on macOS but
the builds have been tested using CMake.


### Visual Studio

On Windows, CMake is used to create Visual Studio solution and project files. For example:
```ps1
cd $(ROOT)/metadata/parser/build/cmake
mkdir build
cmake -B build .
```
with the root directory replaced by the location of the VC-5 software.

The software can be built using Visual Studio or by running CMake:
```ps1
cmake --build ./build -A win32 --config Debug
```


### Gcc toolchain

Building on Linux is the same as on macOS.


## External Software

Libraries required for building the XML parser, XML dumper, and the tools in `$(ROOT)/metadata/tools/`.

1. [Expat](https://libexpat.github.io/)

2. [Getopt](https://www.gnu.org/software/libc/manual/html_node/Getopt.html)

3. [Argparse](https://github.com/cofyc/argparse)

4. [Mini-XML](https://www.msweet.org/mxml/)

Note that it is not necessary to build any of the software in the `$(ROOT)/metadata` tree.
The software is only used for generating test cases that are provided with the software distribution
and for prototyping the new code that adds supporting for metadata to the sample encoder and reference decoder.

The Mini-XML library can be built from source code available on [GitHub](https://github.com/michaelrsweet/mxml.git).
If the build displays the error message "fatal error C1083: Cannot open include file: 'stdio.h': No such file or directory",
then change the version of the Windows SDK to version 10 (was version 8.1).


# Build Environments

The macOS build has been tested on macOS Catalina version 10.15.7 and
the Linux build has been tested on Ubuntu Desktop 20.04.2.0 (amd64).

The Windows build has been tested on Windows 10 with the Visual Studio 2019 build tools.


## Linux and macOS

The XML parser and XML dumper can be built using make files or CMake.

In the `.../metadata` directory, execute the command:
```bash
make
```
to build the XML parser and XML dumper using `make` or execute the command:
```bash
make cmake
```
to build the XML parser and XML dumper using `cmake`.


## Windows

To build the XML parser on Windows,
go into the `.../parser/build/cmake` directory and execute the following commands:
```ps1
mkdir build
cd build
cmake -G "Visual Studio 15" -A win32 ..
cmake --build .
```

Likewise for the XML dumper on Windows,
go into the `.../dumper/build/cmake` directory and execute the following commands:
```ps1
mkdir build
cd build
cmake -G "Visual Studio 15" -A win32 ..
cmake --build .
```

The `CMakeLists.txt` file specifies linking against the static libraries but sometimes the build
uses the dynamic library. In `$(ROOT)/external`, and `addpath.ps1` module provides functions for
managing the executable path:
```ps1
. ./pathtools.ps1
Show-Path
Add-Path -Directory $(ROOT)\external\mxml\vcnet\Debug\Win32
```
with the root directory replaced by the location of the VC-5 software.

May have to add the absolute pathnames for the Expat libraries (debug and release configurations) to the
executable path using the `Add-Path` command provided in the file `$(ROOT)/external/path.ps1`. Execute the
following commands (once) to add the absolute pathnames to the path:
```ps1
Add-Path -Directory $(ROOT)\external\libexpat\expat\build\Release
Add-Path -Directory $(ROOT)\external\libexpat\expat\build\Debug
```


# Test Media

The test cases for metadata are in subdirectories of `$(ROOT)/media/metadata`. Each subdirectory
corresponds to a table in RP 2073-2 Annex D. See the README file for details.


# Metadata Testing

The metadata software uses the new test script in `$(ROOT)/scripts/testcodec.py` that can be
run with the command:
```bash
./testcodec.py
```
on macOS and Linux or
```ps1
py testcodec.py
```
on Windows.

