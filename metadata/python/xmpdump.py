#!/usr/bin/env python3
#
# Script to extract the payload from a test case in XML FormatControl

import sys
from lxml import etree
from bs4 import BeautifulSoup


def dump_xmp_payload_lxml(filename):
    """Output the XMP payload of a test case in XML format using LXML."""
    tree = etree.parse(filename)
    node = tree.xpath("//*[local-name() = 'tuple'][@tag='XMPd']")
    payload = node[0][0]
    print(etree.tostring(payload, encoding='ascii', pretty_print=True).decode('ascii'), end='')


def dump_xmp_payload_soup(filename):
    """Output the XMP payload of a test case in XML format using Beautiful Soup."""
    with open(filename) as input:
        soup = BeautifulSoup(input, 'lxml')
        #print(soup)

        payload = soup.find('x:xmpmeta')
        print(payload.prettify())


for filename in sys.argv[1:]:
    #dump_xmp_payload_lxml(filename)
    dump_xmp_payload_soup(filename)

