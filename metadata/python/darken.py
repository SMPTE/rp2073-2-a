#!/usr/bin/env python3
#
# Script to create dark metadata samples encoded in base64

import os
import csv
import base64


# Column headings for the file size information
headings = ['filename', 'size']


def file_extension(pathname):
    """Return the file extension as a string without the file extension separator."""
    return os.path.splitext(pathname)[1][1:].lower()


def output_dark_metadata(string, args):
    """Output dark metadata in base64 format."""

    buffer = base64.b64encode(bytearray(string, 'ascii'))

    if args.output:
        with open(args.output, 'w') as output:
            output.write(buffer.decode('ascii') + '\n')
    else:
        # NOTE: Printing the header to the standard output adds a newline
        print(buffer.decode("ascii"))

    if args.filesize and args.output:

        # Need to write the CSV file column headings?
        header_flag =  not os.path.isfile(args.filesize) or os.path.getsize(args.filesize) == 0

        with open(args.filesize, 'a', newline='') as filesize:
            writer = csv.writer(filesize)

            if header_flag:
                writer.writerow(headings)

            filename = os.path.splitext(os.path.split(args.output)[1])[0]

            writer.writerow([filename, len(string)])


if __name__ == '__main__':

    from argparse import ArgumentParser
    parser = ArgumentParser(description='Create dark metadata samples encoded in base64')
    parser.add_argument('-s', '--string', help='string to be encoded as dark metadata')
    parser.add_argument('-f', '--filesize', help='output file for information about the dark metadata')
    parser.add_argument('-o', '--output', help='output file in base64 format')
    parser.add_argument('-v', '--verbose', action='store_true', help='enable verbose output for monitoring progress')
    parser.add_argument('-z', '--debug', action='store_true', help='enable extra output for debugging')

    args = parser.parse_args()
    #print(args)


    if args.string:
        string = args.string
    else:
        string = "Default dark metadata string"

    output_dark_metadata(string, args)

