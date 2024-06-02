#!/usr/bin/env python3
#
# Script for testing the VC-5 encoder and decoder
#
# Adapted from the original test codec script written in Tcl.
#
# (c) 2013 Society of Motion Picture & Television Engineers LLC and Woodman Labs, Inc.
# All rights reserved--use subject to compliance with end user license agreement.
#
#TODO: Pass the parts of the pathname as a dictionary that can be input to format string

import os
from platform import system
from filecmp import cmp
from datetime import date
from glob import glob
import subprocess

# Set the root location of the VC-5 reference codec
root = os.path.normpath(os.path.join(os.getcwd(), ".."))
#print(root)

# Choose the default build configuration to run
build = 'release'

# Include the platform-specific definitions file
#set platform $tcl_platform(platform)
#set pathname [format "%s-%s.tcl" [file root [info script]] $platform "tcl"]
#if [file isfile $pathname] {
#   source $pathname
#}

# Include the machine-specific definitions file
#set hostname [file rootname [info hostname]]
#set pathname [format "%s-%s.tcl" [file root [info script]] $hostname "tcl"]
#if [file isfile $pathname] {
#   source $pathname
#}

# Set default values for the sample encoder and reference decoder
default_encoder = "../encoder/build/linux/release/encoder"
default_decoder = "../decoder/build/linux/release/decoder"

# Templates for creating encoder and decoder commands
encoder_template = '{encoder} {flags} -P {part:d} -w {width:d} -h {height:d} -M {metadata} {imagefile} {bitstream}'
decoder_template = '{decoder} {flags} -P {part:d} -w {width:d} -h {height:d} -o {output} -M {metadata} {bitstream} {repacked}'

# Set the pathname for the default XML dumper
default_dumper = "./dumper.py"

# Template for creating the dumper command
dumper_template = '{dumper} --chunk metadata {bitstream} --output {binary}'

# Set the pathname for the default XML comparison tool
default_compare = "../metadata/python/compare.py"

# Template for the command that compares the decoded metadata with the input metadata
#compare_template = 'diff -q {decoded} {reference}'
compare_template = "%s {decoded} {reference}" % default_compare

# Default output format for decoded image files
default_output = 'rg48'


def platform_name():
    """Return the canonical name for the platform."""
    name = system()
    if name == 'Darwin': name = 'macos'

    return name.lower()


def switch(params, name):
    """Return true of the switch exists and is true."""
    return params.get(name, False)


def parameter(params, key, default):
    """Return the value for the specified parameter or the default value."""
    if params:
        return params.get(key, default)
    else:
        return default


def basename(pathname):
    """Return the filename with out the extension."""
    filename = os.path.split(pathname)[1]
    return os.path.splitext(filename)[0]


def check(pathname):
    """Create the directory path if it does not already exist."""
    if not os.path.isdir(pathname):
        os.makedirs(pathname)

    return pathname


def output_filename(testcase, dimensions, input_format, output_format):
    """Return the filename for the output from the reference decoder."""

    return f'{testcase}-{dimensions}-{input_format}-vc5.{output_format}'


def compare(pathname1, pathname2):
    """Return true if two binary files are equal."""
    return cmp(pathname1, pathname2)


def remove_all_files(directory):
    """Remove all files in the specified directory."""
    for file in os.scandir(directory):
        #print(file.path)
        os.remove(file.path)


def prepare_output_directory(directory):
    """Remove all files in the specified directory."""
    if os.path.isdir(directory):
        remove_all_files(directory)
    else:
        os.makedirs(directory)


def test_codec(input, results, params=None):
    """Test the sample encoder and reference decoder on the specified image."""

        # Apply the encoder to the input image to produce an encoded bitstream
        #exec $encoder -w $width -h $height $input $output

            # Decode the bitstream output by the encoder
#                     if $display {
#                         exec $decoder -p $format $output $result $picture
#                     } else {
#                         #puts "Decoder input: $output"
#                         #puts "Decoder output: $result"
#                         exec $decoder -p $format $output $result
#                     }

        # Compare the encoded file with the master file
#               set filename [format "%s-%s-%s.%s" [file rootname $image] $dimension $format vc5]
#               set master [file join $media $testcase master $part [string tolower $build] $filename]
#               if [file exists $master] {
            #if $verbose {puts "Checking result against master: $master"}
#                   if {! [compare $output $master]} {
#                       puts "Failed comparison: [file tail $master] ($build)"
#                       incr failures
#                   }
#               } else {
#                   if $debug {puts "No master file: $master"}
#                   incr missing
#               }


def test_all_images(directory, results, params=None):
    """Test all images in the specified directory."""

    for image in glob(f'{directory}.*.{format}'):

        # Set the full pathname to the input image for testing the encoder
        input = os.path.join(directory, image)

        # Set the full pathname for the bitstream output by the encoder
        filename = "{name}-{dimension}-{format}.vc5".format(name=rootname(image), dimension=dimension, format=format)
        output = os.path.join(media, testcase, 'results', filename)

        # Set the full pathname for the decoded image
        filename = "{name}-{dimension}-{format}-vc5.{extension}".format(name=rootname(image), dimension=dimension, format=format, extension=format)
        result = os.path.join(media, testcase, results, filename)

        if switch(params, display):
            # Set the full pathname for the displayable picture
            picture = os.path.join(media, testcase, results, output_filename(testcase, dimensions, format, "dpx"))

        if switch(params, verbose):
            print(f'Testing input: {input}')

        test_codec(input, results, params)


def test_all_formats():
    """Test all image formats in the specified directory."""

    # Test each pixel format for this image width and height
    for format in glob(directory):
        if switch(params, 'debug'):
            print(f'Format: {format}')

        # Test each image for this combination of image dimensions and pixel format
        directory = os.path.join(media, testcase, dimension, format)

        test_all_images(directory)



def test_all_dimensions(directory, results, params=None):
    """Test all image dimensions in the specified directory."""

    # Test each image width and height in this test case
    for dimension in glob("directory/[0-9]+"):

        if switch(params, 'debug'):
            print(f'Dimension: {dimension}')

        # Split the image dimensions into the width and height
        (width, height) = dimension.split('x')

        directory = os.path.join(media, testcase, dimension)
        if switch(params, 'debug'):
            print(f'Directory: {directory}')

        test_all_formats(directory, results, params)


def test_all_testcases(testcases, results, params=None):
    """Test all combinations of image dimensions and pixel format for each test case."""

    for testcase in testcases:
        if switch(params, 'debug'):
            print(f'Test case: {testcase}')

        # Set the working directory for any debug output from the programs
        os.chdir(check(os.path.join(media, testcase, results)))

        # Create the directory for the test results
        check(os.path.join(media, testcase, results))

        # Initialize the count of failed test cases
        failures = 0

        # Initialize the count of missing master files
        missing = 0

        directory = os.path.join(media, testcase)
        if switch(params, 'debug'):
            print(f'Directory: {directory}')

        test_all_dimensions(directory, results, params)


def test_metadata(params=None):
    """Test the sample encoder and reference decoder for conformance with ST 2073-7 Metadata."""

    verbose = parameter(params, 'verbose', False)
    debug = parameter(params, 'debug', False)
    quiet = parameter(params, 'quiet', False)

    if quiet:
        verbose = False
        debug = False
    else:
        print("Running metadata tests ...")

    encoder = os.path.relpath(parameter(params, 'encoder', default_encoder), '.')
    decoder = os.path.relpath(parameter(params, 'decoder', default_decoder), '.')
    dumper = parameter(params, 'dumper', default_dumper)

    media = parameter(params, 'media', '../media')

    # Get the folder that contains the metadata test cases
    input_dir = os.path.join(media, 'metadata', 'input')
    assert os.path.isdir(input_dir)

    # Set the image file used for encoding
    imagefile = os.path.relpath(os.path.join(media, 'boxes/1280x720/rg48/boxes-1280x720-0000.rg48'), '.')

    # Prepare the directory for the encoded bitstreams
    #bitstream_dir = os.path.join(root, 'encoder', 'metadata', 'temp', 'bitstreams')
    bitstream_dir = os.path.join(root, 'encoder', 'temp', 'bitstreams')
    prepare_output_directory(bitstream_dir)

    # Set the encoding parameters
    part = 7
    width = 1280
    height = 720

    # Suppress all output from the subprograms unless debugging
    flags = '-v' if debug else '-q'

    # Encode all metadata test cases
    for file in os.scandir(input_dir):
        metadata = os.path.relpath(file.path, '.')
        bitstream = os.path.relpath(os.path.join(bitstream_dir, basename(metadata) + '.vc5'), '.')
        scope = locals()
        arguments = {key: eval(key, scope) for key in ['encoder', 'flags', 'part', 'width', 'height', 'metadata', 'imagefile', 'bitstream']}
        command = encoder_template.format(**arguments)
        if debug: print(command)
        result = subprocess.run(command.split())
        if result.returncode != 0: print(result)

    # Prepare the directory for the binary metadata extracted from the bitstreams
    encoded_dir = os.path.join(root, 'encoder', 'temp', 'encoded')
    prepare_output_directory(encoded_dir)

    # Extract the binary metadata from all bitstreams
    for file in os.scandir(bitstream_dir):
        bitstream = os.path.relpath(file.path)
        binary = os.path.relpath(os.path.join(encoded_dir, basename(bitstream) + '.bin'), '.')
        scope = locals()
        arguments = {key: eval(key, scope) for key in ['dumper', 'bitstream', 'binary']}
        command = dumper_template.format(**arguments)
        if debug: print(command)
        result = subprocess.run(command.split())
        if result.returncode != 0: print(result)

    # Prepare the directory for the metadata decoded from the bitstream
    decoded_dir = os.path.join(root, 'decoder', 'temp', 'decoded')
    prepare_output_directory(decoded_dir)

    # Prepare the the directory for the decoded images
    images_dir = os.path.join(root, 'decoder', 'temp', 'images')
    prepare_output_directory(images_dir)

    # Output format for decoded (repacked) images
    output = default_output

    # Decode all bitstreams and output the metadata in XML format
    for file in os.scandir(bitstream_dir):
        bitstream = os.path.relpath(file.path)
        metadata = os.path.relpath(os.path.join(decoded_dir, basename(bitstream) + '.xml'), '.')
        repacked = os.path.relpath(os.path.join(images_dir, basename(bitstream) + '.rg48'), '.')
        scope = locals()
        arguments = {key: eval(key, scope) for key in ['decoder', 'flags', 'part', 'width', 'height', 'output', 'metadata', 'bitstream', 'repacked']}
        command = decoder_template.format(**arguments)
        if debug: print(command)
        result = subprocess.run(command.split())
        if result.returncode != 0: print(result)

    # Compare the decoded metadata with the input metadata
    passed = 0
    failed = 0
    for file in os.scandir(input_dir):
        reference = os.path.relpath(file.path, '.')
        decoded = os.path.relpath(os.path.join(decoded_dir, os.path.split(reference)[1]))
        #print(metadata)
        #print(decoded)
        scope = locals()
        arguments = {key: eval(key, scope) for key in ['decoded', 'reference']}
        command = compare_template.format(**arguments)
        if debug: print(command)
        result = subprocess.run(command.split())
        if result.returncode == 0:
            if verbose: print("Passed: {decoded}".format(decoded=os.path.split(decoded)[1]))
            passed += 1
        else:
            if verbose: print("Failed: {decoded}".format(decoded=os.path.split(decoded)[1]))
            failed += 1

    if not quiet:
        print(f'Test results passed: {passed:d}, failed: {failed:d}')

    return failed


if __name__ == "__main__":
    # Parse the command line arguments
    from argparse import ArgumentParser

    # Use glob to expand filename patterns
    #from glob import glob

    parser = ArgumentParser(conflict_handler='resolve')
    parser.add_argument('-r', '--root', help='set the root directory that contains the codec')
    parser.add_argument('-m', '--media', help='set the location of the test cases')
    parser.add_argument('-b', '--build', help='set the build configuration (debug or release)')
    #parser.add_argument('-c', '--compiler', help='set the version of Visual Studio (Windows only)')
    parser.add_argument('-t', '--testcases', help='list of test cases separated by commas with no spaces')
    parser.add_argument('-p', '--part', help='part of VC-5 standard to test')
    parser.add_argument('-d', '--display', action='store_true', help='output a file containing a displayable picture (for debugging)')
    parser.add_argument('-v', '--verbose', action='store_true', help='enable verbose output for debugging')
    parser.add_argument('-z', '--debug', action='store_true', help='enable extra output for debugging')
    parser.add_argument('-q', '--quiet', action='store_true', help='suppress all output to the terminal')
    parser.add_argument('filelist', nargs='*')
    args = parser.parse_args()
    #print args.filelist
    #output_copyright()

    # Create a dictionary of command-line arguments and other parameters
    params = vars(args)

    root = args.root if args.root else os.path.abspath('..')
    media = args.media if args.media else os.path.join(root, 'media')
    build = args.build if args.build else 'release'
    testcases = args.testcases if args.testcases else ['boxes', 'gradient', 'solid']
    part = args.part if args.part else 1

    build = build.lower()
    name = platform_name()

    for key in ['root', 'media', 'build', 'testcases', 'part', 'name']:
        params[key] = eval(key)

#     if name == 'windows':
#         compiler = args.compiler if args.compiler else 'VS2017'
#         params['compiler'] = compiler

    if not args.quiet and args.debug:
        print("Root directory for codec install: %s" % root)
        print("Root directory for the test media: %s" % media)
        print("Platform: %s" % name)
        print("Build configuration: %s" % build)
        print("Test list: %s" % testcases)
        #print("VC-5 part: %s" % part)

#         if 'compiler' in params:
#             print("Visual Studio version: {compiler}".format(compiler=params['compiler']))

    # Set the pathnames to the encoder and decoder
    encoder = os.path.join(root, 'encoder/build/linux', build, 'encoder')
    decoder = os.path.join(root, 'decoder/build/linux', build, 'decoder')
    #print(encoder, decoder)

    params['encoder'] = encoder
    params['decoder'] = decoder


    # Assume that support for metadata is being tested
    result = test_metadata(params)


    ### The rest of this script for testing VC-5 parts 1-6 is under development ###
    if result == 0:
        exit(0)
    else:
        exit(1)


    # Use the filename of this script and date for the results folder
    name = os.path.join(os.path.rootname(os.path.tail(argv[0])))

    today = date.today()
    testrun = "{0}-{1}".format(name, today.strdate("%Y-%m-%d"))
    results = os.path.join('results', testrun, part.lower(), build.lower())
    print(results)

    test_all_testcases(testcases, results, params)

    if missing > 0:
        print(f'Finished {testcase} with {missing} missing master files for {build} build ({failures} failures)')
    else:
        print(f'Testcase {testcase} done using {build} build ({failures} failures)')

