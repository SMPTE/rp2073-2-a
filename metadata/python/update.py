#!/usr/bin/env python3
#
# Script to update the test cases with the working copies of the XML files

import os
import filecmp
import shutil


# Metadata categories (subdirectories)
categories = ['intrinsic', 'streaming', 'extrinsic', 'dark']


# Directories that contain metadata test cases
directories = ['xml']


# File extensions for metadata test cases
datatypes = ['xml']


def file_extension(pathname):
    """Return the file extension as a string without the file extension separator."""
    return os.path.splitext(pathname)[1][1:].lower()


if __name__ == '__main__':
    from argparse import ArgumentParser
    parser = ArgumentParser(description='Update metadata test cases from the files in the working directories')
    parser.add_argument('target', nargs=1, help='target location for updating the metadata test cases')
    parser.add_argument('-v', '--verbose', action='store_true', help='enable verbose output for monitoring progress')
    parser.add_argument('-z', '--debug', action='store_true', help='enable extra output for debugging')

    args = parser.parse_args()
    #if args.debug: print(args)

    # Directory tree with metadata test cases to copy to the target location
    source = os.getcwd()

    for (directory, folders, filelist) in os.walk(source):

        # Skip files that are not in one of the directories containing metadata test cases
        path_list = directory.split(os.sep)
        if not path_list[-1] in directories: continue

        # Look for metadata test cases in this directory
        for filename in filelist:

            # The file extension signals whether the file is a metadata test case
            filetype = file_extension(filename)

            if filetype in datatypes:
                pathname = os.path.join(directory, filename)

                # Remove the root directory from the pathname
                working_name = '.' + pathname[len(source):]
                #if args.debug: print(working_name)

                path_list = working_name.split(os.sep)

                # Remove the directory for the type of metadata
                path_list = [d for d in path_list if d not in directories]
                #if args.debug: print(path_list)

                target_name = os.sep.join(path_list)
                #if args.debug: print(target_name)

                target_path = os.path.normpath(os.path.join(args.target[0], target_name))
                #if args.debug: print(target_path)

                target_directory = os.path.split(target_path)[0]
                if not os.path.exists(target_directory):
                    os.makedirs(target_directory)

                # Skip copying the files if the files are identical
                if os.path.exists(target_path):
                    if filecmp.cmp(working_name, target_path, shallow=False):
                        if args.debug: print("Skipping files: %s" % working_name)
                        continue

                if args.verbose: print("%s --> %s" % (working_name, target_path))

                shutil.copy(working_name, target_path)

