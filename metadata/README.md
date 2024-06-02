# metadata

## Metadata Subdirectories

This directory contains the software tools and data files used to create test cases
for verifying conformance of decoder and encoder implementations to ST 2073-7.


[python/](./python)
> Python scripts for generating and managing test cases.
See the [README](./python/README.md) file in the python subdirectory for details.

[parser/](./parser)
> The XML parser reads test cases in XML format and outputs a binary representation of metadata
as specified in ST 2073-7. The XML parser is used for testing metadata code before integration
into the sample encoder.

[dumper/](./dumper)
> The XML dumper reads the binary files output by the XML parser and outputs the metadata in XML format.
The XML dumper is used for testing metadata code before integration into the reference decoder.

[tools/](./tools)
> Programs for working with metadata.
See the [README](./tools/README.md) file in the tools subdirectory for details.

[media/](./media)
> Sample files for each class of extrinsic metadata.
The files are not checked into this repository if the files are available in another repository.

[docs/](./docs)
> Documentation that describes the metadata test cases and the design of the XML parser.

[notes/](./notes)
> Logbook that records work on the metadata software, notes describing the test cases, and
instructions for building the metadata software.

[xml/](./xml)
> Sample XML files and XML schema for the representation of metadata defined in ST 2073-7 Annex A.


## Test Case Media Files

The media subdirectory contains the original files used to create the metadata test cases and
is subdivided into individual subdirectories for each metadata class:

[xmp](./media/xmp)
> XMP files extracted from the JPEG files distributed in the Adobe SDK using Exiftool.

[dpx](./media/dpx)
> DPX files from the HDR DPX [reference code/](https://github.com/SMPTE/smpte-31fs-hdrdpx).

[mxf](./media/mxf)
> MXF files.

[aces](./media/aces)
> ACES test data extracted from an MXF file.


## Streaming Data

The sample files used to obtain data for streaming data are included in the [GPMF](https://github.com/gopro/gpmf-parser.git) repository.
The GPMF parser was modified to extract streaming metadata from MP4 files and output the metadata to the `$(ROOT)/metadata/python/GPMF` directory
in CSV format. The `generate.py` script processes the CSV files into metadata in JSON and XML formats.
See the [README](./python/README.md) file in `$(ROOT)/metadata/python` for further details.

