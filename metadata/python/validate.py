#!/usr/bin/env python3
#
# Validate a VC-5 Part 7 XML file against the metadata schema.
#
# Parse the file without validation if the validate option is not specified.


#from lxml import etree, objectify
from lxml import etree
#print("running with lxml.etree")


if __name__ == '__main__':
    from argparse import ArgumentParser
    parser = ArgumentParser(description='Parse XML files')
    parser.add_argument('filelist', nargs='+', help='list of XML filenames')

    args = parser.parse_args()

    for pathname in args.filelist:
        with open(pathname) as xmlfile:
            tree = etree.parse(xmlfile)
            root = tree.getroot()
            #print(root.tag)

            # Cleanup the namespace annotations
            #objectify.deannotate(root, cleanup_namespaces=True)

#           # Print the root children and attributes
#           for child in root:
#               #objectify.deannotate(child, cleanup_namespaces=True)
#               print(child.tag)
#               print(child.attrib)

            # Print all elements and attributes in the tree
            for element in root.iter():
                print(element.tag)
                print(element.attrib)
                print(element.text)
                print("")

