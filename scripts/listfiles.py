#!/usr/binenv python
#
# List specified information for all files in a directory tree

import os


def get_file_types(args):
    """Return the set of file extensions that occur in the directory tree"""
    filetypes = set()

    for pathname in args.filelist:
        #print(pathname)
        for root, folder_list, file_list in os.walk(pathname):
            #print(file_list)
            for filename in file_list:
                #print(filename)

                (basename, extension) = os.path.splitext(filename)
                #print(extension)
                filetypes.add(extension[1:])

    return filetypes


def list_file_names(args):
    """List the file names in the directory tree"""

    for pathname in args.filelist:
        #print(pathname)
        for root, folder_list, file_list in os.walk(pathname):
            #print(file_list)
            for filename in file_list:
                print(filename)


if __name__ == '__main__':

    # Parse the command line arguments
    from argparse import ArgumentParser

    parser = ArgumentParser()
    parser.add_argument('filelist', nargs='*', help='list of directories to walk')
    parser.add_argument('-t', '--types', action='store_true', help='iist file extensions only')
    parser.add_argument('-v', '--verbose', action='store_true', help='enable verbose output')
    args = parser.parse_args()

    #HOME = os.path.expanduser("~")

    if args.types:
        filetypes = get_file_types(args)
        #print(filetypes)
        for type in filetypes: print(type)
    else:
        list_file_names(args)

