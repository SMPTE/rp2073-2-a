# Tools and test data for generating metadata test cases

## Metadata

This directory contains working copies of the metadata test cases that are not checked into the repository.

The command `make update` copies new and changed files to the test cases in `./testcases` and the test cases
in the `./testcases` directory are checked into the repository and deployed to the `$(ROOT)/media/metadata`
using the command `make deploy` after careful review.


### Intrinsic Metadata

#### Simple Intrinsic Metadata

Most of the metadata items defined in ST 2073-7 Annex B can be represented by a single metadata tuple
with the value of the metadata item carried in the value attribute of the metadata tuple. These metadata
items are called simple metadata.

A collection of simple metadata can be represented by a CSV file with one line per metadata item.
The columns correspond to the metadata tag, type, size, count, and value.


#### Nested Intrinsic Metadata

Some intrinsic metadata items defined in ST 2073-7 Annex B must be represented using nested metadata tuples,
specifically the encoding curve metadata and layers metadata. Nested metadata items can be represented using
JSON notation or using a variation of the CSV file format that is unique to the `generate.py` script.

The metadata tags for the parent tuples are treated as special case. When the metadata generator script encounters
a parent tag, all of the following lines in the CSV file, up to the next parent tag or end of file, are children of
the parent metadata item. The `generate.py` script adds the children as child nodes in the JSON or XML metadata output.


#### Complex Intrinsic Metadata

The cross product of the simple intrinsic metadata tuples in `simple.json`, the nested encoding curve metadata,
and the nested layers metadata forms the complex intrinsic metadata test cases that combine simple intrinsic metadata
tuples with nested intrinsic metadata tuples.


### Working Metadata Test Cases

Working copies of the metadata test cases are in subdirectories organized by metadata class:
```bash
./intrinsic/simple
./intrinsic/nested
./intrinsic/complex
./streaming
./extrinsic/xmp
./extrinsic/dpx
./extrinsic/mxf
./extrinsic/aces
./extrinsic/ale
./extrinsic/dmcvt
./dark
./multiclass
```

## Workflow

Metadata can be entered manually into CSV files with custom annotations that mark the beginning and ending
of nested metadata tuple. The `generate.py` script can convert metadata in CSV format to JSON and XML.
The JSON format is easier to work with in code and is often used as an intermediate format between CSV and XML.

Streaming data is extracted from MP4 files into CSV files.

Special tools and methods are required to extract metadata from files that contain extrinsic metadata.

Dark metadata is created using the `darken.py` script.

Multiclass metadata is creaed by combining other classes of metadata in XML format.


## Files and Directories

[Archive](./Archive)
> Old files that will be removed from the repository

[pdoc](./pdoc)
> Documentation for the Python scripts created using `pdoc`.

[docs](./docs)
> Documentation for the Python scripts created using Sphinx.
This method for creating documentation is still under development.

[GPMF](./GPMF)
> Streaming data in CSV format extracted from the MP4 sample files included in the GPMF
[software distribution](https://github.com/gopro/gpmf-parser.git).

[Makefile](./Makefile)
> Make file for generating metadata test cases and documentation for the Python scripts.


## Scripts

### generate.py

Metadata in JSON format output by the `generate.py` script can be converted to XML by the script.

Python script that creates CSV and JSON files into test cases in the XML format defined by ST 2073-7 Annex A.

The script can take CSV files and output XML, but if the metadata has nested tuples better to convert the
specialized dialect of CSV used by `generate.py` into JSON and then convert the JSON into XML.

The script can merge JSON and XML files into a single file. Useful for building up a test cases in stages.


### xmpdump.py

Python script to dump extrinsic metadata values in a readable format.

For example, XMP metadata is displayed as a string with embedded '\n' and '\t' character strings
rather than the newlines and tabs. The `extrinsic.py` script outputs the metadata value, nicely
formatted and indented. The output should be identical to the contents of the XMP file embedded
in the metadata tuple.


### validate.py

Validate a test case in XML format against the XML schema.


### verify.py

Python script to verify that metadata test cases conform to the specifications in ST 2073-7.


### update.py

Update the test cases in the `testcases` subdirectory with the working copies.


### b64string.py

Convert a character string that represents binary data to base64.
The script was written to convert RGBALayout values that were incorrectly represented as character strings
to base64.


### beautify.py

Script written to output extrinsic metadata payloads in JSON files to a readable format.
The output should be identical to the contents of the XMP file embedded in the metadata tuple.

The script was extended to pretty print output from the XML dumper in a format that should be identical
to the test cases created by the `generaate.py` generator script and input the XML parser.


### common.py

Common functions used by the Python scripts for processing metadata.


### compare.py

Compare all test cases in two directory trees.

This script is used to compare the working copies of the test cases in the subdirectories of `$(ROOT)/medata/python`
with the test cases in the `$(ROOT)/medata/python/testcases` directory tree.


### darken.py

Script to create dark metadata samples encoded in base64.


### deploy.py

Script to deploy metadata test cases into the media subdirectories that correspond to the tables
specified in RP 2073-2 Annex D.


### dmcvt.py

Script to create DMCVT metadata test cases in JSON format by reading dynamic metadata color volume transforms
from binary files and converting the metadata to JSON which is then converted to the final XML format.


### fileinfo.py

Script for gathering file information such as the creation and modification times from the source files used
to create metadata test cases. Some metadata classes specify the optional inclusion of file information.
This script creates a database of file information that is queried when the test cases are output in XML format
by the `generate.py` script.


### listfiles.py

Script to list metadata test cases in XML format. The script can walk the directory trees that contain
the working copies of metadata test cases or the directory tree `$(ROOT)/metadata/python/testcases` that
contains the stable versions of the metadata test cases.


### testing.py

Script for running all `generate.py` commands used to create metadata test cases.
The script is intended to be run using the Python code [coverage](https://coverage.readthedocs.io/) tool
to determine what code is actually used to compute the metadata test cases.

The `generate.py` script can append the commands that it executes to a logfile that is used
by the `testing.py` script to run the same commmands with the coverage tool.


## Documentation

Documentation in HTML format can be created for the Python scripts by running `make docs`
from the python directory. The current method for creating documentation uses `pdoc` but
Sphinx might be used in the future.

