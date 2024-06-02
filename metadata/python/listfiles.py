#!/usr/bin/env python3
#
# Script to list metadata test cases

import os
import csv


# Metadata categories (subdirectories)
#categories = ['intrinsic', 'simple', 'nested', 'complex', 'streaming', 'extrinsic', 'dark']
categories = ['simple', 'nested', 'complex', 'streaming', 'extrinsic', 'dark', 'multiclass']


# File extensions for metadata test cases
datatypes = ['xml']


# Output column headings
headings = ['category', 'testcase']


def file_extension(pathname):
    """Return the file extension as a string without the file extension separator."""
    return os.path.splitext(pathname)[1][1:].lower()


if __name__ == '__main__':
    from argparse import ArgumentParser
    parser = ArgumentParser(description='List metadata test cases in the specified directory trees')
    parser.add_argument('roots', nargs='*', help='root directories to search for metadata test cases')
    parser.add_argument('-o', '--output', help='output file for the test case listing in CSV format')
    parser.add_argument('-v', '--verbose', action='store_true', help='enable verbose output for monitoring progress')
    parser.add_argument('-z', '--debug', action='store_true', help='enable extra output for debugging')

    args = parser.parse_args()
    #if args.debug: print(args)

    # Files are listed relative to the current directory
    cwd = os.getcwd()

    # Initialize the list of metadata test case locations and fienames
    testcases = []

    for root in args.roots:

        #if args.debug: print(root)

        for (directory, folders, filelist) in os.walk(root):

            if args.debug: print(directory)

            # Skip files that are not in one of the directories containing metadata test cases
            #path_list = directory.split(os.sep)
            #if not path_list[-1] in categories: continue
            #if 'Archive' in path_list: continue

            # Look for metadata test cases in this directory
            for filename in filelist:

                # The file extension signals whether the file is a metadata test case
                filetype = file_extension(filename)

                if filetype in datatypes:
                    pathname = os.path.abspath(os.path.join(directory, filename))
                    #if args.debug: print(pathname)

                    # Remove the current directory from the pathname
                    #short_name = '.' + pathname[len(cwd):]

                    #path_list = short_name.split(os.sep)
                    pathlist = pathname.split(os.sep)
                    if args.debug: print(pathlist)

                    # Find the category of metadata
                    category = None
                    for folder in reversed(pathlist):
                        if folder in categories:
                            category = folder
                            break

                    if category != None:
                        testcase = filename
                        testcases.append((category, filename))

    if args.output:
        with open(args.output, 'w', newline='') as output:
            writer = csv.writer(output)
            writer.writerow(headings)

            for (category, testcase) in testcases:
                writer.writerow((category, testcase))

    else:
        #print(testcases)
        for (category, testcase) in testcases:
            print(category, testcase)

