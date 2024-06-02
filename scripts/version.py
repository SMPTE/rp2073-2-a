#!/usr/bin/env python3
#
# Script to return the version number for inclusion in the source code and documentation

import os
import yaml
#import chevron


header_template = """/*! @file {pathname}

	Implementation of a version numbering scheme for the VC-5 codec.

	Note: Version numbers are not supported by VC-5 Part 1.

	(c) 2013-2021 Society of Motion Picture & Television Engineers LLC and Woodman Labs, Inc.
	All rights reserved--use subject to compliance with end user license agreement.
*/

#ifndef _VERSION_H
#define _VERSION_H

#define VERSION_MAJOR		{major:d}
#define VERSION_MINOR		{minor:d}
#define VERSION_REVISION	{revision:d}
#define VERSION_BUILD		{build:d}

#endif
"""


if __name__ == '__main__':

    from argparse import ArgumentParser
    parser = ArgumentParser(description='Return the version information as a string')
    parser.add_argument('-c', '--config', default='../version.yaml', help='configuration file that contains the version number in YAML format')
    parser.add_argument('-f', '--format', choices=['string', 'header'], default='string', help='format for the version number')
    parser.add_argument('-o', '--output', help='pathname for the output file')
    parser.add_argument('-r', '--root', default='..', help='root directory of the software distribution')
    parser.add_argument('-v', '--verbose', action='store_true', help='enable verbose output')
    parser.add_argument('-z', '--debug', action='store_true', help='enable extra output for debugging')

    args = parser.parse_args()
    if args.debug: print(args)

    # Read the configuration information from the file
    with open(args.config) as file:
        info = yaml.safe_load(file)

    # Get the version number
    version_info = info['version']

    if args.format == 'string':

        # Form a string for the output and print the string
        version_string = '.'.join(str(version_info.get(item, 0)) for item in ['major', 'minor', 'revision'])

        if 'build' in version_info:
            version_string += '.' + version_info['build']

        print(version_string)

    elif args.format == 'header':

        parameters = version_info
        parameters['pathname'] = os.path.relpath(args.output, args.root) if args.output else "<pathname>"

        if not 'build' in parameters:
            parameters['build'] = 0

        # Output the version number as a header file
        version_string = header_template.format(**parameters)

        if args.output:
            with open(args.output, 'w') as output:
                output.write(version_string)
        else:
            print(version_string)

    else:
        print("Bad output format: {format}".format(format=args.format))
        exit(1)

