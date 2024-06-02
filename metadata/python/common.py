#!/usr/bin/env python3
#
# Common functions used by the Python scripts for processing metadata.

import os
from lxml import etree


# Namespace for the XML metadata files
namespace = {'metadata' : 'https://www.vc5codec.org/xml/metadata'}


def create_xml_root():
    """Create the root element for the XML representation of metadata."""
    root = etree.Element('metadata')
    #root.set("xmlns", "https://www.vc5codec.org/xml/metadata")
    root.set("xmlns", namespace['metadata'])
    return root


# Maximum size in a tuple with a repeat count and the maximum repeat count
tuple_repeat_size_max = 255
tuple_repeat_count_max = 65535


# Dictionary that maps a data type to information about the data type.
#
# Data type zero for nested tuples is represented in XML by the character zero.
#
# The size for a scalar with the specified data type is listed only if the data type has a fixed size.
#
data_type_dict = {
    '0': {'repeat': False},
    'c': {'repeat': False},
    'b': {'repeat': True, 'size': 1},
    'B': {'repeat': True, 'size': 1},
    'h': {'repeat': True, 'size': 2},
    'f': {'repeat': True, 'size': 4},
    'd': {'repeat': True, 'size': 8},
    'E': {'repeat': False},
    'F': {'repeat': True, 'size': 4},
    'G': {'repeat': True, 'size': 16},
    'l': {'repeat': True, 'size': 4},
    'L': {'repeat': True, 'size': 4},
    'j': {'repeat': True, 'size': 8},
    'J': {'repeat': True, 'size': 8},
    'P': {'repeat': False},
    'q': {'repeat': True, 'size': 4},
    'Q': {'repeat': True, 'size': 8},
    'r': {'repeat': True, 'size': 4},
    'R': {'repeat': True, 'size': 4},
    's': {'repeat': True, 'size': 2},
    'S': {'repeat': True, 'size': 2},
    'x': {'repeat': False},
    'u': {'repeat': False},
    'w': {'repeat': False},
    'U': {'repeat': True, 'size': 16}
    #'X': {'repeat': 0}
}


# Dictionary that maps nested tuples tags to the list of children in that nested tuple
nested_tuple_dict = {

    # Intrinsic metadata
    'CFHD': {
        'ROWI': {'required': False},
        'COLI': {'required': False},
        'NCOL': {'required': False},
        'NROW': {'required': False},
        'PFMT': {'required': False},
        'ALPH': {'required': False},
        'ALPM': {'required': False},
        'CFAP': {'required': False},
        'CLSY': {'required': False},
        'CMIN': {'required': False},
        'CMAX': {'required': False},
        'AMIN': {'required': False},
        'AMAX': {'required': False},
        'BLKR': {'required': False},
        'WHTR': {'required': False},
        'COLR': {'required': False},
        'CDCS': {'required': False},
        'RATE': {'required': False},
        'SCAL': {'required': False},
        'FRMN': {'required': False},
        'TIMB': {'required': False},
        'TIMD': {'required': False},
        'TIMS': {'required': False},
        'FRMZ': {'required': False},
        'LAYR': {'required': False},
        'ICCP': {'required': False},
        'LOGA': {'required': False},
        'GAMA': {'required': False},
        'LINR': {'required': False},
        'FSLG': {'required': False},
        'LOGC': {'required': False},
        'PQEC': {'required': False},
        'HLGE': {'required': False}
    },
    
    # Encoding curve metadata (ST 2073-7 Annex B.9 and Table 10)
    'LOGA': {
        'LOGb': {'required': True}
        },
    'GAMA': {
        'GAMp': {'required': True}
        },
    #'LINR': [],
    'FSLG': {
        'FSCL': {'required': True}
        },
    'LOGC': {
        'LOGt': {'required': True},
        'LOGa': {'required': True},
        'LOGb': {'required': True},
        'LOGc': {'required': True},
        'LOGd': {'required': True},
        'LOGe': {'required': True},
        'LOGf': {'required': True}
        },
    #'PQEC': [],
    #'HLGE': [],

    # Layer metadata (ST 2073-7 Annex B.13)
    'LAYR': {
        'LAYN': {'required': True},
        'LAYD': {'required': True}
    },

    # Streaming metadata (ST 2073-7 Annex C)
    'GPMF': {
        'DEVC': {'required': True}
    },

    'DEVC': {
        'DVID': {'required': False},
        'DVNM': {'required': False},
        'TICK': {'required': False},
        'STRM': {'required': True}
    },

    'STRM': {
        'STID': {'required': False},
        'STNM': {'required': False},
        'SCAL': {'required': False},
        'SIUN': {'required': False},
        'UNIT': {'required': False},
        'TIMO': {'required': False},
        'TYPE': {'required': False},
        'ACCL': {'required': False},
        'GYRO': {'required': False},
        'MTRX': {'required': False},
        'ORIN': {'required': False},
        'ORIO': {'required': False},
        'STMP': {'required': False},
        'TMPC': {'required': False},
        'TSMP': {'required': False},
        'GPS5': {'required': False},
        'GPSF': {'required': False},
        'GPSP': {'required': False},
        'GPSU': {'required': False},
        'ISOG': {'required': False},
        'MAGN': {'required': False},
        'SHUT': {'required': False},
        'EMPT': {'required': False},
        'FCNM': {'required': False},
        'FWVS': {'required': False},
        'ISOE': {'required': False},
        'WBAL': {'required': False},
        'WRGB': {'required': False},
        'MFGI': {'required': False},
        'UNIF': {'required': False},
        'YAVG': {'required': False},
        'acc1': {'required': False},
        'CORI': {'required': False},
        'VPTS': {'required': False},
        'SROT': {'required': False},
        'IORI': {'required': False},
        'GRAV': {'required': False}        
    },

    # Dark metadata (ST 2073-7 Annex D)
    'DARK': {
        'VENI': {'required': False},
        'VENS': {'required': False},
        'VEND': {'required': True}
    },

    # XMP extrinsic metadata (ST 2073-7 Annex E)
    'XMPD': {
        'XMPd': {'required': True},
        'PATH': {'required': False},
        'FCDT': {'required': False},
        'FMDT': {'required': False}
    },

    # DPX extrinsic metadata (ST 2073-7 Annex F)
    'DPXF': {
        'DPXh': {'required': True},
        'PATH': {'required': False},
        'FCDT': {'required': False},
        'FMDT': {'required': False}
    },

    # MXF Annex F and G essence descriptors (ST 2073-7 Annex G)
    'MXFD' : {
        'MXFd': {'required': True}
    },

    # ACES attributes (ST 2073-7 Annex H)
    'ACES': {
        'ACEh': {'required': True},
        'PATH': {'required': False},
        'FCDT': {'required': False},
        'FMDT': {'required': False}
    },

    # ALE metadata (ST 2073-7 Annex I)
    'ALEM': {
        'ALEd': {'required': True},
        'PATH': {'required': False},
        'FCDT': {'required': False},
        'FMDT': {'required': False}
    },

    # Dynamic Metadata for Color Volume Transform (ST 2073-7 Annex J)
    'DMCT': {
        'CVTS': {'required': True},
        'CVTD': {'required': True}
    }
}


def has_repeat_count(type):
    """Return true of the data type has a repeat count."""
    data_type_info = data_type_dict.get(type, None)
    return data_type_info and data_type_info['repeat']


def file_extension(pathname):
    """Return the file extension as a string without the file extension separator."""
    return os.path.splitext(pathname)[1][1:].lower()


def indentation(level):
    """Return a string to indent a printed line for the specified nesting level."""
    spaces_per_indent = 2
    return ' ' * (spaces_per_indent * level)


# def remove_namespace(tree, namespace):
#     """Remove namespace from all elements in the XML tree using in place computation."""
#     ns = u'{%s}' % namespace
#     for element in tree.getiterator():
#         if element.tag.startswith(ns):
#             # Strip the namespace prefix from the element tag
#             element.tag = element.tag[len(ns):]


def remove_namespace(tree, args=None):
    """Remove namespace from all elements in the XML tree using in place computation."""
    if args and args.debug: print("Remove namespace:", tree)
    for element in tree.getiterator():
        element.tag = etree.QName(element).localname

    etree.cleanup_namespaces(tree)
    if args and args.debug: print("Result namespace:", tree)


def get_attribute(tuple, attribute):
    """Get the specified attribute from the tuple attributes (return None if not present in the tuple)."""
    return tuple.attrib.get(attribute, None)


def compute_padding(size, count=1):
    """Compute the tuple padding required for the specified metadata size and count."""
    assert(size != None)
    size = int(size)
    count = int(count) if count != None else 1
    actual_count = max(count, 1)
    value_size = size * actual_count
    segment_count = int((value_size + 3) / 4)
    payload_size = 4 * segment_count
    payload_padding = payload_size - value_size

    #print(size, actual_count, value_size, payload_size, payload_padding)

    return payload_padding

