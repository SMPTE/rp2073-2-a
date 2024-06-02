#!/usr/bin/env python
#
# Download test materials (media files) from the S3 bucket used for the VC-5 codec
#
# (c) 2013-2017 Society of Motion Picture & Television Engineers LLC and Woodman Labs, Inc.
# All rights reserved--use subject to compliance with end user license agreement.

import boto3
import hashlib
import os
import json
import errno

# Additional packages required for guest access
from botocore.client import Config
from botocore.exceptions import ClientError


class MediaError(Exception):
    """Define a class for media upload errors"""
    def __init__(self, value):
        self.value = value

    def __str__(self):
        return repr(self.value)


def compute_checksum(pathname):
    sha1 = hashlib.sha1()
    block_size = (1 << 16)

    with open(pathname, 'rb') as file:
        while True:
            data = file.read(block_size)
            if not data: break
            sha1.update(data)

    return sha1.hexdigest()


def media_object_checksum(media_object):
    """Get the user-defined metadata associated with the media object"""
    metadata = media_object.get()['Metadata']
    checksum = metadata.get('checksum', "")
    return checksum


def media_file_location(key_name, media):
    """Translate an object key into a media file pathname"""
    pathlist = key_name.split('/')
    if pathlist[0] == 'media':
        #pathname = apply(os.path.join, [media] + pathlist[1:])
        pathname = os.path.join(*[media] + pathlist[1:])
        return pathname
    else:
        raise MediaError("Bad key name syntax: %s" % key_name)


def is_media_file(key_name):
    """Return true if this object key corresponds to a media file"""
    pathlist = os.path.split(key_name)
    return pathlist[1] != ''


def create_pathname_directory(pathname):
    directory = os.path.split(pathname)[0]
    #print directory
    try:
        os.makedirs(directory)
    except OSError as error:
        if not (error.errno == errno.EEXIST and os.path.isdir(directory)):
            raise


def download_file(pathname, key_name, media_checksum):
    if os.path.isfile(pathname):
        local_checksum = compute_checksum(pathname)
        #print "%s\n%s\n%s\n%s\n" % (pathname, key_name, media_checksum, local_checksum)
        if local_checksum == media_checksum:
            if debug: print("Skipping file: %s" % pathname)
            return

    #print("Bucket object: %s" % key_name)
    create_pathname_directory(pathname)

    if verbose: print("Fetching file: %s" % pathname)
    bucket.download_file(key_name, pathname)


def download_media_file(key_name):
    if debug: print("Media file: %s" % key_name)
    media_object = bucket.Object(key_name)
    media_checksum = media_object_checksum(media_object)

    pathname = media_file_location(key_name, media)
    download_file(pathname, key_name, media_checksum)


def download_media_folder(folder_key):
    if debug: print("Media folder: %s" % folder_key)
    directory = media_file_location(folder_key, media)
    #print directory

    try:
        for media_object in bucket.objects.filter(Prefix=folder_key):
            key_name = media_object.key
            if debug: print("Media object key: %s" % key_name)
            if is_media_file(key_name):
                media_checksum = media_object_checksum(media_object)
                pathname = media_file_location(key_name, media)

                if debug: print("Downfile folder file: %s" % pathname)
                download_file(pathname, key_name, media_checksum)
    except ClientError as e:
        print(e)


if __name__ == '__main__':

    # Parse the command line arguments
    from argparse import ArgumentParser

    parser = ArgumentParser()
    parser.add_argument('-m', '--media', help='folder that contains the downloaded media')
    parser.add_argument('-f', '--filelist', help="download the media files listed in this JSON file")
    parser.add_argument('-p', '--profile', default='vc5codec-guest', help='profile in the credentials file')
    parser.add_argument('-d', '--dryrun', action='store_true', help='disable uploading media files (for debugging)')
    parser.add_argument('-v', '--verbose', action='store_true', help='enable verbose output for monitoring progress')
    parser.add_argument('-e', '--debug', action='store_true', help='enable more verbose output (for debugging)')
    args = parser.parse_args()

    # Set global flags based on the command-line arguments
    dryrun = args.dryrun
    verbose = args.verbose
    debug = args.debug

    HOME = os.path.expanduser("~")

    # Set the directory tree that contains the media files to upload
    if args.media:
        media = args.media
    else:
        #media = os.path.join(HOME, 'Projects/GoPro/Cedoc/gopro-sdk-cedoc/VC-5/media')
        media = os.path.normpath(os.path.join(os.path.dirname(os.path.realpath(__file__)), '../media'))

    if debug:
    	print("Media folder: %s" % media)

    if args.filelist:
        import json
        with open(args.filelist) as input_file:
            media_list = json.load(input_file)

    # Create a session for the specified profile
    #s3 = boto3.resource('s3')
    session = boto3.Session(profile_name=args.profile)
    if debug:
        print("Session profile: %s" % session.profile_name)

    # Obtain access to the S3 resource
    s3 = session.resource('s3', config=Config(signature_version='s3v4'))

    # Get the S3 bucket that contains the media files
    bucket = s3.Bucket('vc5codec')
    if debug:
        print("Processing bucket: %s" % bucket.name)

    if args.filelist:
        # Download all media files in the list of files and folders
        for item in media_list:
            if item.has_key('file'):
                download_media_file(item['file'])
            elif item.has_key('folder'):
                download_media_folder(item['folder'])
            else:
                raise MediaError("Unknown item key: %s" % item.key)
    else:
        # Download all media files in the bucket
        for media_object in bucket.objects.filter(Prefix='media'):
            key_name = media_object.key

            # Is the object a pathname?
            if is_media_file(key_name):

                # Get the user-defined metadata associated with the media object
                checksum = media_object_checksum(media_object)

                pathname = media_file_location(key_name, media)
                #print pathname

                download_file(pathname, key_name, checksum)
