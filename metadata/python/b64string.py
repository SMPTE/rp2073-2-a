#!/usr/bin/env python3
#
# Convert character string that represents binary data to base64

import base64


def encode_rgbalayout(string):
    """Convert the string representing the RGBALayout to base64."""
    if verbose: print("RGBALayout: {0}".format(string))

    byte_array = []
    i = 0

    while i < len(string):
        # Get the code for the pixel component
        code = string[i]
        i += 1

        byte_array.append(ord(code))

        # Get the length of the component in bits
        depth = 0
        while i < len(string) and string[i].isdigit():
            depth = 10 * depth + int(string[i])
            i += 1
        byte_array.append(depth)

    # Pad the byte array with zero bytes
    byte_array += [0] * (16 - len(byte_array))

    if debug: print(byte_array)

    buffer = base64.b64encode(bytearray(byte_array))
    return buffer.decode()


def encode_icc_profile(string):
    """Convert the string representing an ICC Profile to base64."""
    if verbose: print("ICC Profile: {0}".format(string))

    buffer = base64.b64encode(bytearray(string, 'ASCII'))
    return buffer.decode()


conversion_routine = {
    'rgba': encode_rgbalayout,
    'icc': encode_icc_profile
}


if __name__ == '__main__':
    from argparse import ArgumentParser
    parser = ArgumentParser(description='Convert string representation of binary data to base64')
    parser.add_argument('string', nargs=1, help='character string representation of binary data')
    parser.add_argument('-m', '--metadata', choices=['rgba', 'icc'], required=True, help='metadata item represented by the character string')
    parser.add_argument('-v', '--verbose', action='store_true', help='enable verbose output for monitoring progress')
    parser.add_argument('-z', '--debug', action='store_true', help='enable extra output for debugging')

    args = parser.parse_args()

    metadata = args.metadata
    string = args.string[0]
    verbose = args.verbose
    debug = args.debug

    if debug: print(metadata, string, conversion_routine[metadata])

    binary = conversion_routine[metadata](string)

    print(binary)

    