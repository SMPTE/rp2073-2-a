#!/usr/bin/env python3
#
# Deploy the metadata test cases into the media subdirectories specified in RP 2073-2 Annex D.

#import sys
import os
from glob import glob
import shutil
#import common

# Default root directory for the software distribution
root_dir = os.path.normpath('../..')

# Default root directory for the deployed metadata files
media_dir = os.path.normpath(os.path.join(root_dir, 'media', 'metadata'))

# Global switches for verbose and debug output
verbose = False
debug = False


def file_extension(pathname):
    """Return the file extension as a string without the file extension separator."""
    return os.path.splitext(pathname)[1][1:].lower()


def deploy_input(source, target):
    """Deploy the XML metadata files input to the sample encoder."""
    #global root_dir

    #print("Source: $(ROOT)/{0}\nTarget: $(ROOT)/{1}".format(os.path.relpath(source, root_dir), os.path.relpath(target, root_dir)))

    # Walk the directory tree of test cases and copy each test case in XML format
    subdirectory_list = ['intrinsic/simple', 'intrinsic/complex', 'streaming', 'extrinsic', 'dark', 'multiclass']

    testcase_list = []

    for subdirectory in subdirectory_list:
        testcase_directory = os.path.normpath(os.path.join(source, subdirectory))
        for directory, folder_list, file_list in os.walk(testcase_directory):
            #print(directory, folder_list, file_list)
            for file in file_list:
                filetype = file_extension(file)
                if filetype == 'xml':
                    pathname = os.path.normpath(os.path.join(directory, file))
                    #print(pathname)
                    testcase_list.append(pathname)

    #print(testcase_list)

    # Copy the files to the target directory without subdirectory paths

    for source_pathname in testcase_list:
        #filename = os.path.split(pathname)[1]
        #if debug: print(source_pathname, filename, target)

        target_pathname = os.path.join(target, os.path.split(source_pathname)[1])

        if debug:
            indent = ' ' * 4
            print("Source: {source}\n{indent}Target: {target}".format(indent=indent, source=source_pathname, target=target_pathname))

        if not os.path.isfile(target_pathname) or os.stat(source_pathname).st_mtime > os.stat(target_pathname).st_mtime:
            # Copy the source file to the target directory preserving file metadata
            shutil.copy2(source_pathname, target)
            if verbose: print(f'Copied newer input: {source_pathname}')
        else:
            if verbose: print(f'Skipped older file: {source_pathname}')

    return True


def deploy_bitstreams(source, target):
    """Deploy the encoded bitstreams containing embedded metadata."""
    #global root_dir

    #print("Source: $(ROOT)/{0}\nTarget: $(ROOT)/{1}".format(os.path.relpath(source, root_dir), os.path.relpath(target, root_dir)))

    for source_pathname in glob(os.path.join(source, '*.vc5')):
        #filename = os.path.split(source_pathname)[1]
        #if debug: print(source_pathname, filename, target)

        target_pathname = os.path.join(target, os.path.split(source_pathname)[1])

        if debug:
            indent = ' ' * 4
            print("Source: {source}\n{indent}Target: {target}".format(indent=indent, source=source_pathname, target=target_pathname))

        if not os.path.isfile(target_pathname) or os.stat(source_pathname).st_mtime > os.stat(target_pathname).st_mtime:
            # Copy the source file to the target directory preserving file metadata
            shutil.copy2(source_pathname, target)
            if verbose: print(f'Copied newer input: {source_pathname}')
        else:
            if verbose: print(f'Skipped older file: {source_pathname}')

    return True


def deploy_encoded(source, target):
    """Deploy the binary metadata extracted from the encoded bitstreams."""
    #global root_dir

    #print("Source: $(ROOT)/{0}\nTarget: $(ROOT)/{1}".format(os.path.relpath(source, root_dir), os.path.relpath(target, root_dir)))

    for source_pathname in glob(os.path.join(source, '*.bin')):
        #filename = os.path.split(source_pathname)[1]
        #if debug: print(source_pathname, filename, target)

        target_pathname = os.path.join(target, os.path.split(source_pathname)[1])

        if debug:
            indent = ' ' * 4
            print("Source: {source}\n{indent}Target: {target}".format(indent=indent, source=source_pathname, target=target_pathname))

        if not os.path.isfile(target_pathname) or os.stat(source_pathname).st_mtime > os.stat(target_pathname).st_mtime:
            # Copy the source file to the target directory preserving file metadata
            shutil.copy2(source_pathname, target)
            if verbose: print(f'Copied newer input: {source_pathname}')
        else:
            if verbose: print(f'Skipped older file: {source_pathname}')

    return True


def deploy_decoded(source, target):
    """Deploy the XML metadata output by the reference decoder."""
    #global root_dir

    #print("Source: $(ROOT)/{0}\nTarget: $(ROOT)/{1}".format(os.path.relpath(source, root_dir), os.path.relpath(target, root_dir)))

    for source_pathname in glob(os.path.join(source, '*.xml')):
        #filename = os.path.split(source_pathname)[1]
        #if debug: print(source_pathname, filename, target)

        target_pathname = os.path.join(target, os.path.split(source_pathname)[1])

        if debug:
            indent = ' ' * 4
            print("Source: {source}\n{indent}Target: {target}".format(indent=indent, source=source_pathname, target=target_pathname))

        if not os.path.isfile(target_pathname) or os.stat(source_pathname).st_mtime > os.stat(target_pathname).st_mtime:
            # Copy the source file to the target directory preserving file metadata
            shutil.copy2(source_pathname, target)
            if verbose: print(f'Copied newer input: {source_pathname}')
        else:
            if verbose: print(f'Skipped older file: {source_pathname}')

    return True


# Dictionary of handlers for deploying each table of metadata in RP 2073-2 Annex D
metadata_table_dict = {
    'input'     : {'handler': deploy_input, 'source': 'metadata/python/testcases'},
    'bitstreams': {'handler': deploy_bitstreams, 'source': 'encoder/metadata/bitstreams'},
    'encoded'   : {'handler': deploy_encoded, 'source': 'encoder/metadata/encoded'},
    'decoded'   : {'handler': deploy_decoded, 'source': 'decoder/metadata/decoded'}
}


if __name__ == '__main__':
    from argparse import ArgumentParser
    parser = ArgumentParser(description='Deploy the metadata testcases')
    parser.add_argument('-r', '--root', help='root directory for the software distribution')
    parser.add_argument('-m', '--media', help='root directory for the deployed metadata files')
    parser.add_argument('-v', '--verbose', action='store_true', help='enable verbose output for monitoring progress')
    parser.add_argument('-z', '--debug', action='store_true', help='enable extra output for debugging')

    args = parser.parse_args()
    #print(args)

    verbose = args.verbose
    debug = args.debug

    if args.root: root_dir = args.root
    if args.media: media_dir = args.media

    root_dir = os.path.abspath(root_dir)
    media_dir = os.path.abspath(media_dir)

    if debug: print(f'Root: {root_dir}\nMedia: {media_dir}')

    #TODO: Pass the flags and directories to the handlers as dictionary arguments

    for table in metadata_table_dict.keys():

        if verbose:
            print(f'Deploying metadata test cases: {table}')

        # Get the table entry that specifies how to deploy this metadata table
        metadata_table_entry = metadata_table_dict[table]

        # Get the location of the files to be deployed
        source = metadata_table_entry['source']
        source = os.path.join(root_dir, source)
        #print(source)

        # Compute the location for deploying the files
        target = os.path.join(media_dir, table)
        #print(target)

        if not os.path.isdir(target):
            print(f'Creating directory: {target}')
            os.path.makedirs(target)

        # Get the handler for deploying the files
        handler = metadata_table_entry['handler']

        # Deploy the files
        if handler(source, target):
            print(f'Deployed metadata test cases: {table}')
        else:
            print(f'Failure deploying test cases: {table}')

