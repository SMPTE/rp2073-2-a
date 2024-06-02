#!/usr/bin/env python3
#
# Script to create DMCVT metadata test cases in JSON format

import os
import base64
import json
import csv
from fileinfo import file_resource_identifier, ISO_datetime, file_creation_time, file_modification_time


# Maximum size in a tuple with a repeat count and the maximum repeat count
tuple_repeat_size_max = 255
tuple_repeat_count_max = 65535


def file_extension(pathname):
    """Return the file extension as a string without the file extension separator."""
    return os.path.splitext(pathname)[1][1:].lower()


def printable_string(label):
    """Convert a universal label (binary) to a printable string."""
    string_list = ["%02X" % byte for byte in label]
    string = ' '.join(string_list)
    return string


def print_universal_label(label):
    """Print a universal label (the key in a SMPTE KLV tuple."""
    string = printable_string(label)
    #print(string)


def label_string(label):
    """Convert a universal label (binary) to a string without spaces."""
    #print(label)
    string_list = ["%02X" % byte for byte in label]
    string = str(''.join(string_list))
    #print("Universal label string: {string} ({length})".format(string=string, length=len(string)))
    return string


def BER_length(buffer):
    """Decode the BER encoded length and return the number of bytes used for the BER encoding and the length."""

    #print("Buffer: %02X %02X %02X %02X" % (buffer[0], buffer[1], buffer[2], buffer[3]))
    count = buffer[0]
    #print("Byte count:", count)
    if count & 0x80 == 0x80:
        count &= 0x7F
        length = 0
        for i in range(count):
            #print("Loop length:", length)
            length = (length << 8) | buffer[i+1]
        #print("Loop length: %d, count: %d" % (length, count))
        count += 1
        #print("After increment:", length)
    else:
        length = count
        count = 1

    #print("Return count: %d, length: %d" % (count, length))
    return (count, length)


def update_tuple_list(tuple_list, tag, new_entries):
    """Update the specified tuple in a list of tuples."""
    for tuple in tuple_list:
        if tuple['tag'] == tag:
            tuple.update(new_entries)


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


def create_DMCVT_class(label, length, value, fileinfo=None, filesize=None):
    """Return an extrinsic DMCVT metadata class instance."""

    # Adjust the size and count to fit the fields in a tuple with a repeat count
    (size, count) = adjust_file_size_and_count(length, 1)

    # Create the metadata class instance
    instance = {'tag': 'DMCT', 'type': 'E', 'value':
        [
            {'tag': 'CVTS', 'type': 'U', 'size': 16, 'count': 1, 'value': label},
            {'tag': 'CVTD', 'type': 'B', 'size': str(size), 'count': str(count)}
        ]
    }

    if filesize:
        print("File size information:", filesize)

        # Add extra information about the extrinsic metadata file
        if filesize != None and 'size' in filesize:

            # Adjust the size and count to fit the fields in a tuple with a repeat count
            (size, count) = adjust_file_size_and_count(int(filesize['size']), 1)

            # Update the tuple with the new size and count
            #filesize_info = {'size': size, 'count': count}
            #value_tuple.update(filesize_info)

            extra_info = {'size': size, 'count': count}

            #instance['value'][1].update(extra_info)
            update_tuple_list(instance['value'], 'CVTD', extra_info)

    # Add the DMCVT metadata tuple to the DMCVT class instance
    update_tuple_list(instance['value'], 'CVTD', {'value': value})

    if fileinfo:
        # Add optional file information to the metadata class instance
        extra_info = [{'tag': tag, 'type': 'c', 'value': value} for (tag, value) in fileinfo.items()]
        instance['value'].append(extra_info)

    return instance


# def read_file_information(pathname):
#     """Read the table of file information from the specified file in JSON format."""
#     if os.path.isfile(pathname):
#         with open(pathname) as input:
#             file_info_dict = json.load(input)
#     else:
#         exit("Could not read file information: %s" % pathname)

#     return file_info_dict


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

                        print(extra_file_info)

                        # Index the information by filename (without the file extension)
                        if 'filename' in extra_file_info:
                            extra_info_key = os.path.splitext(extra_file_info['filename'])[0]
                            extra_info_dict[extra_info_key] = extra_file_info
                return extra_info_dict
        else:
            exit("Could not read extra information: %s" % pathname)

        return None


def output_dmcvt_metadata(filename, args):
    """Output DMCVT metadata in JSON format."""

    with open(filename, 'rb') as input:
        buffer = input.read()

        # Get the universal label (the key)
        label = buffer[:16]
        #if args.debug: print_universal_label(label)

        # Convert the universal label to a string
        label = label_string(label)

        (count, length) = BER_length(buffer[16:])
        #if args.debug: print("Count: %d, length: %d" % (count, length))

        # Encode the KLV value in base64
        #value = [hex(x) for x in buffer[16+count:]]
        #print(value)
        value = base64.b64encode(buffer[16+count:])

        # Convert the bytearray to a string in US-ASCII
        value = value.decode('ASCII')

        if args.debug:
            print("Label: {label}".format(label=label))
            print("BER count: {count}, length: {length}".format(count=count, length=length))
            print("Value: {value}".format(value=value))

        # Convert the value to a string
        #value = value.decode('ascii')
        #if args.debug: print(value)

        if args.fileinfo:
            # Read the file information from a file in JSON format
            file_info_dict = read_file_information(args.fileinfo)

            # The key into the file information is the input fielname without the extension
            file_info_key = os.path.splitext(os.path.split(filename)[1])[0]
            #if args.debug: print(file_info_key, file_info_dict)

            # Get the file information for the original input file used to create the metadata test case
            input_file_info = file_info_dict[file_info_key]

            # Skip file information that is not used for extrinsic metadata
            input_file_info = {key: input_file_info[key] for key in ['PATH', 'FCDT', 'FMDT']}

        else:
            # Use the file information for the input file
            input_file_info = {
                'PATH' : file_resource_identifier(filename),
                'FCDT' : file_creation_time(filename),
                'FMDT' : file_modification_time(filename)
            }

        # print("Extra information filename:", args.extra)

        # if args.extra:
        #     # Read extra information from a file in CSV format
        #     extra_info_dict = read_file_information(args.extra)
        #     #if args.debug: print(extra_info_dict)

        #     # The key into the extra information is the input fielname without the extension
        #     extra_info_key = os.path.splitext(os.path.split(filename)[1])[0]
        #     #extra_info_key = os.path.split(filename)[1]
        #     if args.debug: print(extra_info_key, extra_info_dict)

        #     # Get the extra information for the original input file used to create the metadata test case
        #     extra_file_info = extra_info_dict.get(extra_info_key, None)

        #if args.debug: print(input_file_info)

        # Form the dictionary hierachy for the JSON metadata output
        #metadata = create_DMCVT_class(label, length, value, input_file_info)
        # metadata = create_DMCVT_class(label, length, value, None, extra_file_info)
        metadata = create_DMCVT_class(label, length, value, None, None)
        if args.debug: print(metadata)

        string = json.dumps([metadata], indent=2)

        if args.output:
            with open(args.output, 'w') as output:
                output.write(string + '\n')
        else:
            print(string)


if __name__ == '__main__':

    from argparse import ArgumentParser
    parser = ArgumentParser(description='Create DMCVT test cases in JSON format from KLV tuples')
    parser.add_argument('filelist', nargs='*', help='list of files containing DMCVT metadata in binary format')
    parser.add_argument('-F', '--fileinfo', help='file of information about the original samples used to create the metadata test cases')
    parser.add_argument('-X', '--extra', help='extra information for generating the test case')
    #parser.add_argument('-d', '--directory', help='output directory')
    parser.add_argument('-o', '--output', help='output file in JSON format')
    parser.add_argument('-v', '--verbose', action='store_true', help='enable verbose output for monitoring progress')
    parser.add_argument('-z', '--debug', action='store_true', help='enable extra output for debugging')

    args = parser.parse_args()
    #print(args)

    for filename in args.filelist:
        if args.debug: print("Input file: %s" % filename)

        output_dmcvt_metadata(filename, args)

