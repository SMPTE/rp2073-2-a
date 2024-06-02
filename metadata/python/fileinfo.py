#!/usr/bin/env python3
#
# Obtain information about a file that is included in extrinsic metadata

import os
import sys
import json
import uuid
from datetime import datetime, timezone


# Default location of the media files
default_media_location = 'https://github.com/SMPTE/smpte-10e-vc5/blob/master/metadata/media'


def file_resource_identifier(pathname, location=None):
    """Return the uniform resource identifier (URI) for the file as specified in RFC 3986."""
    #return "file://" + os.path.abspath(pathname)

    if location:
        # Replace the directory path to the media file with the original directory path
        realpath = os.path.join(location, os.path.split(pathname)[1])

    else:
        # Replace the directory path to the media file with the default directory path
        realpath = os.path.join(default_media_location, os.path.split(pathname)[1])

    return realpath


def ISO_datetime(time, tz=timezone.utc):
    """Return the timestamp converted to date and time in ISO 8601 format."""
    return datetime.fromtimestamp(time, tz=tz).isoformat()


def file_creation_time(pathname):
    """Return the file creation date and time in ISO 8601 format."""
    time = os.path.getctime(pathname)
    return ISO_datetime(time, timezone.utc)


def file_modification_time(pathname):
    """Return the file modification date and time in ISO 8601 format."""
    time = os.path.getmtime(pathname)
    return ISO_datetime(time, timezone.utc)


def create_file_info_record(pathname, location=None):
    """Return a dictionary containing information about the file."""

    # Optional information for an extrinsic metadata class instance
    resource_identifier = file_resource_identifier(pathname, location)
    creation_time = file_creation_time(pathname)
    modification_time = file_modification_time(pathname)

    # The filename without the extension is the key for the file information
    file_info_key = os.path.splitext(os.path.split(pathname)[1])[0]

    # Create a unique identifer for the file (used for dark metadata test cases)
    file_info_uuid = uuid.uuid4().hex

    # Create a file information record
    file_info_record = {
        'PATH': resource_identifier,
        'FCDT': creation_time,
        'FMDT': modification_time,
        'UUID': file_info_uuid
    }
    #print(file_info_record)

    return (file_info_key, file_info_record)


def output_file_info(file_info, args=None):
    """Output the file information in JSON format."""

    if args and args.output:
        if os.path.isfile(args.output):
            with open(args.output) as input:
                file_info_table = json.load(input)

            # Merge the new file information with the existing file information
            for key, value in file_info.items():
                if not key in file_info_table:
                    file_info_table[key] = file_info[key]

            with open(args.output, 'w') as output:
                output.write(json.dumps(file_info_table, indent=2) + '\n')

        else:
            with open(args.output, 'w') as output:
                output.write(json.dumps(file_info, indent=2) + '\n')

    else:
        print(json.dumps(file_info, indent=2))


if __name__ == '__main__':
    from argparse import ArgumentParser
    parser = ArgumentParser(description='Generate a metadata file in JSON or XML format')
    parser.add_argument('filelist', nargs='*', help='list of input metadata files')
    parser.add_argument('-t', '--type', help='comma separated list of media file extensions')
    parser.add_argument('-l', '--location', help='location of the folder that contains the media files')
    parser.add_argument('-o', '--output', help='output file in XML format')
    parser.add_argument('-v', '--verbose', action='store_true', help='enable verbose output for monitoring progress')
    parser.add_argument('-z', '--debug', action='store_true', help='enable extra output for debugging')

    args = parser.parse_args()
    #print(args)

    # Set the location of the metadata media folder
    if args.location:
        location = args.location
    else:
        # Use the default location of the media files
        location = default_media_location

    # Initialize the table of file information records
    file_info_dict = {}

    for pathname in args.filelist:

        (file_info_key, file_info_record) = create_file_info_record(pathname, location)

        # Add the information for this file to the list of file information records
        file_info_dict[file_info_key] = file_info_record

    output_file_info(file_info_dict, args)

