#!/usr/bin/env python3
#
# Script written to output extrinsic metadata payloads in JSON files to a readable format.
# The output should be identical to the contents of the XMP file embedded in the metadata tuple.
#
# Extended to pretty print output from the XML dumper in a format that should be identical
# to the test cases created by the Python generator script and input the XML parser.

import sys
import json
from lxml import etree
from common import file_extension


def beautify_xmp(filename):
    """Pretty print the XMP payload in a JSON file."""

    metadata = []

    for filename in args.filelist:
        filetype = file_extension(filename)
        if filetype != 'json':
            #print("Input filetype must be JSON: {filename} {filetype}".format(filename=filename, filetype=filetype))
            print("Input filetype must be JSON: {filename}".format(filename=filename))
            exit(1)

        with open(filename) as file:
            metadata += json.load(file)

    for item in metadata:
        if item['tag'] == 'XMPD':
            for subitem in item['value']:
                if 'tag' in subitem and subitem['tag'] == 'XMPd':
                    for line in subitem['value'].split('\n'):
                        # Skip blank lines?
                        if args.skip and not line.strip():
                            continue

                        if args.directory:
                            exit("Output directory option not implemented")
                        else:
                            # Output the metadata value without an extra newline at the end
                            #print(subitem['value'], end = '')
                            print(line)


def beautify_xml(args):
    """ Pretty print XML in the same format as output by the Python generator script."""

    # Use an XML parser that preserves payloads in CDATA elements
    parser = etree.XMLParser(strip_cdata=False)

    for filename in args.filelist:
        filetype = file_extension(filename)
        if filetype != 'xml':
            #print("Input filetype must be XML: {filename} {filetype}".format(filename=filename, filetype=filetype))
            print("Input filetype must be XML: {filename}".format(filename=filename))
            exit(1)

        if False:
            with open(args.output, mode='w', encoding='utf-8') as output:
                etree.canonicalize(from_file=filename, out=output)
        else:
            # Read the XML file
            tree = etree.parse(filename, parser)

            # Output the XML file using the same code used by the test case generator
            # tree = etree.ElementTree(root)
            if args and args.output:
                # Output the XML to the specified file
                with open(args.output, 'wb') as output:
                    tree.write(output, xml_declaration=True, encoding='UTF-8', pretty_print=True)
            else:
                # Output the XML to the standard output
                output = sys.stdout.buffer if sys.version_info.major == 3 else sys.stdout
                tree.write(output, xml_declaration=True, encoding='UTF-8', pretty_print=True)
                #tree.write(output, xml_declaration=True, encoding='UTF-8', pretty_print=True, method="xml")


if __name__ == '__main__':
    from argparse import ArgumentParser
    parser = ArgumentParser(description='Output extrinsic metadata in a readable format')
    parser.add_argument('filelist', nargs='+', help='list of input metadata files')
    parser.add_argument('-x', '--xmp', action='store_true', help='beautify the XMP payload in a JSON file')
    parser.add_argument('-s', '--skip', action='store_true', help='do not output blank lines')
    parser.add_argument('-d', '--directory', help='write output to files in the specified directory')
    parser.add_argument('-o', '--output', help='output to the specified file')
    parser.add_argument('-v', '--verbose', action='store_true', help='enable verbose output for monitoring progress')

    args = parser.parse_args()
    #print(args)

    if args.xmp:
        beautify_xmp(args)
    else:
        beautify_xml(args)

