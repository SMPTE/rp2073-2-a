#!/usr/bin/env python3
#
# Return the list of output pathnames from the input files and location of the output

import os
import sys


if __name__ == '__main__':
    from argparse import ArgumentParser
    parser = ArgumentParser(description='Return the list of output pathnames corresponding to the input files')
    parser.add_argument('filelist', nargs='+', help='Input files containing lists of metadata test cases')
    #parser.add_argument('-d', '--directory', help='Root of the directory tree for the output files')
    parser.add_argument('-v', '--verbose', action='store_true', help='enable verbose output for monitoring progress')
    parser.add_argument('-z', '--debug', action='store_true', help='enable extra output for debugging')

    args = parser.parse_args()

    verbose = args.verbose
    debug = args.debug

    #if debug: print(args)
    #if debug: print(args.filelist)

    # Initialize the list of output files
    output_file_list = []

    # Set the location of the output files
    #directory = args.directory if args.directory else '$(OUTPUT_DIR)'

    for pathname in args.filelist:
        #if verbose: print("Processing input file: {file}".format(file=pathname))

        # Form the name of the output file (without the output path)
        filename = os.path.split(pathname)[1]
        basename = os.path.splitext(filename)[0]
        filename = basename + '.xml'

        # Replace the subdirectory for the output files
        output_list = os.path.split(pathname)[0].split(os.sep)
        output_path = ''
        for folder in output_list:
            if folder == 'bin': folder = 'xml'
            output_path += (os.sep if len(output_path) > 0 else '') + folder

        # Add the output path onto the output filename
        #output_path = os.path.split(pathname)[0];
        output_file = os.path.join(output_path, filename)

        output_file_list.append(output_file)

    if debug:
        print('\n'.join(output_file_list))
    else:
        print(' '.join(output_file_list))

