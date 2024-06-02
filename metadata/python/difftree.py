#!/usr/bin/env python3
#
# Compare all test cases in two directory trees

import sys
import os
import filecmp


# File extensions for metadata test cases
#metadata_filetype_list = ['json', 'xml', 'b64']
metadata_filetype_list = ['xml']


def file_extension(pathname):
    """Return the file extension as a string without the file extension separator."""
    return os.path.splitext(pathname)[1][1:].lower()


def first_pathname(directory, filename):
    """Return the pathname in the first directory tree."""
    if 'Archive' in directory.split(os.sep):
        return None
    else:
        return os.path.join(directory, filename)


def second_pathname(root1, pathname1, root2, edit_list=None):
    """Return the pathname in the second directory tree."""

    if sys.version_info > (3,9):
        suffix = pathname1.removeprefix(root1)
    else:
        if pathname1.startswith(root1):
            suffix = pathname1[len(root1):]
        else:
            suffix = pathname1

    # Force the relative pathname to not look like an absolute pathname
    if suffix[0] == os.sep: suffix = suffix[1:]

    #print("Suffix: %s" % suffix)

    if edit_list:
        (directory, filename) = os.path.split(suffix)
        root2_list = directory.split(os.sep)
        root2_list = [directory for directory in root2_list if not directory in edit_list]
        #print(root2_list)
        suffix = os.path.join(os.sep.join(root2_list + [filename]))

    return os.path.join(root2, suffix)


def output_verification_results(pathnames, format, output=None):
    """Output the verification results."""
    if output:
        output_line = format % tuple(pathnames)
        output.write(output_line + '\n')
    else:
        print(format % tuple(pathnames))


if __name__ == '__main__':
    from argparse import ArgumentParser
    parser = ArgumentParser(description='Compare the metadata test cases in two directory trees')
    parser.add_argument('root', nargs=2, help='root of each directory tree')
    parser.add_argument('-e' ,'--edit', help='remove subdirectories that do not occur in the right tree')
    parser.add_argument('-w', '--width', type=int, help='width of each column in the listing of differences')
    parser.add_argument('-o', '--output', help='output file for the verification results')
    parser.add_argument('-v', '--verbose', action='store_true', help='enable verbose output for monitoring progress')
    parser.add_argument('-z', '--debug', action='store_true', help='enable extra output for debugging')

    args = parser.parse_args()

    #if args.debug: print(args.root)

    if args.edit:
        edit_list = args.edit.split(',')
        #if args.debug: print(edit_list)
    else:
        edit_list = None

    # Initialize the list of files in the first and second trees
    filelist = []

    # Initialize the maximum length of all pathnames in either directory tree
    path_max = [0, 0]

    # Walk the first directory tree comparing files in corresponding folders in the second directory tree
    for directory, folder_list, file_list in os.walk(args.root[0]):
        for filename in file_list:
            filetype = file_extension(filename)
            if filetype in metadata_filetype_list:

                # Get the pathname for the file in the first directory tree
                pathname1 = first_pathname(directory, filename)
                #print(pathname1)
                if pathname1 == None: continue

                # Get the pathname for the file in the second directory tree
                pathname2 = second_pathname(args.root[0], pathname1, args.root[1], edit_list)
                #print(pathname2)
                if pathname2 == None: continue

                pathnames = [pathname1, pathname2]
                #if args.debug: print(pathnames)

                # Determine the maximum length of the pathnames in either directory tree
                for index in range(2):
                    length = len(pathnames[index])
                    path_max[index] = length if length > path_max[index] else path_max[index]

                # Add the pathname pair to the list of corresponding filenames
                filelist.append(pathnames)

    # Format for printing pairs of corresponding pathnames
    if args.width:
        format = "%%-%ds %%-%ds" % (args.width, args.width)
    else:
        #if args.debug: print("Pathname maximums: %s" % path_max)
        format = "%-" + str(path_max[0]) + "s %-" + str(path_max[1]) + "s"

    if args and args.output:
        output = open(args.output, 'w')
    else:
        output = None

    for pathnames in filelist:
        #if args.debug: print(pathnames)

        if not os.path.exists(pathnames[0]):
            pathnames[1] = 'MISSING'
            #print(format % tuple(pathnames))
            output_verification_results(pathnames, format, output)

        elif not os.path.exists(pathnames[1]):
            pathnames[0] = 'MISSING'
            #print(format % tuple(pathnames))
            output_verification_results(pathnames, format, output)

        else:
            # Compare the contents of the two files
            if not filecmp.cmp(pathnames[0], pathnames[1], shallow=False):
                #print(format % (pathnames[0], 'DIFFERENT'))
                pathnames[1] = 'DIFFERENT'
                output_verification_results(pathnames, format, output)

