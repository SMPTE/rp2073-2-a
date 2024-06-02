#!/usr/bin/env python3
#
# Create makefile rules for converting the XML representation of metadata test cases to binary

import os
import sys


# Program for creating a binary test case from its XML representation
program = '$(PARSER)'

# Command for creating a binary test case from its XML representation
command = '$(PARSER) -o $@ $<'


if __name__ == '__main__':
    from argparse import ArgumentParser
    parser = ArgumentParser(description='Create makefile rules for the binary representation of metadata test cases')
    parser.add_argument('filelist', nargs='+', help='Input files containing lists of metadata test cases')
    parser.add_argument('-o', '--output', help='Output file for the makefile rules')
    parser.add_argument('-d', '--directory', help='Output directory for the binary files')
    parser.add_argument('-s', '--input', help='Input directory for the test cases in XML format')
    parser.add_argument('-v', '--verbose', action='store_true', help='enable verbose output for monitoring progress')
    parser.add_argument('-z', '--debug', action='store_true', help='enable extra output for debugging')

    args = parser.parse_args()

    verbose = args.verbose
    debug = args.debug

    #if debug: print(args)
    #if debug: print(args.filelist)

    # Set the output file for the makefile rules
    output = open(args.output, 'w') if args.output else sys.stdout

    # Set the input directory for the metadata test cases in XML format
    input = args.input if args.input else "$(INPUT_DIR)"

    # Set the output directory for the makefile targets
    directory = args.directory if args.directory else '$(OUTPUT_DIR)'

    # Output the file header line
    output.write("# AUTOMATICALLY GENERATED FILE -- DO NOT EDIT\n")

    for pathname in args.filelist:
        #if verbose: print("Processing test case list: {file}".format(file=pathname))

        with open(pathname, 'r') as file:

            # Create a makefile rule for each test case in the file
            for testcase in file:
                source = testcase.strip()
                #target_path = os.path.split(source)[0]

                # Replace the subdirectory for the output files
                output_list = os.path.split(source)[0].split(os.sep)
                output_path = ''
                for folder in output_list:
                    if folder == 'xml': folder = 'bin'
                    output_path += (os.sep if len(output_path) > 0 else '') + folder

                source = os.path.join(input, source)
                if debug: print(source)

                target = os.path.splitext(os.path.split(source)[1])[0] + '.bin'
                target = os.path.join(output_path, target)
                if debug: print(target)

                #print("{target}: {source}".format(target=target, source=source))
                output.write("\n{directory}/{target}: {source} {program}\n".format(directory=directory, target=target, source=source, program=program))
                output.write("\t{command}\n".format(command=command))

