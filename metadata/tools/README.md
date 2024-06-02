# Metadata Tools

This directory contains the source code for tools used to create metadata test cases.


## bintool

Program to convert binary files to base64 and records the size of the binary file
in the `filesize.csv` file in the root directory for the category of metadata.
The size of the binary blob is included in the metadata test case so that the size
of the binary blob, after decoding from the base64 representation, is known.


## dpxdump

Program to dump the header of a DPX file in base64 format.


## exrdump

Program to dump the ACES metadata from an OpenEXR file.


## klvdump

Program for dumping a file of SMPTE KLV tuples. Useful for creating a human-readable representation
of the KLV tuples in dynamic metadata color volume transforms (DMCVT) extracted from MXF files.


## include

Common header files used by the metadata tools.


## Makefile

Make file for building all of the metadata tools. Each metadata has its own make file that includes
the file `common.mk` that provides common definitions and rules used for building all of the tools.


## CMakeLists.txt

CMake build settings for all of the metadata tools.

