#!/usr/bin/env python3
#
# Verify the correctness of metadata files.

import sys
import csv
from lxml import etree
from common import *


def verify_metadata_tuple(tuple, level=0):
    """Verify the correctness of a metadata tuple."""

    result = True

    #TODO: Check that the tag is present and valid
    tag = get_attribute(tuple, 'tag')
    if tag == None:
        print("{0}tuple invalid tag: {1}".format(indentation(level), tag))
        result = result and False

    # Verify the metadata tuple type is present and valid
    type = get_attribute(tuple, 'type')
    if type == None: type = '0'
    if type not in data_type_dict:
        print("{0}tuple {1} invalid type: {2}".format(indentation(level), tag, type))
        result = result and False

    # Verify that the metadata tuple size is present and correct
    size = get_attribute(tuple, 'size')
    if size == None:
        print("{0}tuple {1} missing size".format(indentation(level), tag))
        result = result and False
    elif (int(size) % data_type_dict.get('size', 1)) != 0:
        print("{0}tuple {1} wrong size: {size:d}".format(indentation(level), tag, size=int(size)))
        result = result and False

    # Verify the metadata tuple repeat count
    count = get_attribute(tuple, 'count')
    if has_repeat_count(type) and count == None:
        print("{0}tuple {1} repeat count missing".format(indentation(level), tag))
        result = result and False
    elif not has_repeat_count(type) and count != None:
        print("{0}tuple {1} invalid repeat count: {count:d}".format(indentation(level), tag, count=int(count)))
        result = result and False

    # The value of the tuple padding must be correct (okay for the tuple padding to be missing)
    tuple_padding = get_attribute(tuple, 'padding')
    if tuple_padding != None:
        tuple_padding = int(tuple_padding)
        payload_padding = compute_padding(size, count)
        if tuple_padding != payload_padding:
            print("{0}tuple {1} incorrect padding: {padding:d}".format(indentation(level), tag, padding=int(padding)))
            result = result and False

    return result


def verify_simple_tuple(tuple, level=0):
    """Verify the correcness of a simple intrinsic metadata tuple passed as a dictionary."""
    return True


def verify_nested_tuple(tuple, level=0):
    """Verify the correctness of a nested intrinsic metadata tuple passed as a dictionary."""

    # Verify the correctness of the parent tuple
    result = verify_metadata_tuple(tuple, level)

    # Verify that the child nodes can be children of the parent tuple and required child nodes are present
    tag = get_attribute(tuple, 'tag')
    if tag == None:
        print("{0}tuple invalid tag: {1}".format(indentation(level), tag))
        result = result and False

    # Is every child node of the tuple element a possible child tuple of the parent tuple?
    child_dict = nested_tuple_dict.get(tag, None)
    for child in tuple:
        child_tag = get_attribute(child, 'tag')
        assert(child_tag != None)
        if child_tag not in child_dict.keys():
            print("{0}tuple {1} invalid child {child}".format(indentation(level), tag, child=child_tag))
            result = result and False

    # Are all required child tuples in the payload of the parent?
    for child_tag, child_info in child_dict.items():
        #rint(child_tag, child_info)
        if child_info.get('required', False):
            found_required_child = False
            for child in tuple:
                #print("Checking", child, get_attribute(child, 'tag'))
                if get_attribute(child, 'tag') == child_tag:
                    found_required_child = True
                    break

            if not found_required_child:
                # The parent tuple is missing a required child tuple
                print("{0}tuple {1} missing child {child}".format(indentation(level), tag, child=child_tag))
                result = result and False

    return result


def verify_complex_tuple(tuple, level=0):
    """Verify the correcness of a complex intrinsic metadata tuple passed as a dictionary."""
    return True


def verify_streaming_tuple(tuple, level=0):
    """Verify the correcness of a streaming metadata tuple passed as a dictionary."""

    #tag = tuple['tag']
    size = int(tuple['size'])
    count = int(tuple['count'])

    #tuple_format = "Tag: {tag}, type: {type}, size: {size}, count: {count}, length: {length}"
    tuple_format = 'Tag: {tag}, type: {type}, size: {size}, count: {count}, length: {length}, value: \"{value}\"'

    if tuple['type'] == 'c':

        if debug: print("US-ASCII: {value}".format(value=tuple['value']))

        if not (tuple['count'] == 1 and tuple['size'] == len(tuple['value'])):
            print(tuple_format.format(tag = tuple['tag'], type = tuple['type'], size = tuple['size'], count = tuple['count'], length = len(tuple['value']), value=tuple['value']))

    elif tuple['type'] == 'u':

        if debug: print("UTF-8: {value}".format(value=tuple['value']))

        if not (tuple['count'] == 1 and tuple['size'] == len(tuple['value'])):
            print(tuple_format.format(tag = tuple['tag'], type = tuple['type'], size = tuple['size'], count = tuple['count'], length = len(tuple['value']), value=tuple['value']))

    # elif tuple['tag'] == 'TSMP':
    #         print(tuple_format.format(tag = tuple['tag'], type = tuple['type'], size = tuple['size'], count = tuple['count'], length = len(tuple['value']), value=tuple['value']))


def result_string(result):
    """Return a printable string corresponding to the verification test results."""
    return "passed" if result else "failed"


def verify_test_case(node, args=None, level=0):
    """Verify the metadata test case read from an XML file."""

    # Is this the top-level node in the tree?
    if node.tag == 'metadata':
        if verbose: print("{0}{1}".format(indentation(level), node.tag))

        # Just process the children (no size attribute in the metadata element)
        result = True
        for child in list(node):
            result = result and verify_test_case(child, args, level+1)

        if verbose: print("{0}{1} {2}".format(indentation(level), node.tag, result_string(result)))

        return result

    # Is this node a metadata chunk element?
    elif node.tag == 'chunk':
        if verbose: print("{0}{1}".format(indentation(level), node.tag))

        # Compute the size of the chunk element in units of segments
        result = True
        for child in list(node):
            result = result and verify_test_case(child, args, level+1)

        if verbose: print("{0}{1} {2}".format(indentation(level), node.tag, result_string(result)))

        return result

    # Is this a nested tuple with children?
    elif len(list(node)) > 0:

        if verbose: print("{0}{name} {tag}".format(indentation(level), name=node.tag, tag=node.attrib['tag']))

        # Verify that the parent tuple is correct and the child nodes belong inside the parent payload
        result = verify_nested_tuple(node)

        # Verify the correctness of the tuples in the nested payload
        for child in list(node):
            result = result and verify_test_case(child, args, level+1)

        if verbose: print("{0}{name} {tag} {result}".format(indentation(level), name=node.tag, tag=node.attrib['tag'], result=result_string(result)))

        return result

    # The node must be a metadata tuple that is not nested (no children)
    else:
        if verbose: print("{0}{name} {tag}".format(indentation(level), name=node.tag, tag=node.attrib['tag']))

        result = verify_metadata_tuple(node, level)

        if verbose: print("{0}{name} {tag} {result}".format(indentation(level), name=node.tag, tag=node.attrib['tag'], result=result_string(result)))

        return result


# Table of metadata verifiers indexed by category
tuple_category_dict = {
    'simple'   : verify_simple_tuple,
    'nested'   : verify_nested_tuple,
    'complex'  : verify_complex_tuple,
    'streaming': verify_streaming_tuple
}


def verify_category_tuple(tuple, category=None):
    """Verify the correcness of the metadata tuple passed as a dictionary."""
    if debug: print(tuple, category)

    return tuple_category_dict.get(category, verify_metadata_tuple)(tuple)


if __name__ == '__main__':
    from argparse import ArgumentParser
    parser = ArgumentParser(description='Verify the correctness of metadata files')
    parser.add_argument('filelist', nargs='*', help='list of input metadata files')
    parser.add_argument('-o', '--output', help='output file for verification results')
    parser.add_argument('-c', '--category', choices=['simple', 'nested', 'complex', 'streaming'], help='category of metadata')
    parser.add_argument('-v', '--verbose', action='store_true', help='enable verbose output for monitoring progress')
    parser.add_argument('-z', '--debug', action='store_true', help='enable extra output for debugging')
    parser.add_argument('-q', '--quiet', action='store_true', help='suppress all output to the terminal')

    args = parser.parse_args()

    verbose = args.verbose
    debug = args.debug

    for pathname in args.filelist:
        #if debug: print(pathname)

        filetype = file_extension(pathname)
        #if debug: print(filetype)

        if filetype == 'csv':
            if verbose: print("Processing CSV input file: %s" % pathname)
            with open(pathname, encoding = 'ISO-8859-1') as input:
                reader = csv.reader(input, delimiter=',')
                line_count = 0
                for row_values in reader:
                    if line_count == 0:
                        column_names = row_values
                        #print("Column names: %s" % (", ".join(column_names)))
                        line_count += 1
                    else:
                        #print_row_data(column_names, row_values)
                        metadata_tuple = dict(zip(column_names, row_values))
                        verify_category_tuple(metadata_tuple, args.category)
                        line_count += 1
                #print("Processed line count: %d" % line_count)

        elif filetype == 'json':
            if verbose: print("Processing JSON input file: %s" % pathname)
            with open(pathname) as input:
                metadata += json.load(input)

            #TODO: Finish handling test cases in JSON format

        elif filetype == 'xml':
            if verbose: print("Processing XML input file: %s" % pathname)


            with open(args.filelist[0], 'rb') as input:
                tree = etree.parse(input)

            if args.debug:
                output = sys.stdout.buffer if sys.version_info.major == 3 else sys.stdout
                tree.write(output, xml_declaration=True, encoding='UTF-8', pretty_print=True)

            root = tree.getroot()
            remove_namespace(root)

            result = verify_test_case(root, args, level=0)

            if not args.quiet:
                message = "Passed" if result else "Failed"
                print("{result}: {testcase}".format(testcase=os.path.split(pathname)[1], result=message))

            if verbose: print('')

        else:
            exit("Unknown file type: %s" % filetype)

