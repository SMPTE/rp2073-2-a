#!/usr/bin/env python
#
# Script to compute all possible combinations of simple and nested intrinsic test cases

import os
import json
#import yaml
import itertools
import re


# List of each category of intrinsic metadata used to create complex test cases
complex_categories = ['simple', 'encoding', 'layers']

# Pattern for parsing encoding test case filenames
encoding_pattern = re.compile('encoding\-([A-Z]{4})\.json')

# Pattern for parsing layers test case filenames
layers_pattern = re.compile('layers\-(.+)\.json')

# Output directories for complex test cases in JSON or XML format
complex_json_dir = '$(COMPLEX_JSON_DIR)'
complex_xml_dir = '$(COMPLEX_XML_DIR)'

# Command for generating a complex intrinsic metadata test case (JSON format)
json_target_command = '$(GENERATE) -m -j $^ -o $@'

# Command for generating a complex intrinsic metadata test case (XML format)
xml_target_command = '$(GENERATE) $^ -o $@'
#xml_target_command = '$(GENERATE) -r $^ -o $@'


def file_extension(pathname):
    # Return the file extension as a string without the file extension separator
    return os.path.splitext(pathname)[1][1:].lower()


def filename_element(testname, pathname):
    """Extract portion of the filename to form a filename for a complex test case."""

    # Extract the filename without the directory path
    filename = os.path.split(pathname)[1]
    #print("%s: %s" % (testname, filename))

    if testname == 'simple':
        return os.path.splitext(filename)[0]

    elif testname == 'encoding':
        match = encoding_pattern.search(filename)
        if match:
            curve = match.group(1)
            return curve
        else:
            print("Could not match encoding filename: %s" % filename)

    elif testname == 'layers':
        match = layers_pattern.search(filename)
        if match:
            layer = match.group(1)
            return layer
        else:
            print("Could not match layer filename: %s" % filename)

    else:
        exit("Unsupported test category: %s" % testname)


def output_makefile_rule(testcase, output=None):
    """Output a make file rule for building a complex metadata test case (JSON format)."""
    target = "%s/%s: " % (complex_json_dir, testcase['output'])
    target += ' '.join(testcase['input'])
    target += "\n\t"
    target += json_target_command
    target += "\n"

    if output:
        output.write(target + "\n")
    else:
        print(target)


def output_makefile_testcase(filename, output=None):
    """Output the filename for a complex intrinsic metadata test case."""

    if output:
        output.write(filename + "\n")
    else:
        print(filename)


def output_complex_testcase(combination, category_names, output=None, args=None):
    """Output a filename for the testcase combining files from multiple categories."""

    # List of key filename parts from each combination of metadata files
    filename_parts = []

    for testname, pathname in zip(category_names, combination):
        #print(testname, pathname)
        filename_parts.append(filename_element(testname, pathname))

    #print(filename_parts)

    filename = "%s-%s-%s.json" % tuple(filename_parts)
    testcase = {'output': filename, 'input': combination}
    #print(testcase)

    if args:
        if args.rules:
            output_makefile_rule(testcase, output)
        elif args.testcases:
            output_makefile_testcase(filename, output)
        else:
            exit("Must specify type of output")
    else:
        # Output the make file rules by default
        output_makefile_rule(testcase, output)


def generate_json_inputs(category_lists, category_names, args=None):
    """Output rules and targets for all combinations of each item in each list of test data."""
    #print(category_names)

    if args and args.output:
        with open(args.output, 'w') as output:
            for combination in itertools.product(*category_lists):
                #print(combination)
                output_complex_testcase(combination, category_names, output, args)
    else:
        for combination in itertools.product(*category_lists):
            #print(combination)
            output_complex_testcase(combination, category_names, None, args)


def xml_target_filename(filename):
    """Return the filename for a complex test case in XML format."""
    basename = os.path.splitext(filename)[0]
    return "%s.xml" % basename


def generate_xml_targets(filelist, args=None):
    """Generate a list of make file targets for complex intrinsic metadata test cases in XML format."""
    #print(filelist)

    if args and args.output:
        with open(args.output, 'w') as output:
            for filename in filelist:
                filename = filename.strip()
                filename = xml_target_filename(filename)
                #print(filename)
                output.write(filename + "\n")
    else:
        for filename in filelist:
            filename = filename.strip()
            filename = xml_target_filename(filename)
            print(filename)


def output_xml_rule(input_filename, output_filename, output):
    """Output a make file rule for generating a complex intrinsic metadata test case in XML format."""
    target = "%s/%s: " % (complex_xml_dir, output_filename)
    target += "%s/%s" % (complex_json_dir, input_filename)
    target += "\n\t"
    target += xml_target_command
    target += "\n"

    if output:
        output.write(target + "\n")
    else:
        print(target)


def generate_xml_rules(filelist, args=None):
    """Generate make file rules for generating complex intrinsic metadata test cases in XML format."""
    #print(filelist)

    if args and args.output:
        with open(args.output, 'w') as output:
            for filename in filelist:
                input_filename = filename.strip()
                output_filename = os.path.splitext(input_filename)[0] + ".xml"
                output_xml_rule(input_filename, output_filename, output)
    else:
        for filename in filelist:
            input_filename = filename.strip()
            output_filename = os.path.splitext(input_filename)[0] + ".xml"
            output_xml_rule(input_filename, output_filename, None)


def generate_xml_inputs(filelist, args=None):
    """Output rule and targets for generating complex intrinsic metadata test cases in XML format."""
    #print(targets)

    if args:
        if args.testcases:
            generate_xml_targets(filelist, args)
        elif args.rules:
            generate_xml_rules(filelist, args)
        else:
            exit("Must specify type of output")
    else:
        # Output the make file targets by default
        generate_xml_targets(filelist, args)


if __name__ == '__main__':
    from argparse import ArgumentParser
    parser = ArgumentParser(description='Generate all combinations of complex intrinsic metadata test cases')
    parser.add_argument('filelist', nargs=1, help='lists of simple and nested test cases (JSON or YAML format)')
    parser.add_argument('-t', '--testcases', action='store_true', help='output a list of metadata test cases (default)')
    parser.add_argument('-r', '--rules', action='store_true', help='output the rules for making the test cases')
    parser.add_argument('-f', '--format', default='json', help='format for the test cases (JSON or XML)')
    parser.add_argument('-o', '--output', help='output file for the testcases or rules')
    parser.add_argument('-v', '--verbose', action='store_true', help='enable verbose output for monitoring progress')

    args = parser.parse_args()
    #print(args)

    if not (args.testcases or args.rules):
        # Output a list of testcases by default
        args.testcases = True

    pathname = args.filelist[0]
    filetype = file_extension(pathname)

    if args.format == 'json':
        assert filetype == 'json'
    elif 'xml':
        assert filetype == 'targets'
    else:
        exit("Unsupported output format: %s" % args.format)

    if filetype == 'json':
        with open(pathname) as input:
            testdata = json.load(input)
            #print(testdata)

        # Initialize the list of category names and the list of files in each category
        category_names = []
        category_lists = []

        for category in testdata['combinations']:
            #print("Category: %s" % category)
            for key, value in category.items():
                if key in complex_categories:
                    category_names.append(key)
                    category_lists.append(value)

        #print(datafile_list)

        generate_json_inputs(category_lists, category_names, args)

    elif filetype == 'targets':
        with open(pathname) as input:
            targets = input.readlines()
            #print(targets)
            generate_xml_inputs(targets, args)

    elif filetype == 'yaml':
        exit("File type not supported: %s" % filetype)

    else:
        exit("Unknown file type: %s" % filetype)

