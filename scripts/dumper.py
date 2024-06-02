#!/usr/bin/env python3
#
# Output a human-readable representation of a VC-5 bitstream

import os
from pathlib import Path
from struct import unpack

def file_size(pathname):
    """Return the size of the file in bytes."""
    if os.path.isfile(pathname):
        return Path(pathname).stat().st_size
    else:
        return 0


# Dictionary that maps pixel format number into pixel format string
pixel_format_string = {
    120: "RG48",
    121: "B64A"
}


# Dictionary that maps tag numbers to chunk types
chunk_type_dict = {
    0x60   : 'codeblock',
    0x61   : 'metadata',
    0x4010 : 'metadata'
}


def chunk_type(tag):
    """Convert the tag number to a string for the chunk type."""
    return chunk_type_dict.get(tag, None)


def parse_tuple_header(segment):
    """Parse the segment into the tuple tag and value."""

    #print(segment)
    #print("%02X %02X %02X %02X" % tuple(segment))

    # Split the segment into a tag and value
    #tag = int(segment[0] << 8 | segment[1])
    (tag, value) = unpack('>hH', segment)
    #print("%d (%04X)" % (tag, tag))

    # Is this an optional tag-value pair?
    if tag < 0:
        #print("Negating tag: %d" % tag)
        tag = -tag

    #print("%d (%04X)" % (tag, tag))

    # Is this segment a small chunk header?
    if tag in [0x4000, 0x4001, 0x4002, 0x4003, 0x4004, 0x4010]:
        value = segment[2] << 8 | segment[3]

    # Is this segment a large chunk header?
    elif tag >> 8 in [0x60, 0x61]:
        value = (tag & 0xFF) << 16 | segment[2] << 8 | segment[3];
        tag = tag >> 8

    else:
        # Must be a tag-value pair for a codec state parameter
        value = segment[2] << 8 | segment[3]

    return (tag, value)


def parameter_name(tag):
    """Return the name of the parameter corresponding to the tag if known."""

    #print("Tag:", tag, "Type:", type(tag))

    tag_name_dict = {

        # ST 2072-1 Table B.2
         20: "ImageWidth",
         21: "ImageHeight",
        101: "BitsPerComponent",
         12: "ChannelCount",
         14: "SubbandCount",
         62: "ChannelNumber",
         48: "SubbandNumber",
         35: "LowpassPrecision",
         53: "Quantization",
        109: "PrescaleShift",
        104: "ChannelWidth",
        105: "ChannelHeight",

        # ST 2073-1 Table B.3
        0x60: "Codeblock",

        # ST 2072-3 Table A.1
        106: "PatternWidth",
        107: "PatternHeight",
        108: "ComponentsPerSample",
         84: "ImageFormat",
        102: "MaxBitsPerComponent",

        # ST 2073-3 Table B.1
        0x4000: "VendorSpecificData",
        0x4001: "InversePermutation",
        0x4002: "InverseTransform",
        0x4003: "InverseTransform16",
        0x4004: "UniqueImageIdentifier",

        # ST 2073-7 Table 6
        0x4010: "SmallMetadataChunk",
        0x61: "LargeMetadataChunk",

        # Optional tags for debugging
        1001: "PixelFormat"
    }

    # Return the tuple name or the tuple number as a string
    return tag_name_dict.get(tag, "%04X (%d)" % (tag, tag))


if __name__ == '__main__':

    # Parse the command line arguments
    from argparse import ArgumentParser

    parser = ArgumentParser()
    parser.add_argument('filelist', nargs='*', help='bitstream files for extracting chunk elements')
    parser.add_argument('-c', '--chunk', choices=['codeblock', 'metadata'], help='type of chunks to extract from the bitstream')
    parser.add_argument('-o', '--output', help='output file for the chunks extracted from the bitstream')
    parser.add_argument('-v', '--verbose', action='store_true', help='enable verbose output')
    parser.add_argument('-z', '--debug', action='store_true', help='enable extra output for debugging')
    args = parser.parse_args()

    verbose = args.verbose
    debug = args.debug

    if args.output:
        # Open a file for writing chunks from the bitstream
        output = open(args.output, 'wb')
    else:
        output = None

    for pathname in args.filelist:
        if verbose: print(pathname)

        filesize = file_size(pathname)
        assert(filesize > 0)

        if debug: print("File size: {size:d}".format(size=filesize))

        with open(pathname, 'rb') as bitstream:
            # Read the bitstream start marker
            start_marker = bitstream.read(4)
            #print("Start marker:", start_marker.hex())
            if verbose: print("Start marker:", start_marker.decode('ASCII'))

            # Read the next tuple header in the bitstream
            segment = bitstream.read(4)
            filesize -= 4

            while len(segment) == 4:

                if debug: print("File size: {size:d}".format(size=filesize))

                (tag, value) = parse_tuple_header(segment)

                # Is this tag for a small chunk element?
                if tag in [0x4000, 0x4001, 0x4002, 0x4003, 0x4004, 0x4010]:
                    #print("Large chunk tag: {tag:d} (0x{tag:02X}), size: {size}".format(tag=tag, size=value))

                    # Convert the size from segments to bytes
                    size = value * 4

                    if verbose: print("Small chunk tag: {tag:d} (0x{tag:04X}), size: {size} bytes".format(tag=tag, size=size))

                    # Read the chunk payload
                    payload = bitstream.read(size)
                    filesize -= size

                    if output and chunk_type(tag) == args.chunk:
                        output.write(segment)
                        output.write(payload)

                elif tag in [0x60, 0x61]:
                    #print("Large chunk tag: {tag:d} (0x{tag:02X}), size: {size}".format(tag=tag, size=value))

                    # Convert the size from segments to bytes
                    size = value * 4

                    if verbose: print("large chunk tag: {tag:d} (0x{tag:04X}), size: {size} bytes".format(tag=tag, size=size))

                    # Read the chunk payload
                    payload = bitstream.read(size)
                    filesize -= size

                    if output and chunk_type(tag) == args.chunk:
                        output.write(segment)
                        output.write(payload)

                else:
                    #print("Tuple header:", segment.hex())
                    name = parameter_name(tag)
                    if name == "PrescaleShift":
                        shift = []
                        shift.append((value >> 14) & 0x03);
                        shift.append((value >> 12) & 0x03);
                        shift.append((value >> 10) & 0x03);
                        if verbose: print("{name}: {0} {1} {2}".format(*shift, name=name))
                    elif name == "PixelFormat":
                        string = pixel_format_string.get(value, "unknown")
                        if verbose: print("{name}: {string}".format(name=name, string=string))
                    else:
                        if verbose: print("{name}: {value}".format(name=name, value=value))

                # Read the next tuple header in the bitstream
                segment = bitstream.read(4)
                filesize -= 4

    # Close the output file for selected chunks
    if output:
        output.close()
        output = None

