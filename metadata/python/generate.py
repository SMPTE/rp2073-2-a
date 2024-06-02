#!/usr/bin/env python3
#
# Generate a valid XML file of metadata as defined in ST 2073-7 Metadata.
#
# This script accepts a list of metadata tuples in CSV format and randomly generates an
# output list of tuples with or without duplicates as specified by command-line options.
# The output file is in the XML format defined in ST 2073-7 Annex A.
#
# TODO: Add code to create large metadata chunks
#
# TODO: Add code to create instances for other metadata classes
#
# TODO: Should the type for nested tuples be the zero character or numerical zero?
#
# TODO: Can the EncodingCurve, LayerMetadata, and StreamingData classes be refactored to use a common base class?

import os
import csv
import sys
import random
import json
import time
import re
import uuid
from datetime import datetime, timezone
from pathlib import Path
from lxml import etree


# Import common definitions used by the metadata scripts
from common import *


# Location of the metadata media on GitHub
media = 'https://github.com/SMPTE/smpte-10e-vc5/tree/master/media'


# Namespace for the XML metadata files
namespace = {'metadata' : 'https://www.vc5codec.org/xml/metadata'}


# List of tags for metadata tuples that can contain nested tuples (used for GPMF streaming data)
nested_tag_list = ['DEVC', 'STRM']

# Metadata tuple tags for streaming device parameters
#device_parameter_tags = ['DVID', 'DVNM', 'TICK']

# Dictionary that maps common acronyms for extrinsic metadata to the tags for the class and data
metadata_class_tag_dict = {
    'XMP'   : {'class': 'XMPD', 'value': 'XMPd', 'type': 'x'},
    'DPX'   : {'class': 'DPXF', 'value': 'DPXh', 'type': 'B'},
    'MXF'   : {'class': 'MXFD', 'value': 'MXFd', 'type': 'B'},
    'ACES'  : {'class': 'ACES', 'value': 'ACEh', 'type': 'B'},
    'ALE'   : {'class': 'ALEM', 'value': 'ALEd', 'type': 'c'},
    'DMCVT' : {'class': 'DMCT', 'value': 'CVTD', 'type': 'B'},
}


# Pattern for matching an entry in the testing log file
test_pattern = re.compile(r'^cd\s+(.+?)\;\s+(.+)$')


# Pattern for normalizing the string representation of floating-point numbers
float_pattern = re.compile(r'^(\-{0,1}\d+\.\d+[1-9])0*$')


# Pattern for normalizing the string representation of floating-point numbers without decimal points
integer_pattern = re.compile(r'^(\-{0,1}\d+)(\.0*){0,1}$')


def file_resource_identifier(pathname):
    """Return the uniform resource identifier (URI) for the file as specified in RFC 3986."""
    #return "file://" + os.path.abspath(pathname)

    # Split the pathname into the list of directories and the filename
    pathlist = os.path.split(pathname)
    filename = pathlist[1]
    pathlist = pathlist[0].split(os.sep)

    # Find the directory of test cases in the pathlist
    testcases = ['intrinsic', 'streaming', 'extrinsic', 'dark']
    for index in range(len(pathlist)):
        if pathlist[index] in testcases: break

    pathlist = pathlist[index:]

    fullpath = os.path.join(media, os.sep.join(pathlist), filename)
    #print(fullpath)

    return fullpath


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


def print_row_data(names, values):
    """Print a row of data from a CSV file."""
    for name, value in zip(names, values):
        print(name, value)


def append_row_data(metadata, names, values):
    """Append a dictionary representing one row of data."""
    tuple = {}
    for name, value in zip(names, values):
        #print(name, value)
        if name == 'tag' and value[0] == '#':
            # Skip this tuple if the row begins with the comment character
            return
        tuple[name] = value

    #print(tuple)
    metadata.append(tuple)


def create_xml_root():
    """Create the root element for the XML representation of metadata."""
    root = etree.Element('metadata')
    #root.set("xmlns", "https://www.vc5codec.org/xml/metadata")
    root.set("xmlns", namespace['metadata'])
    return root


def append_xml_chunk(root, tag):
    """Append an internal XML representation of a small metadata chunk to the root element."""
    chunk = etree.SubElement(root, 'chunk')
    chunk.set('tag', tag)
    return chunk


def append_xml_class(chunk, tag):
    """Append the XML representation of a metadata class to the chunk."""
    instance = etree.SubElement(chunk, 'tuple')
    instance.set('tag', tag)
    instance.set('type', 'E')
    return instance


def append_xml_data(instance, tag):
    """Append the element that contains the data in the class instance."""
    data = etree.SubElement(instance, 'tuple')
    data.set('tag', tag)
    data.set('type', 'x')
    return data


def append_xml_tuple(parent, tuple):
    """Append an internal XML representation of a metadata tuple to the parent element."""
    element = etree.SubElement(parent, 'tuple')
    for key, value in tuple.items():
        #print(key, value)
        element.set(key, str(value))

    return element


def append_xml_file_info(instance, fileinfo):
    """Append the file information metadata to the extrinsic class instance."""
    for tag, value in fileinfo.items():
        #tuple = {'tag': tag, 'type': 'c', 'size': str(len(value)), 'count': str(0), 'value': value}
        tuple = {'tag': tag, 'type': 'c', 'size': str(len(value)), 'value': value}
        append_xml_tuple(instance, tuple)


def append_xml_element(parent, tag, type, size, count, value):
    """Append an internal XML representation of a metadata tuple with the value as serialized text."""
    element = etree.SubElement(parent, 'tuple')

    #print("append_xml_element tag: %s, type: %s" % (tag, type))
    element.set('tag', tag)
    element.set('type', type)

    if size != None:
            element.set('size', str(size))

    # Only add the repeat count attribute if the tuple has a repeat count
    # data_type_info = data_type_dict.get(type, None)
    # if data_type_info and data_type_info['repeat'] and count != None:
    #     element.set('count', str(max(count, 1)))
    if has_repeat_count(type) and count != None:
        element.set('count', str(max(count, 1)))

    element.text = value

    return element


def append_xml_tree(parent, child):
    """Append an element tree as the text body of the parent element."""
    #parent.text = root
    #etree.SubElement(parent, child)
    parent.append(child)


def append_nested_tuple(parent, tuple):
    """Recursively append a nested tuple and its children to the parent tuple."""
    element = append_xml_tuple(parent, {key: value for (key, value) in tuple.items() if key != 'value'})

    if 'value' in tuple:
        if type(tuple['value']) is list:
            for child in tuple['value']:
                append_nested_tuple(element, child)
        else:
            if tuple['tag'] in ['XMPd']:
                # Add the value as a CDATA element
                element.text = etree.CDATA(tuple['value'])

            elif tuple['tag'] in ['MXFd', 'ACEh']:
                # Add the value as element text
                element.text = tuple['value']

            else:
                # Add the value as an attribute
                element.set('value', tuple['value'])


def default_tuple_type(tag):
    """Return a default data type for a tuple with the specified tag."""

    # Encoding curves are a special case
    if tag in ['LOGA', 'GAMA', 'LINR', 'FSLG', 'LOGC', 'PQEC', 'HLGE']:
        return 'P'
    else:
        # Assume the metadata tuple is a nested tuple
        return 0


def size_in_segments(size):
    """Round up the size in bytes to the number of segments."""
    return int((size + 3) / 4)


def update_payload_size(node, args=None, level=0):
    """Update the size and padding attributes in nested metadata chunks and tuples."""

    verbose = args.verbose if args else False

    # Size of a metadata tuple header (in bytes)
    header_size = 8

    #print(etree.tostring(tree, pretty_print=True))

    # size = int(node.attrib.get('size', 0))
    # print("{0}{1}: {2:d}".format(indentation(level), node.tag, size))

    # Is this the top-level node in the tree?
    if node.tag == 'metadata':
        if verbose: print("{0}{1}".format(indentation(level), node.tag))

        # Just process the children (no size attribute in the metadata element)
        for child in list(node):
            update_payload_size(child, args, level+1)

        if verbose: print("{0}{1}".format(indentation(level), node.tag))

    # Is this node a metadata chunk element?
    elif node.tag == 'chunk':
        if verbose: print("{0}{1}".format(indentation(level), node.tag))

        # Compute the size of the chunk element in units of segments
        size = 0
        for child in list(node):
            size += update_payload_size(child, args, level+1)

        # Round up the size in bytes to the number of segments
        segment_count = size_in_segments(size)

        # Set the chunk size attribute (in units of segments)
        node.set('size', str(segment_count))

        if verbose: print("{0}{1} size: {2:d} segments".format(indentation(level), node.tag, segment_count))

        # Return the size of the payload in bytes plus the size of the header (4 bytes)
        return int(4 * segment_count + header_size)

    # Is this a nested tuple with children?
    elif len(list(node)) > 0:
        if verbose: print("{0}{name} {tag}".format(indentation(level), name=node.tag, tag=node.attrib['tag']))

        # Fill in a default data type if the type is missing
        data_type = get_attribute(node, 'type')
        if data_type == None:
            tag = get_attribute(node, 'tag')
            data_type = default_tuple_type(tag)
            node.set('type', str(data_type))

        # Compute the total size of all tuples comprising the value of this tuple
        size = 0
        for child in list(node):
            size += update_payload_size(child, args, level+1)

        # Set the size of the value (count should be zero)
        node.set('size', str(size))

        # Compute the size of the payload in segments
        segment_count = size_in_segments(size)

        # Compute the padding used to round up the value size to an integer number of segments
        padding = 4 * segment_count - size

        # Set the size of the padding
        node.set('padding', str(padding))

        # Compute the size of the tuple including the header
        total_size = int(4 * segment_count + header_size)

        if verbose:
            print("{0}{name} {tag}, value size: {size:d} bytes, padding: {padding:d}, tuple size: {total_size:d}"
                .format(indentation(level), name=node.tag, tag=node.attrib['tag'], size=size, padding=padding, total_size=total_size))

        return total_size


    # The node must be a metadata tuple that is not nested (no children)
    else:
        #print("{0}{name} {tag}".format(indentation(level), name=node.tag, tag=node.attrib['tag']))

        # Fill in a default data type if the type is missing
        data_type = get_attribute(node, 'type')
        if data_type == None:
            tag = get_attribute(node, 'tag')
            data_type = default_tuple_type(tag)
            node.set('type', str(data_type))

        # Set the size to zero if it is missing
        #size = int(node.attrib.get('size', 0))
        size = get_attribute(node, 'size')
        if size == None:
            size = 0
            node.set('size', str(size))

            # Set the padding attribute even though the tuple does not have a value
            padding = 0
            node.set('padding', str(padding))

            # The total size of the tuple is the size of the header (no payload)
            return header_size

        # Convert the size string to an integer
        size = int(size)

        # Set the count to one if the tuplep does not have a repeat count
        count = max(int(node.attrib.get('count', 0)), 1)

        # Compute the size of hte metadata payload (in bytes)
        value_size = size * count

        # Set the repeat count only if the data type has a repeat count (minimum value is one)
        if 'type' in node.attrib and has_repeat_count(node.attrib['type']):
            node.set('count', str(count))
        elif 'count' in node.attrib:
            del node.attrib['count']

        # Compute the size of the payload in segments
        segment_count = size_in_segments(value_size)

        # Compute the padding used to round up the value size to an integer number of segments
        padding = 4 * segment_count - value_size

        # Set the size of the padding
        node.set('padding', str(padding))

        # Compute the size of the tuple including the header
        total_size = int(4 * segment_count + header_size)

        if verbose:
            print("{0}{name} {tag}, value size: {size:d} bytes, padding: {padding:d}, tuple size: {total_size:d}"
                .format(indentation(level), name=node.tag, tag=node.attrib['tag'], size=value_size, padding=padding, total_size=total_size))

        # Return the size of the value plus padding and the header (4 bytes)
        return total_size


def output_xml_metadata(root, args=None):
    """Output the XML metadata representation to the terminal window or a file."""

    # Adjust the size and padding in the metadata chunk element and the nested tuples in its payload
    update_payload_size(root, args)

    # Write the XML tree to the specified output file or the terminal
    tree = etree.ElementTree(root)
    if args and args.output:
        # Output the XML to the specified file
        with open(args.output, 'wb') as output:
            tree.write(output, xml_declaration=True, encoding='UTF-8', pretty_print=True)
    else:
        # Output the XML to the standard output
        output = sys.stdout.buffer if sys.version_info.major == 3 else sys.stdout
        tree.write(output, xml_declaration=True, encoding='UTF-8', pretty_print=True)


def append_metadata_chunk(metadata, chunk, args=None):
    """Append a chunk subtree to the metadata element."""
    #objectify.deannotate(chunk, xsi_nil=True, cleanup_namespaces=True)
    #remove_namespace(chunk, namespace['metadata'])
    remove_namespace(chunk, args)
    metadata.append(chunk)


def write_nested_metadata(root, tag, template):
    """Write the nested metadata element to an output filename based on the tag."""
    tree = etree.ElementTree(root)

    #print(root, tag, template)

    if template:
        # Output the XML to a file
        filename = template % tag
        #print("Filename: %s" % filename)
        with open(filename, 'wb') as output:
            tree.write(output, xml_declaration=True, encoding='UTF-8', pretty_print=True)
    else:
        # Output the XML to the standard output
        output = sys.stdout.buffer if sys.version_info.major == 3 else sys.stdout
        tree.write(output, xml_declaration=True, encoding='UTF-8', pretty_print=True)


def is_nested_tuple(tag):
    """Return true if the tuple value is a list of nested metadata tuples (used for GPMF streaming data)."""
    return tag in nested_tag_list


def create_nested_tuple_list(tuple_list, nested_tuple_tag, nested_tag_list = [], level=0):
    """Return the tuples within a nested tuple and the remainder of the tuple list."""

    print("%screate_nested_tuple_list(nested_tuple_tag: %s)" % (indentation(level), nested_tuple_tag))

    # List of tags in the nesting of tuples at the current level
    nested_tag_list.append(nested_tuple_tag)

    # Initialize a list for the nested tuples within the current tuple
    nested_tuple_list = []

    while (tuple_list):
        tuple = tuple_list.pop(0)
        print("%sTuple at current level: %s" % (indentation(level), tuple['tag']))

        if tuple_list is None:
            # No more tuples in the list
            nested_tuple_list.append(tuple)
            print("%sEmpty tuple list: %s" % (indentation(level), [tuple['tag'] for tuple in nested_tuple_list]))
            return (nested_tuple_list, tuple_list)

        elif tuple['tag'] == nested_tuple_tag:
            # Done collecting the tuples nested within this nesting level
            print("%sLeaving recursion: %s" % (indentation(level), [tuple['tag'] for tuple in nested_tuple_list]))
            return (nested_tuple_list, [tuple] + tuple_list)

        elif is_nested_tuple(tuple['tag']) and not tuple['tag'] in nested_tag_list:
            # Found a nested tuple within the current nesting level
            print("%sFound nested tuple: %s" % (indentation(level), tuple['tag']))
            (nested_list, tuple_list) = create_nested_tuple_list(tuple_list, tuple['tag'], list(nested_tag_list), level+1)
            tuple['value'] = nested_list

        elif is_nested_tuple(tuple['tag']):
            # Found nested tuple at higher level
            print("%sFound nested tuple at higher level: %s" % (indentation(level), tuple['tag']))
            return (nested_tuple_list, [tuple] + tuple_list)

        # Add this tuple to the list of tuples nested within the tuple at this level
        nested_tuple_list.append(tuple)
        print("%sAdded tuple to nested_tuple_list: %s" % (indentation(level), tuple['tag']))

    print("%sReturning tuple list: %s" % (indentation(level), [tuple['tag'] for tuple in nested_tuple_list]))
    return (nested_tuple_list, tuple_list)


def create_json_class(tuple_list, tag):
    """Return a dictionary for the metadata class instance and its payload."""

    if tag == 'GPMF':
        # Modify the tuple list to nest streaming data within streams and streams within devices

        print("Creating nested list for %s" % tag)

        nested_tuple_list = []

        while (tuple_list):
            tuple = tuple_list.pop(0)
            if tuple['type'] == '0' or is_nested_tuple(tuple['tag']):
                (nested_list, tuple_list) = create_nested_tuple_list(tuple_list, tuple['tag'])
                tuple['value'] = nested_list
            nested_tuple_list.append(tuple)

        return nested_tuple_list

    else:
        return {'tag': tag, 'type': 'E', 'value': tuple_list}


def read_extrinsic_payload(extrinsic_class_code, pathname):
    """Read the payload for an extrinsic metadata class instance from the specified file."""

    # This code has only been tested on the extrinsic metadata classes listed in the table
    assert extrinsic_class_code in metadata_class_tag_dict.keys()

    with open(pathname) as file:
        payload = file.read()
        #print(payload)

    #TODO: Need to encode binary payloads as base64
    return payload


def extrinsic_metadata_tags(extrinsic_class_code):
    """Return the tags for the metadata class, payload value, and payload type."""

    if extrinsic_class_code in metadata_class_tag_dict.keys():
        metadata_class_info = metadata_class_tag_dict[extrinsic_class_code]
        return (metadata_class_info[key] for key in ['class', 'value', 'type'])
    else:
        exit("Unknown extrinsic metadata acronym: %s" % extrinsic_class_code)

    return (metadata_class)


def adjust_file_size_and_count(size, count):
    """Reduce the size and increase the count to fit in the size and repeat count fields."""

    if count == 0: count = 1

    total_size = size * count

    # Factor the total size into two integers that fit the size and repeat count fields
    for divisor in range(2, total_size + 1):
        if (total_size % divisor) == 0:
            reduced_size = total_size // divisor
            if reduced_size <= tuple_repeat_size_max:
                (size, count) = (reduced_size, divisor)
                break

    assert (size * count == total_size)

    if size <= tuple_repeat_size_max and count <= tuple_repeat_count_max:
        return (size, count)

    # Reduce the size by a factor of two and double the repeat count even if the total size is increased
    repeat_count = 1
    while total_size > tuple_repeat_size_max:
        if ((total_size % 2) != 0): total_size += 1
        total_size //= 2;
        repeat_count *= 2;

    return (total_size, repeat_count)


def create_extrinsic_class(extrinsic_class_code, payload, fileinfo=None, filesize=None):
    """Return an extrinsic metadata class instance."""

    #print("Creating extrinsic metadata class instance: %s" % metadata_class_tag)

    # Get the tags used in the metadata class instance
    (metadata_class_tag, metadata_value_tag, metadata_value_type) = extrinsic_metadata_tags(extrinsic_class_code)

    # Create the metadata class instance
    instance = {'tag': metadata_class_tag, 'type': 'E'}

    #value_tuple = {'tag': metadata_value_tag, 'type': metadata_value_type, 'value': payload}
    value_tuple = {'tag': metadata_value_tag, 'type': metadata_value_type}

    # Add extra information about the extrinsic metadata file
    if filesize != None and 'size' in filesize:
        # Adjust the size and count to fit the fields in a tuple with a repeat count
        (size, count) = adjust_file_size_and_count(int(filesize['size']), 1)

        # Update the tuple with the new size and count
        filesize_info = {'size': size, 'count': count}
        value_tuple.update(filesize_info)

    elif extrinsic_class_code == 'ALE':
        # Use the size of the character string payload for the tuple size (no repeat count)
        filesize_info = {'size': len(payload.encode('utf-8')), 'count': 0}
        value_tuple.update(filesize_info)

    elif extrinsic_class_code == 'XMP':
        filesize_info = {'size': len(payload.encode('utf-8')), 'count': 0}
        value_tuple.update(filesize_info)

    # Add the payload tp the extrinsic metadata class instalce
    value_tuple.update({'value': payload})
    instance['value'] = [value_tuple]

    # Change to include file information after ST 2072-7 is updated to include file information for MXF and DMCVT metadata
    if fileinfo and metadata_class_tag not in ['MXFD', 'DMCT']:
        # Add optional file information to the metadata class instance
        file_info = [{'tag': tag, 'type': 'c', 'size': len(value), 'count': 0, 'value': value} for (tag, value) in fileinfo.items()]
        instance['value'] += file_info

    return instance


def create_dark_metadata_class(payload, input_pathname=None, args=None):
    """Return a dark metadata class instance."""

    # The default identification code is a FOURCC
    dark_identification_code = 'TEST'
    dark_identification_type = 'F'
    dark_identification_string = 'Dark metadata test case'

    if args and args.guid:
        # Use the specified GUID as the identification code
        #dark_identification_code = uuid.uuid4().hex
        dark_identification_code = args.guid
        dark_identification_type = 'G'

    if input_pathname:
        dark_identification_string += ": %s" % os.path.split(input_pathname)[1]

    if args.extra:
        # Read extra information from a file in CSV format
        extra_info_dict = read_file_information(args.extra)
        #if args.debug: print(extra_info_dict)

        # The key into the extra information is the input fielname without the extension
        extra_info_key = os.path.splitext(os.path.split(input_pathname)[1])[0]
        #if args.debug: print(extra_info_key, extra_info_dict)

        # Get the extra information for the original input file used to create the metadata test case
        extra_file_info = extra_info_dict.get(extra_info_key, None)
    else:
        extra_file_info = None

    if dark_identification_type == 'F':
        dark_identification_code_size = 4
    elif dark_identification_type == 'G':
        dark_identification_code_size = 16
    else:
        print("Unknown dark identification code type: %c" % dark_identification_type)

    # The data type of the vendor indentification code has a repeat coount while the other tuples do not
    instance = {
        'tag': 'DARK', 'type': 'E', 'value': [
            {'tag': 'VENI', 'type': dark_identification_type, 'size': dark_identification_code_size, 'count': 1, 'value': dark_identification_code},
            {'tag': 'VENS', 'type': 'c', 'size': len(dark_identification_string), 'value': dark_identification_string},
        ]
    }

    value_tuple = {'tag': 'VEND', 'type': 'B'}

    if extra_file_info:
        value_tuple.update({'size': extra_file_info['size'], 'count': 1})

    value_tuple.update({'value': payload})

    instance['value'].append(value_tuple)

    return instance


def create_json_chunk(instance, tag):
    """Return a dictionary for the metadata chunk and its payload."""
    return {'tag': tag, 'value': instance}


def output_json_metadata(metadata, args):
    """Output the JSON metadata representation to the terminal windows or a file."""

    if args.debug: print("Output JSON metadata: ", metadata)

    if args and args.output:
        # Output the metadata in JSON for to the specified file
        assert(file_extension(args.output) == 'json')
        with open(args.output, 'w') as output:
            output.write(json.dumps(metadata, indent=2) + '\n')
    else:
        # Output the metadata in JSON format to the standard output
        #output = sys.stdout.buffer if sys.version_info.major == 3 else sys.stdout
        #output.write(json.dumps(metadata, indent=2) + '\n')
        print(json.dumps(metadata, indent=2))


def output_xmp_metadata(payload, size, args, fileinfo=None):
    """Output a string of XMP metadata as an XML file."""

    #print("output_xmp_metadata")
    #print(fileinfo)

    # Embed XMP metadata as a CDATA element
    cdata_flag = True

    # Create the root of the XML metadata tree
    root = create_xml_root()

    # This implementation uses large chunks since extrinsic metadata can be very large
    chunk = append_xml_chunk(root, '0x61')

    # Add the element for the extrinsic class instance
    instance = append_xml_class(chunk, 'XMPD')

    # Add the element for the class data
    data = append_xml_data(instance, 'XMPd')

    # Add the size and count attributes
    data.set('size', str(size))

    if cdata_flag:
        # Append the XMP payload string as a CDATA section
        data.text = etree.CDATA(payload)

    else:
        # Convert the XMP payload string into an element tree
        xmp_element_tree = etree.fromstring(payload, parser=etree.XMLParser(recover=True))
        #etree.dump(xmp_element_tree)

        # Add the XMP element tree to the body of the instance
        append_xml_tree(data, xmp_element_tree)

    # Add the optional file info to the element tree
    if fileinfo:
        append_xml_file_info(instance, fileinfo)

    # Output the XMP metadata in XML format
    output_xml_metadata(root, args)


def find_metadata_tuple(metadata, tag):
    """Find a tuple with the specified tag in the internal representation of metadata."""

    for tuple in metadata:
        if tuple['tag'] == tag:
            # Found the tuple with the specified tag
            return tuple
        elif 'value' in tuple and isinstance(tuple['value'], list):
            # Recursively search the tuple value
            result = find_metadata_tuple(tuple['value'], tag)
            if result != None: return result

    return None


def output_dmcvt_metadata(metadata, args, fileinfo=None, filesize=None):
    """Output a color volume transform the its label."""

    # Create the root of the XML metadata tree
    root = create_xml_root()

    # This implementation uses large chunks since extrinsic metadata can be very large
    chunk = append_xml_chunk(root, '0x61')

    # Add the element for the extrinsic class instance
    instance = append_xml_class(chunk, 'DMCT')

    # Add elements for the DMCVT label and data
    for tag in ['CVTS', 'CVTD']:
        tuple = find_metadata_tuple(metadata, tag)
        append_xml_tuple(instance, tuple)

    # Output the DMCVT metadata in XML format
    output_xml_metadata(root, args)


def output_extrinsic_metadata(payload, args, fileinfo=None, filesize=None):
    """Output nested tuples representing an extrinsic metadata class instance in XML format."""

    # Get the tags used in the metadata class instance
    (metadata_class_tag, metadata_value_tag, metadata_value_type) = extrinsic_metadata_tags(args.extrinsic)
    if args.debug: print("output_extrinsic_metadata tag: %s, value: %s, type: %s" % (metadata_class_tag, metadata_value_tag, metadata_value_type))

    # Output the medata in XML format by default
    root = create_xml_root()

    # This implementation uses large chunks since extrinsic metadata can be very large
    chunk = append_xml_chunk(root, '0x61')

    # Add the element for the extrinsic class instance
    instance = append_xml_class(chunk, metadata_class_tag)

    if filesize != None and 'size' in filesize:
        # Adjust the size and count to fit the fields in a tuple with a repeat count
        (size, count) = adjust_file_size_and_count(int(filesize['size']), 1)
    elif args.extrinsic == 'ALE':
        (size, count) = (len(payload), 0)
    else:
        size = None
        count = None

    # Add the tuple for the extrinsic metadata payload
    append_xml_element(instance, metadata_value_tag, metadata_value_type, size, count, payload)

    # Change to include file information after ST 2072-7 is updated to include file information for MXF and DMCVT metadata
    if fileinfo and metadata_class_tag not in ['MXFD', 'DMCT']:

        # Add optional file information to the metadata class instance
        for (tag, value) in fileinfo.items():
            tuple = {'tag': tag, 'type': 'c', 'size': str(len(value)), 'count': '0', 'value': value}
            append_xml_tuple(instance, tuple)

    output_xml_metadata(root, args)


def output_dark_metadata(metadata, args):
    """Output nested tuples representing an extrinsic metadata class instance in XML format."""

    # Output the dark metadata in XML format
    root = create_xml_root()

    # This implementation uses large chunks since dark metadata can potentially be very large
    chunk = append_xml_chunk(root, '0x61')

    # Add the element for the extrinsic class instance
    instance = append_xml_class(chunk, 'DARK')

    for tuple in metadata[0]['value']:
        append_xml_tuple(instance, tuple)

    output_xml_metadata(root, args)


class EncodingCurve():
    """Process encoding curve metadata including the nested parameter tuples."""

    def __init__(self, args=None):
        # List of metadata tags for encoding curve metadata
        self.encoding_curve_tags = ['LOGA', 'GAMA', 'LINR', 'FSLG', 'LOGC', 'PQEC', 'HLGE']

        # Initialize the flag indicating whether an encoding curve is being processed
        self.current_encoding_curve = None

        # Save the template for the output filename
        self.template = args.template
        if args.verbose: print("Template: %s" % self.template)

        # Initalize the verbose output flag
        self.verbose = args.verbose


    def begin(self, metadata_tuple):
        """Output the element that encloses the encoding curve parameters."""

        # Close the encoding curve that was being processed (if any)
        self.close()

        tag = metadata_tuple['tag']

        if self.verbose: print("Begin encoding curve: %s" % tag)
        self.current_encoding_curve = tag

        # Create the root for the new XML tree
        self.root = create_xml_root()

        # Create a small metadata chunk element within the toplevel metadata element
        self.chunk = append_xml_chunk(self.root, '0x4010')

        # Create an intrinsic metadata class instance within the metadata chunk element
        self.instance = append_xml_class(self.chunk, 'CFHD')

        # Add the encoding curve tuple to the intrinsic metadata class instance
        self.tuple = append_xml_tuple(self.instance, metadata_tuple)


    def parameter(self, parameter_tuple):
        """Process a parameter nested in the encoding curve metadata tuple."""
        if self.verbose: print("    Encoding parameter: %s" % parameter_tuple)

        append_xml_tuple(self.tuple, parameter_tuple)


    def close(self):
        """Finish the encoding curve that is being processed."""
        if self.current_encoding_curve:
            if self.verbose: print("Close encoding curve: %s\n" % self.current_encoding_curve)

            # Output the XML tree for the current encoding curve
            write_nested_metadata(self.root, self.current_encoding_curve, self.template)

            # Cleanup references to the elements in the nested metadata tuple
            self.current_encoding_curve = None
            self.root = None
            self.chunk = None
            self.instance = None
            self.tuple = None


    def process(self, metadata_tuple):
        """Process one row of encoding curve metadata."""
        tag = metadata_tuple.get('tag', None)
        if tag == None or tag[0] == '#':
            # Skip this tuple if the row begins with a comment character
            pass
        elif tag in self.encoding_curve_tags:
            # Found the start of a nested encoding curve metadata tuple
            self.begin(metadata_tuple)
        else:
            if self.current_encoding_curve:
                # This metadata tuple is a parameter for the current encoding curve
                self.parameter(metadata_tuple)


class LayerMetadata():
    """Process layer metadata including the nested parameter tuples."""

    def __init__(self, args=None):
        # List of metadata tags for layers
        self.layer_parameter_tags = ['LAYN', 'LAYD']

        # Initalize the verbose output flag
        self.verbose = args.verbose

        # Save the template for the output filename
        self.template = args.template
        if self.verbose: print("Template: %s" % self.template)

        # Create the root for the new XML tree
        self.root = create_xml_root()

        # Create a small metadata chunk element within the toplevel metadata element
        self.chunk = append_xml_chunk(self.root, '0x4010')

        # Create an intrinsic metadata class instance within the metadata chunk element
        self.instance = append_xml_class(self.chunk, 'CFHD')

        # Initialize the flag indicating whether layer metadata is being processed
        self.current_layer_metadata = None


    def begin(self, metadata_tuple):
        """Output the element that encloses the layer metadata tuple."""

        # Close the layer tuple that was being processed (if any)
        #self.close()

        tag = metadata_tuple['tag']

        if self.verbose: print("Begin layer tuple: %s" % tag)
        #self.current_layer_tuple = tag

        # Add the layer metadata tuple to the intrinsic metadata class instance
        self.current_layer_metadata = append_xml_tuple(self.instance, metadata_tuple)


    def parameter(self, parameter_tuple):
        """Process a parameter nested in the layer metadata tuple."""
        if self.verbose: print("    Layer parameter: %s" % parameter_tuple)

        if self.current_layer_metadata is not None:
            append_xml_tuple(self.current_layer_metadata, parameter_tuple)
        else:
            exit("Found layer parameter without layer metadata tuple")


    def close(self):
        """Finish the layer metadata that is being processed."""
        if self.current_layer_metadata is not None:
            if self.verbose: print("Close layer tuple: %s\n" % self.current_layer_metadata)

            # Output the XML tree for the current layer tuple
            write_nested_metadata(self.root, self.current_layer_metadata, self.template)

            # Cleanup references to the elements in the nested metadata tuple
            self.current_layer_metadata = None
            # self.root = None
            # self.chunk = None
            # self.instance = None
            # self.tuple = None


    def process(self, metadata_tuple):
        """Process one row of layer metadata."""
        tag = metadata_tuple.get('tag', None)
        if tag == None or tag[0] == '#':
            # Skip this tuple if the row begins with a comment character
            pass
        elif tag == 'LAYR':
            # Found the start of a nested layer metadata tuple
            self.begin(metadata_tuple)
        elif tag in self.layer_parameter_tags:
            # Found a parameter for the current layer tuple
            self.parameter(metadata_tuple)
        else:
            exit("Unknown layer tag")


class StreamingMetadata():
    """Process streaming metadata including the nested parameter tuples."""

    def __init__(self, args=None):
        # List of metadata tags for streaming data
        self.streaming_parameter_tags = ['DEVC', 'STRM']

        # Initalize the verbose output flag
        self.verbose = args.verbose

        # Save the template for the output filename
        self.template = args.template
        if self.verbose: print("Template: %s" % self.template)

        # Create the root for the new XML tree
        self.root = create_xml_root()

        # Create a small metadata chunk element within the toplevel metadata element
        self.chunk = append_xml_chunk(self.root, '0x4010')

        # Create an intrinsic metadata class instance within the metadata chunk element
        self.instance = append_xml_class(self.chunk, 'GPMF')

        # Initialize the flag indicating whether streaming metadata is being processed
        self.current_streaming_metadata = None


    def begin(self, metadata_tuple):
        """Output the element that encloses the streaming metadata tuple."""

        # Close the streaming tuple that was being processed (if any)
        #self.close()

        tag = metadata_tuple['tag']

        if self.verbose: print("Begin streaming tuple: %s" % tag)
        #self.current_streaming_tuple = tag

        # Add the streaming metadata tuple to the streaming metadata class instance
        self.current_streaming_metadata = append_xml_tuple(self.instance, metadata_tuple)


    def parameter(self, parameter_tuple):
        """Process a parameter nested in the streaming metadata tuple."""
        if self.verbose: print("    Streaming parameter: %s" % parameter_tuple)

        if self.current_streaming_metadata is not None:
            append_xml_tuple(self.current_streaming_metadata, parameter_tuple)
        else:
            exit("Found streaming parameter without streaming metadata tuple")


    def close(self):
        """Finish the streaming metadata that is being processed."""
        if self.current_streaming_metadata is not None:
            if self.verbose: print("Close streaming tuple: %s\n" % self.current_streaming_metadata)

            # Output the XML tree for the current streaming tuple
            write_nested_metadata(self.root, self.current_streaming_metadata, self.template)

            # Cleanup references to the elements in the nested metadata tuple
            self.current_streaming_metadata = None
            # self.root = None
            # self.chunk = None
            # self.instance = None
            # self.tuple = None


    def process(self, metadata_tuple):
        """Process one row of streaming metadata."""
        tag = metadata_tuple.get('tag', None)
        if tag == None or tag[0] == '#':
            # Skip this tuple if the row begins with a comment character
            pass
        elif tag == 'DEVC':
            # Found the start of a device stream
            self.begin(metadata_tuple)

            #TODO: Need to handle device parameters

        elif tag == 'STRM':
            # Found the start of a data stream
            #TODO: Need to next the data stream in the device payload
            pass

        elif tag in self.streaming_parameter_tags:
            # Found a parameter for the current streaming tuple
            self.parameter(metadata_tuple)
        else:
            exit("Unknown streaming tag")


def generate_metadata_tuples(args):
    """Process files of tuples in CSV format into the JSON or XML representation of metadata."""

    # List of metadata tuples with each tuple represented by a dictionary
    metadata = []

    # Collect the metadata tuples from all input files into a list
    for pathname in args.filelist:
        #print(pathname)
        filetype = file_extension(pathname)
        #print(filetype)

        if filetype == 'csv':
            if args.verbose: print("Processing CSV input file: %s" % pathname)
            with open(pathname, encoding='utf-8') as input:
                reader = csv.reader(input, delimiter=',')
                line_count = 0
                for row_values in reader:
                    if line_count == 0:
                        column_names = row_values
                        #print("Column names: %s" % (", ".join(column_names)))
                        line_count += 1
                    else:
                        #print_row_data(column_names, row_values)
                        append_row_data(metadata, column_names, row_values)
                        line_count += 1
                #print("Processed line count: %d" % line_count)
        elif filetype == 'json':
            if args.verbose: print("Processing JSON input file: %s" % pathname)
            with open(pathname) as input:
                metadata += json.load(input)

        else:
            exit("Unknown file type: %s" % filetype)

    #if args.debug: print(metadata)

    # Create the list of metadata tuples
    if args.duplicates:
        #print(metadata)
        tuple_list = []
        for iteration in range(args.count):
            # Metadata lists with duplicate tuples are always randomized
            index = random.randrange(len(metadata))
            tuple_list.append(metadata[index])
    elif args.randomize:
        # Randomize the list of tuples
        count = len(metadata)
        tuple_list = random.sample(metadata, count)
    else:
        # Output the list of tuples in the same order as input
        tuple_list = metadata

    # Partition the metadata tuples into chunks
    chunk_count = args.chunks
    chunk_length = int((len(tuple_list) + chunk_count - 1)/chunk_count)
    tuple_chunks = [tuple_list[index : index + chunk_length] for index in range(0, len(tuple_list), chunk_length)]

    # Randomize the list of metadata tuples
    #tuple_list =random.sample(metadata, count)

    # Output the metadata tuples as JSON or XML (the default)?
    if args and (args.json or (args.output and file_extension(args.output) == 'json')):

        # Output the metadata tuples in JSON format to a file or the terminal

        # Create the list of metadata chunks and tuples within each chunk
        metadata = []

        # Partition the randomized tuple list into chunks
        for tuple_list in tuple_chunks:
            #print(tuple_list)

            if args.gpmf:
                # Create a streaming data (time series) metadata class instance
                instance = create_json_class(tuple_list, 'GPMF')

                # Create a large metadata chunk containing the metadata class instance
                chunk = create_json_chunk(instance, '0x61')

            else:
                # Create an intrinsic metadata class instance
                instance = create_json_class(tuple_list, 'CFHD')

                # Create a small metadata chunk containing the metadata class instance
                chunk = create_json_chunk(instance, '0x4010')

            # Append the chunk to the JSON metadata
            metadata.append(chunk)

        output_json_metadata(metadata, args)

    else:
        # Output the metadata tuples in XML format to a file or the terminal

        # Create the root metadata element
        root = create_xml_root()

        # Partition the randomized tuple list into chunks
        for tuple_list in tuple_chunks:
            #print(tuple_list)

            # Create a small metadata chunk element within the toplevel metadata element
            chunk = append_xml_chunk(root, '0x4010')

            # Assume this is an intrinsic metadata class instance
            class_tag = 'CFHD'
            # if args:
            #     if args.extrinsic: class_tag = args.extrinsic
            #     if args.gpmf: class_tag = 'GPMF'

            # Create a metadata class instance within the metadata chunk element
            if args.verbose: print("Creating metadata class instance: %s" % class_tag)
            instance = append_xml_class(chunk, class_tag)

            # Add the list of metadata tuples to the class instance
            for tuple in tuple_list:
                #if args.debug: print("Appending XML tuple: %s" % tuple)

                # Is this a nested metadata tuple?
                if 'value' in tuple and type(tuple['value']) is list:

                    if args.verbose: print("Processing nested metadata tuple: %s" % tuple['tag'])

                    # Create an element for the tuple without the value in the tuple
                    parent_tuple = {key: value for (key, value) in tuple.items() if key != 'value'}
                    element = append_xml_tuple(instance, parent_tuple)

                    # Add each value as a child of the tuple element
                    for child in tuple['value']:
                        #if args.debug: print("Adding child element to nested metadata tuple: %s" % child['tag'])
                        append_xml_tuple(element, child)

                else:
                    append_xml_tuple(instance, tuple)

        output_xml_metadata(root, args)


def generate_encoding_metadata(args):
    """Generate encoding curve test cases."""

    # The input files contain test data for encoding curve metadata
    for pathname in args.filelist:
        if args.debug: print(pathname)
        filetype = os.path.splitext(pathname)[1][1:]
        if args.debug: print(filetype)

        encoding_curve = EncodingCurve(args)

        if filetype == 'csv':
            with open(pathname, encoding='utf-8') as input:
                reader = csv.reader(input, delimiter=',')
                line_count = 0
                for row_values in reader:
                    if line_count == 0:
                        column_names = row_values
                        if args.debug: print("Column names: %s" % (", ".join(column_names)))
                        line_count += 1
                    else:
                        metadata_tuple = dict(zip(column_names, row_values))
                        encoding_curve.process(metadata_tuple)
                        line_count += 1

                if args.debug: print("Processed line count: %d" % line_count)

            # Finish the encoding curve that is being processed
            encoding_curve.close()

        # elif filetype == 'json':
        #     with open(pathname) as input:
        #         for tuple in json.load(input):
        #             print(tuple)

        else:
            exit("Unknown file type: %s" % filetype)


def generate_layer_metadata(args):
    """Generate test data for layers."""

    # The input files contain test data for layer metadata
    for pathname in args.filelist:
        if args.debug: print(pathname)
        filetype = os.path.splitext(pathname)[1][1:]
        if args.debug: print(filetype)

        layer_metadata = LayerMetadata(args)

        if filetype == 'csv':
            with open(pathname, encoding='utf-8') as input:
                reader = csv.reader(input, delimiter=',')
                line_count = 0
                for row_values in reader:
                    if line_count == 0:
                        column_names = row_values
                        if args.debug: print("Column names: %s" % (", ".join(column_names)))
                        line_count += 1
                    else:
                        metadata_tuple = dict(zip(column_names, row_values))
                        layer_metadata.process(metadata_tuple)
                        line_count += 1

                if args.debug: print("Processed line count: %d" % line_count)

        else:
            exit("Unknown file type: %s" % filetype)

        # Finish processing the file(s) of layer metadata
        layer_metadata.close()


def generate_streaming_metadata(args):
    """Generate test data for streaming data."""

    # The input files contain test data for streaming metadata
    for pathname in args.filelist:
        if args.debug: print(pathname)
        filetype = os.path.splitext(pathname)[1][1:]
        if args.debug: print(filetype)

        if filetype == 'csv':

            #TODO: This class does not appear to handle doubly nested metadata (data within stream within device)
            streaming_metadata = StreamingMetadata(args)

            # Open the CSV fle of streaming GPMF metadata with an explicit encoding to handle special characters in the file
            #with open(pathname, encoding = 'ISO-8859-1') as input:
            with open(pathname, encoding='utf-8') as input:
                reader = csv.reader(input, delimiter=',')
                line_count = 0
                for row_values in reader:

                    if line_count == 0:

                        # The first line is the list of tuple attribute names
                        column_names = row_values
                        if args.debug: print("Column names: %s" % (", ".join(column_names)))
                        line_count += 1

                    else:

                        # Create and process the metadata tuple
                        metadata_tuple = dict(zip(column_names, row_values))

                        # Skip spurious tuples in the metadata from Karma drones
                        if metadata_tuple['tag'] in ['sdeg', 'drad']:
                            print("Found bad karma tag: ", metadata_tuple['tag'])
                            continue

                        streaming_metadata.process(metadata_tuple)

                        line_count += 1

                if args.debug: print("Processed line count: %d" % line_count)

            # Finish processing the file(s) of streaming metadata
            streaming_metadata.close()

        elif filetype == 'json':
            with open(pathname) as input:
                metadata = json.load(input)

            # Walk the JSON tree to create an XML tree

            # Create the root metadata element
            root = create_xml_root()

            # Create a large metadata chunk element
            chunk = append_xml_chunk(root, '0x61')

            # Create a streaming metadata class instance
            class_tag = 'GPMF'
            if args.verbose: print("Creating metadata class instance: %s" % class_tag)
            instance = append_xml_class(chunk, class_tag)

            # Process each device tuple in the streaming data
            for device in metadata:

                # Create a device element
                #device_tuple = {key: value for (key, value) in device.items() if key != 'value'}
                device_tuple = {'tag': device['tag']}
                device_element = append_xml_tuple(instance, device_tuple)

                # Process each metadata tuple in the device
                for tuple in device['value']:

                    # Found a stream nested in the device tuple?
                    if tuple['tag'] == 'STRM':
                        #stream_tuple = {key: value for (key, value) in tuple.items() if key != 'value'}
                        stream_tuple = {'tag': tuple['tag']}
                        stream_element = append_xml_tuple(device_element, stream_tuple)

                        # Add each data tuple to the stream
                        for data_tuple in tuple['value']:

                            #if args.debug: print(data_tuple)

                            # Skip spurious tuples in the metadata from Karma drones
                            if data_tuple['tag'] in ['sdeg', 'drad']:
                                print("Found bad karma tag: ", data_tuple['tag'])
                                continue

                            #print("Data tuple tag: ", data_tuple['tag'])

                            #if data_tuple['type'] == 'f' and data_tuple['size'] == '4' and data_tuple['count'] == '1':
                            if data_tuple['type'] == 'f':

                                # Normalize the representation of floating-point values
                                if args.debug: print("Normalizing tuple:", data_tuple)

                                normalized_value = []

                                for number in data_tuple['value'].split():

                                    match = re.match(float_pattern, number)
                                    if match:
                                        old_value = number
                                        number = match.group(1)
                                        #print("Old value: {old}, new value:{new}".format(old=old_value, new=number))

                                    match = re.match(integer_pattern, number)
                                    if match:
                                        old_value = number
                                        number = match.group(1) + '.0'
                                        #print("Old value: {old}, new value:{new}".format(old=old_value, new=number))

                                    normalized_value.append(number)

                                data_tuple['value'] = ' '.join(normalized_value)

                            # if data_tuple['type'] == 'u':

                            #     # Adjust the length of Unicode character strings to be the number of bytes
                            #     string = data_tuple['value']
                            #     size = len(string.encode('utf-8'))

                            #     if args.debug: print("Unicode length: {length}, size: {size}".format(length=data_tuple['size'], size=size))

                            #     data_tuple['size'] = str(size)

                            if data_tuple['type'] == 'u':
                                if args.debug: print("Unicode string size: %d, value: \"%s\"" % (data_tuple['size'], data_tuple['value']))

                            append_xml_tuple(stream_element, data_tuple)

                    else:
                        # This tuple must be a device parameter
                        append_xml_tuple(device_element, tuple)

            output_xml_metadata(root, args)

        else:
            exit("Unknown file type: %s" % filetype)


def read_file_information(pathname):
    """Read additional information from a file in JSON or CSV format."""
    filetype = file_extension(pathname)

    if filetype == 'json':

        # Read the table of file information from the specified file in JSON format
        if os.path.isfile(pathname):
            with open(pathname) as input:
                file_info_dict = json.load(input)
                return file_info_dict
        else:
            exit("Could not read file information: %s" % pathname)


    if filetype == 'csv':

        # Read auxiliary information from the specified file in CSV format
        if os.path.isfile(pathname):
            extra_info_dict = {}
            with open(pathname, encoding='utf-8') as input:
                reader = csv.reader(input, delimiter=',')
                line_count = 0
                for row_values in reader:
                    if line_count == 0:
                        column_names = row_values
                        #print("Column names: %s" % (", ".join(column_names)))
                        line_count += 1
                    else:
                        # Read the extra information for one file
                        extra_file_info = dict(zip(column_names, row_values))

                        # Index the information by filename (without the file extension)
                        if 'filename' in extra_file_info:
                            extra_info_key = os.path.splitext(extra_file_info['filename'])[0]
                            extra_info_dict[extra_info_key] = extra_file_info
                return extra_info_dict
        else:
            exit("Could not read extra information: %s" % pathname)

        return None


def generate_extrinsic_metadata(args):
    """Generate extrinsic metadata for the specified metadata class."""

    input_pathname = args.filelist[0]
    input_filetype = file_extension(input_pathname)

    if args.fileinfo:
        # Read the file information from a file in JSON format
        file_info_dict = read_file_information(args.fileinfo)

        # The key into the file information is the input fielname without the extension
        file_info_key = os.path.splitext(os.path.split(input_pathname)[1])[0]
        #if args.debug: print(file_info_key, file_info_dict)

        # Get the file information for the original input file used to create the metadata test case
        input_file_info = file_info_dict[file_info_key]

        # Skip file information that is not used for extrinsic metadata
        input_file_info = {key: input_file_info[key] for key in ['PATH', 'FCDT', 'FMDT']}

    else:
        # Use the file information for the input file
        input_file_info = {
            'PATH' : file_resource_identifier(input_pathname),
            'FCDT' : file_creation_time(input_pathname),
            'FMDT' : file_modification_time(input_pathname)
        }

    if args.debug: print(input_file_info)


    if args.extra:
        # Read extra information from a file in CSV format
        extra_info_dict = read_file_information(args.extra)
        #if args.debug: print(extra_info_dict)

        # The key into the extra information is the input fielname without the extension
        extra_info_key = os.path.splitext(os.path.split(input_pathname)[1])[0]
        #if args.debug: print(extra_info_key, extra_info_dict)

        # Get the extra information for the original input file used to create the metadata test case
        extra_file_info = extra_info_dict.get(extra_info_key, None)

    else:
        extra_file_info = None

    if args.debug: print(extra_file_info)


    # Set the output filetype even if the output is not written to a file
    if args and args.output:
        output_pathname = args.output
        output_filetype = file_extension(output_pathname)
    elif args and args.json:
        output_filetype = 'json'
    else:
        # Output metadata in XML format by default
        output_filetype = 'xml'


    if output_filetype == 'json':

        # This code has only been tested for converting XMP to JSON
        #assert input_filetype == 'xmp'

        # Read the payload as a text string from the input file
        payload = read_extrinsic_payload(args.extrinsic, input_pathname)

        # Create an extrinsic metadata class instance as nested tuples
        instance = create_extrinsic_class(args.extrinsic, payload, input_file_info, extra_file_info)
        #print("Instance: %s" % instance['value'])

        # Output the extrinsic metadata instance in JSON format
        output_json_metadata([instance], args)

    elif output_filetype == 'xml':

        # Get the tags used in the metadata class instance
        #(metadata_class_tag, metadata_value_tag, metadata_value_type) = extrinsic_metadata_tags(args.extrinsic)

        if input_filetype == 'json':

            # Read the payload from the input file
            #payload = read_extrinsic_payload(metadata_class_tag, input_pathname)

            with open(input_pathname) as input:
                metadata = json.load(input)

            # Change to not handle DMCVT as a special case after ST 2073-7 is revised to add file information to this metadata category
            if args.extrinsic == 'DMCVT':
                # Output the DMCVT universal label and the value for the color transforn
                if args.debug: print("DMCVT metadata:", metadata)
                output_dmcvt_metadata(metadata, args, input_file_info, extra_file_info)

            else:

                # The extrinsic payload is the value string in the first child of the metadata tuple
                tuple = metadata[0]
                tuple = tuple['value'][0]
                payload = tuple['value']

                if args.extrinsic == 'XMP':
                    size = tuple.get('size', len(payload.encode('utf-8')))
                    output_xmp_metadata(payload, size, args, input_file_info)
                else:
                    output_extrinsic_metadata(payload, args, input_file_info, extra_file_info)

        elif input_filetype == 'xmp':

            # Read the payload as a text string from the input file
            payload = read_extrinsic_payload(args.extrinsic, input_pathname)

            output_xmp_metadata(payload, input_file_info, args)

        else:
            exit("Invalid extrinsic metadata input file type: %s" % input_filetype)

    else:
        exit("Unknown extrinsic metadata output file type: %s" % output_filetype)


def generate_dark_metadata(args):
    """Generate dark metadata in the format specified by the command-line arguments."""

    input_pathname = args.filelist[0]
    input_filetype = file_extension(input_pathname)

    # Set the output filetype even if the output is not written to a file
    if args and args.output:
        output_pathname = args.output
        output_filetype = file_extension(output_pathname)
    elif args and args.json:
        output_filetype = 'json'
    else:
        # Output metadata in XML format by default
        output_filetype = 'xml'

    if input_filetype == 'b64':
        with open(input_pathname) as input:
            payload = input.read().strip()

        if output_filetype == 'json':

            # Create a dark metadata class instance as nested tuples
            instance = create_dark_metadata_class(payload, input_pathname, args)
            #print("Instance: %s" % instance['value'])

            # Output the extrinsic metadata instance in JSON format
            output_json_metadata([instance], args)

        elif output_filetype == 'xml':
            exit("Dark metadata XML output not implemented")

        else:
            exit("Unknown dark metadata output file type: %s" % output_filetype)

    elif input_filetype == 'json':
        with open(input_pathname) as input:
            metadata = json.load(input)

        assert output_filetype == 'xml'

        output_dark_metadata(metadata, args)

    else:
        exit("Invalid dark metadata input file type: %s" % input_filetype)


def generate_multiclass_metadata(args):
    """Merge multiple metadata class instances into a single test case."""

    # Merge the class instances from all input files into a single list
    metadata = []
    for pathname in args.filelist:
        assert(file_extension(pathname) == 'json')
        tuple_list = read_file_information(pathname)

        filename = os.path.split(pathname)[1]
        if filename.startswith('simple'):
            # Wrap the tuple list in a CFHD class instance
            #metadata += [{'tag': 'CFHD', 'type': 'E', 'value': tuple_list}]
            instance = create_json_class(tuple_list, 'CFHD')
            metadata += [instance]
        else:
            metadata += tuple_list

    filetype = file_extension(args.output)

    if filetype == 'json':
        # Output the merged metadata class instances in JSON format
        output_json_metadata(metadata, args)

    elif filetype == 'xml':
        # Output the merged metadata class instances in a single metadata chunk
        root = create_xml_root()
        chunk = append_xml_chunk(root, '0x61')

        for instance in metadata:
            append_nested_tuple(chunk, instance)

        output_xml_metadata(root, args)

    else:
        exit("Unsupported multiclass file type: %s" % filetype)


def merge_metadata_files(args):
    """Merge one or more files of metadata tuples into a single file."""

    # Initialize the type of input files (all files must be the same type)
    input_type = None

    # Check that all filetypes are either XML or JSON
    for pathname in args.filelist:
        #print(pathname)
        filetype = os.path.splitext(pathname)[1][1:]
        #print(filetype)

        if input_type and filetype != input_type:
            exit("Inconsistent file types")

        if filetype in ['xml', 'json']:
            input_type = filetype
        else:
            exit("Unknown file type: %s" % filetype)

    if input_type == 'xml':

        # Create the root metadata element
        root = create_xml_root()

        # Collect the metadata tuples from all input files into a single metadata element
        for pathname in args.filelist:

            metadata = etree.parse(pathname).getroot()
            #metadata = etree.parse(pathname)
            #print(metadata)

            for chunk in metadata.findall('metadata:chunk', namespace):
                #print(chunk, chunk.attrib)
                append_metadata_chunk(root, chunk, args)

        output_xml_metadata(root, args)

    elif input_type == 'json':

        # Initialize a list for the metadata tuples read from the input files
        metadata = []

        # Collect the metadata tuples from all input files into a single metadata element
        for pathname in args.filelist:

            with open(pathname) as input:
                metadata += json.load(input)

        # Randomize the tuple list (with and without duplicates)
        if args.duplicates:
            tuple_list = []
            for iteration in range(args.count):
                # Metadata lists with duplicate tuples are always randomized
                index = random.randrange(len(metadata))
                tuple_list.append(metadata[index])
        elif args.randomize:
            # Randomize the list of tuples
            count = len(metadata)
            tuple_list = random.sample(metadata, count)
        else:
            # Output the list of tuples in the same order as input
            tuple_list = metadata


        output_json_metadata(tuple_list, args)

    else:
        exit("Unknown input type")


def split_metadata_file(args):
    """Split JSON input file(s) into one JSON file per metadata item."""

    # Must have a template for output file names
    if not args.template:
        exit("Must provide a template for output files")

    # Initialize a list for the metadata tuples read from the input files
    metadata = []

    for pathname in args.filelist:
        #print(pathname)
        filetype = file_extension(pathname)
        #print(filetype)

        if filetype != 'json':
            exit("Not a JSON input file type: %s" % filetype)

        with open(pathname) as input:
            metadata += json.load(input)


    for tuple in metadata:
        output_pathname = args.template % tuple['tag']
        if args.verbose: print("Output filename: %s" % output_pathname)

        with open(output_pathname, 'w') as output:
            output.write(json.dumps([tuple], indent=2) + '\n')


def convert_input_tuples(args):
    """Convert CSV input format for simple metadata tuples to JSON format."""

    # List of metadata tuples with each tuple represented by a dictionary
    metadata = []

    # Collect the metadata tuples from all input files into a list
    for pathname in args.filelist:
        #print(pathname)
        filetype = file_extension(pathname)
        #print(filetype)

        if filetype == 'csv':
            with open(pathname, encoding='utf-8') as input:
                reader = csv.reader(input, delimiter=',')
                line_count = 0
                for row_values in reader:
                    if line_count == 0:
                        column_names = row_values
                        #print("Column names: %s" % (", ".join(column_names)))
                        line_count += 1
                    else:
                        # Skip spurious tuples in the metadata from Karma drones
                        metadata_tuple = dict(zip(column_names, row_values))
                        if metadata_tuple['tag'] in ['sdeg', 'drad']:
                            print("Found bad karma tag: ", metadata_tuple['tag'])
                            continue

                        #print_row_data(column_names, row_values)
                        append_row_data(metadata, column_names, row_values)

                        line_count += 1

                #print("Processed line count: %d" % line_count)
        else:
            exit("Unknown file type: %s" % filetype)

    #print(metadata)

    # Output the list of tuples without nesting in chunk and class elements
    output_json_metadata(metadata, args)


def convert_encoding_tupes(args):
    """Convert CSV input format for encoding metadata tuples to JSON format."""

    # TODO: Use class static data to store the encoding curve tags in one place only
    encoding_curve_tags = ['LOGA', 'GAMA', 'LINR', 'FSLG', 'LOGC', 'PQEC', 'HLGE']

    encoding_metadata_tuples = []

    encoding_curve_tuple = None

    for pathname in args.filelist:
        #print(pathname)
        filetype = file_extension(pathname)
        #print(filetype)

        if filetype == 'csv':
            with open(pathname, encoding='utf-8') as input:
                reader = csv.reader(input, delimiter=',')
                line_count = 0
                for row_values in reader:
                    if line_count == 0:
                        column_names = row_values
                        if args.debug: print("Column names: %s" % (", ".join(column_names)))
                        line_count += 1
                    else:
                        metadata_tuple = dict(zip(column_names, row_values))
                        #print(metadata_tuple)
                        tag = metadata_tuple.get('tag', None)
                        if tag == None or tag[0] == '#':
                            # Skip this tuple if the row begins with a comment character
                            pass

                        elif tag in encoding_curve_tags:
                            # Already processing an encoding curve metadata tuple?
                            if encoding_curve_tuple:
                                # Output the current encoding curve metadata tuple
                                encoding_metadata_tuples.append(encoding_curve_tuple)

                            # Found the start of a nested encoding curve metadata tuple
                            encoding_curve_tuple = metadata_tuple
                            if args.debug: print("Encoding curve: %s" % encoding_curve_tuple)

                        else:
                            if encoding_curve_tuple:
                                # This metadata tuple is a parameter for the current encoding curve metadata tuple
                                if args.debug: print("Encoding parameter: %s" % metadata_tuple)
                                if 'value' in encoding_curve_tuple:
                                    encoding_curve_tuple['value'].append(metadata_tuple)
                                else:
                                    encoding_curve_tuple['value'] = [metadata_tuple]
                            else:
                                exit("Found encoding curve parameter before encoding curve tuple")

                        line_count += 1

                if encoding_curve_tuple:
                    # Output the current encoding curve metadata tuple
                    encoding_metadata_tuples.append(encoding_curve_tuple)

                if args.debug: print("Processed line count: %d" % line_count)

        else:
            exit("Unknown file type: %s" % filetype)

    # Finish the encoding curve that is being processed
    output_json_metadata(encoding_metadata_tuples, args)


def convert_layer_tuples(args):
    """Convert CSV input format for layer metadata tuples to JSON format."""

    # List of metadata tags for layers
    layer_parameter_tags = ['LAYN', 'LAYD']

    layer_metadata_list = []

    # Initialize the flag indicating whether layer metadata is being processed
    current_layer_tuple = None

    for pathname in args.filelist:
        #print(pathname)
        filetype = file_extension(pathname)
        #print(filetype)

        if filetype == 'csv':
            with open(pathname, encoding='utf-8') as input:
                reader = csv.reader(input, delimiter=',')
                line_count = 0
                for row_values in reader:
                    if line_count == 0:
                        column_names = row_values
                        if args.debug: print("Column names: %s" % (", ".join(column_names)))
                        line_count += 1
                    else:
                        metadata_tuple = dict(zip(column_names, row_values))
                        #print(metadata_tuple)
                        tag = metadata_tuple.get('tag', None)
                        if tag == None or tag[0] == '#':
                            # Skip this tuple if the row begins with a comment character
                            pass

                        elif tag in layer_parameter_tags:
                             # Adjust the size and count
                            if metadata_tuple['type'] == 'c':
                                metadata_tuple['size'] = str(len(metadata_tuple['value']))
                                metadata_tuple['count'] = '0'

                            if current_layer_tuple:
                                # This metadata tuple is a parameter for the current layer metadata tuple
                                if args.debug: print("Layer metadata parameter: %s" % metadata_tuple)

                                # Append the metadata tuple to the current layer metadata tuple value or initialize the value
                                if 'value' in current_layer_tuple:
                                    current_layer_tuple['value'].append(metadata_tuple)
                                else:
                                    current_layer_tuple['value'] = [metadata_tuple]
                            else:
                                exit("Found layer parameter before layer metadata tuple")

                        elif tag == 'LAYR':
                            if current_layer_tuple:
                                # Output the current layer metadata tuple
                                layer_metadata_list.append(current_layer_tuple)

                            # Found the start of a new layer metadata tuple
                            current_layer_tuple = metadata_tuple
                            if args.debug: print("Layer metadata tuple: %s" % current_layer_tuple)

                        else:
                            exit("Unknown layer tag: %s" % tag)

                        line_count += 1

                if args.debug: print("Processed line count: %d" % line_count)

        else:
            exit("Unknown file type: %s" % filetype)

    if current_layer_tuple:
        layer_metadata_list.append(current_layer_tuple)

    # Finish the encoding curve that is being processed
    output_json_metadata(layer_metadata_list, args)


def convert_streaming_tuples(args):
    """Convert CSV input format for streaming metadata tuples to JSON format."""

    # List of metadata tags for streaming device parameters
    streaming_device_tags = ['DVID', 'DVNM', 'TICK']

    streaming_metadata_list = []

    # Initialize the current device tuple
    current_device_tuple = None

    # Initialize the current stream tuple
    current_stream_tuple = None

    for pathname in args.filelist:
        #print(pathname)
        filetype = file_extension(pathname)
        #print(filetype)

        if filetype == 'csv':
            if args.debug: print("Converting CSV file: %s" % pathname);
            #with open(pathname, newline='', encoding='latin_1') as input:
            with open(pathname, newline='', encoding='utf-8') as input:
                reader = csv.reader(input, delimiter=',')
                line_count = 0
                for row_values in reader:
                    if line_count == 0:
                        column_names = row_values
                        if args.debug: print("Column names: %s" % (", ".join(column_names)))
                        line_count += 1
                    else:
                        #if args.debug: print("Row values: %s" % row_values);
                        metadata_tuple = dict(zip(column_names, row_values))
                        if args.debug: print(metadata_tuple)

                        tag = metadata_tuple.get('tag', None)
                        if tag == None or tag[0] == '#':
                            # Skip this tuple if the row begins with a comment character
                            pass

                        elif tag in ['sdeg', 'drad']:
                            print("Found bad Karma tag: ", metadata_tuple['tag'])
                            pass

                        elif tag in streaming_device_tags:
                            if current_device_tuple:
                                # This metadata tuple is a parameter for the current streaming metadata tuple
                                if args.debug: print("Streaming device parameter: %s" % metadata_tuple)
                                if 'value' in current_device_tuple:
                                    current_device_tuple['value'].append(metadata_tuple)
                                else:
                                    current_device_tuple['value'] = [metadata_tuple]
                            else:
                                exit("Found streaming device parameter before device tuple")

                        elif tag == 'DEVC':
                            if current_device_tuple:
                                if current_stream_tuple:
                                    # Output the current data stream metadata tuple
                                    if 'value' in current_device_tuple:
                                        current_device_tuple['value'].append(current_stream_tuple)
                                    else:
                                        current_device_tuple['value'] = [current_stream_tuple]

                                    # The new device will contain its own data streams
                                    current_stream_tuple = None

                                # Add the current device with its nested parameters and streams to the metadata tuple list
                                streaming_metadata_list.append(current_device_tuple)

                            # Found the start of a new device
                            current_device_tuple = metadata_tuple
                            if args.debug: print("Streaming device tuple: %s" % current_device_tuple)

                        elif tag == 'STRM':
                            if current_stream_tuple:
                                # Output the current streaming metadata tuple
                                if 'value' in current_device_tuple:
                                    current_device_tuple['value'].append(current_stream_tuple)
                                else:
                                    current_device_tuple['value'] = [current_stream_tuple]

                            # Found the start of a new data stream
                            current_stream_tuple = metadata_tuple
                            if args.debug: print("Data stream data tuple: %s" % current_stream_tuple)

                        else:
                            if current_stream_tuple:
                                # Add the streaming data tuple to the current stream
                                if 'value' in current_stream_tuple:
                                    current_stream_tuple['value'].append(metadata_tuple)
                                else:
                                    current_stream_tuple['value'] = [metadata_tuple]
                            else:
                                exit("Found data tuple before stream tuple: %s" % metadata_tuple)

                        line_count += 1

                if args.debug: print("Processed line count: %d" % line_count)

        else:
            exit("Unknown file type: %s" % filetype)

    if current_stream_tuple:
        if current_device_tuple:
            if 'value' in current_device_tuple:
                current_device_tuple['value'].append(current_stream_tuple)
            else:
                current_device_tuple['value'] = [current_stream_tuple]

            current_stream_tuple = None
            streaming_metadata_list.append(current_device_tuple)
            current_device_tuple = None

        else:
            print("No device tuple at end of file")

    # Finish the encoding curve that is being processed
    output_json_metadata(streaming_metadata_list, args)


def generate(args, directory=None):
    """Generate a metadata test case using the arguments parsed from the command line."""

    #print(args)

    if directory:
        # Set the working directory
        os.chdir(directory)

    # Set the random number seed to provide reproduciblity between runs
    if args.seed:
        if args.verbose: print("Setting random seed: %d" % args.seed)
        random.seed(args.seed)

    if args.convert:
        if args.encoding:
            convert_encoding_tupes(args)
        elif args.layers:
            convert_layer_tuples(args)
        elif args.gpmf:
            convert_streaming_tuples(args)
        else:
            convert_input_tuples(args)

    elif args.encoding:
        # Generate encoding curve metadata
        generate_encoding_metadata(args)

    elif args.layers:
        # Generate test data for layer metadata
        generate_layer_metadata(args)

    elif args.gpmf:
        # Generate test data for streaming metadata
        generate_streaming_metadata(args)

    elif args.merge:
        # Merge one or more metadata files into a single file
        merge_metadata_files(args)

    elif args.split:
        split_metadata_file(args)

    elif args.multiclass:
        # Merge multiple metadata class instances into a single test case
        generate_multiclass_metadata(args)

    elif args.extrinsic:
        # The extrinsic argument is the common acronym for the metadata class
        #metadata_class_tag = metadata_class_tag_dict[args.extrinsic.upper()]

        # Output the extrinsic metadata test case in JSON or XML format
        generate_extrinsic_metadata(args)

    elif args.dark:
        # Output the dark metadata test case in JSON or XML format
        generate_dark_metadata(args)

    else:
        # Generate test data for simple intrinsic metadata items
        generate_metadata_tuples(args)


def append_logfile_entry(pathname, argv):
    """Append an entry for this script execution to the log file."""

    with open(pathname, 'a') as logfile:
        # Write a log file entry using the syntax of a Bash command
        entry = "cd %s; %s\n" % (os.getcwd(), ' '.join(argv))
        logfile.write(entry)


if __name__ == '__main__':
    from argparse import ArgumentParser
    parser = ArgumentParser(description='Generate a metadata file in JSON or XML format')
    parser.add_argument('filelist', nargs='*', help='list of input metadata files')
    parser.add_argument('-o', '--output', help='output file in XML format')
    parser.add_argument('-r', '--randomize', action='store_true', help='randomize the output tuples')
    parser.add_argument('-d', '--duplicates', action='store_true', help='allow duplicate output tuples')
    parser.add_argument('-n', '--count', type=int, default=10, help='number of output tuples')
    parser.add_argument('-c', '--chunks', type=int, default=1, help='number of chunks')
    parser.add_argument('-m', '--merge', action='store_true', help='merge multiple XML or JSON files into a single output file')
    parser.add_argument('-e', '--encoding', action='store_true', help='generate encoding curve test cases')
    parser.add_argument('-l', '--layers', action='store_true', help='generate test data for layer metadata')
    parser.add_argument('-g', '--gpmf', action='store_true', help='generate test data for GPMF streaming metadata')
    parser.add_argument('-E', '--extrinsic', help='generate test data for the specified extrinsic metadata class')
    parser.add_argument('-F', '--fileinfo', help='file of information about the original samples used to create the metadata test cases')
    parser.add_argument('-X', '--extra', help='extra information for generating the test case')
    parser.add_argument('-D', '--dark', action='store_true', help='generate test data for dark metadata')
    parser.add_argument('-G', '--guid', help='UUID for the dark metadata identification code (default is a FOURCC)')
    parser.add_argument('-M', '--multiclass', action='store_true', help='merge multiple class instances into a single test case')
    parser.add_argument('-f', '--template', help='filename template for multiple output files')
    parser.add_argument('-j', '--json', action='store_true', help='output metadata in JSON format')
    parser.add_argument('-s', '--split', action='store_true', help='split JSON input file into one file per metadata item')
    parser.add_argument('-S', '--seed', type=int, help='initialize the random number seed')
    parser.add_argument('-L', '--log', help='record each command line for use in creating test scripts')
    parser.add_argument('-R', '--run', help='run the commands recorded in a log file')
    parser.add_argument('-N', '--dryrun', action='store_true', help='print commands that would be run without executing the commands')
    parser.add_argument('-v', '--verbose', action='store_true', help='enable verbose output for monitoring progress')
    parser.add_argument('-z', '--debug', action='store_true', help='enable extra output for debugging')
    parser.add_argument('-C', '--convert', action='store_true', help='convert metadata tuples in CSV format to JSON')

    args = parser.parse_args()

    if args.log:
        generate_logfile_pathname = os.path.join(os.path.split(__file__)[0], args.log)
        append_logfile_entry(generate_logfile_pathname, sys.argv)

    elif generate_logfile_pathname := os.getenv('GENERATE_LOG'):
        append_logfile_entry(generate_logfile_pathname, sys.argv)

    if args.run:
        testing_log_pathname = os.path.join(os.path.split(__file__)[0], args.run)
        with open(testing_log_pathname) as testing_log:
            for line in testing_log.readlines():
                match = re.match(test_pattern, line)
                if match:
                    # Get the working directory and command line
                    directory = match.group(1)
                    command = match.group(2)
                    #if args.debug: print("%s: %s" % (directory, command))

                    if args.dryrun:
                        print("[%s] %s" % (directory, command))
                    else:
                        # Split the command line into an argument vector
                        argv = command.split(' ')
                        if args.debug: print(argv[1:], directory)

                        args = parser.parse_args(argv[1:])
                        if args.debug: print(args)

                        generate(args, directory)

    else:

        # Generate a metadata test case using the arguments parsed from the command line
        generate(args)

