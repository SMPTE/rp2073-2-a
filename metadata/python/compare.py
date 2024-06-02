#!/usr/bin/env python3
#
# Script to compare two XML files for substantive differences

from lxml import etree
from xmldiff import main, formatting


def compare_xml_trees(args):
    """Compare XML files using lxml to read each file into an XML tree."""

    # Read the first metadata file
    with open(args.filelist[0], 'rb') as file:
        first_tree = etree.parse(file)

    #formatter = formatting.XmlDiffFormatter(normalize=formatting.WS_BOTH, pretty_print=True)
    formatter = formatting.XmlDiffFormatter(normalize=formatting.WS_TAGS, pretty_print=True)
    #formatter = formatting.XMLFormatter(normalize=formatting.WS_TAGS, pretty_print=True)

    for filename in args.filelist[1:]:
        with open(filename, 'rb') as file:
            next_tree = etree.parse(file)

            # Compare the two XML trees
            result = main.diff_trees(first_tree, next_tree, formatter=formatter)
            #print(result)
            if len(result) > 0:
                if args.verbose: print(result)
                return False

    # All files are identical
    return True


def compare_xml_files(args):
    """Compare XML files by passing each filename to xmldiff."""

    # Compare each file with the first file in the list
    first_file = args.filelist[0]

    #formatter = formatting.XmlDiffFormatter(normalize=formatting.WS_BOTH, pretty_print=True)
    formatter = formatting.XmlDiffFormatter(normalize=formatting.WS_TAGS, pretty_print=True)
    #formatter = formatting.XMLFormatter(normalize=formatting.WS_TAGS, pretty_print=True)

    for filename in args.filelist[1:]:
        next_file = filename

        # Compare the two XML trees
        result = main.diff_files(first_file, next_file, formatter=formatter)
        #print(result)
        if len(result) > 0:
            if args.verbose: print(result)
            return False

    # All files are identical
    return True


if __name__ == "__main__":
    # Parse the command line arguments
    from argparse import ArgumentParser

    parser = ArgumentParser(description='Compare metadata files in XML format')
    parser.add_argument('filelist', nargs='+', help='list of metadata files to compare')
    parser.add_argument('-a', '--algorithm', choices=['lxml', 'file'], default='file', help='method for comparing XML files')
    parser.add_argument('-v', '--verbose', action='store_true', help='enable verbose output for debugging')
    parser.add_argument('-z', '--debug', action='store_true', help='enable extra output for debugging')
    parser.add_argument('-q', '--quiet', action='store_true', help='suppress all output to the terminal')
    args = parser.parse_args()
    #print(args)

    # Create a dictionary of command-line arguments and other parameters
    #params = vars(args)

    if args.algorithm == 'lxml':
        result = compare_xml_trees(args)
    else:
        result = compare_xml_files(args)

    if result:
        if args.verbose: print("Identical")
        exit(0)
    else:
        if args.verbose: print("Different")
        exit(1)

