---
title: Metadata Test Cases
---

# Introduction

This document is a list of the XML file that are (or will be) listed in the revision to RP 2073-2 for conformance to ST 2073-7.

NOTE: This document is way out of date.


# Metadata Categories

There are four categories of metadata:

* Intrinsic metadata defined in ST 2073-7 Annex B,
* Streaming data defined in ST 2073-7 Annex C,
* Dark metadata defined in ST 2073-7 Annex D,
* Extrinsic metadata defined in other standards as defined in ST 2073-7 Annexes E through J.


# Test Data

Test data is represented as XML per ST 2073-7 Annex A.

The Python script `generate.py` was used to create XML test data files from test date in more convenient formats.

Test data comprise metadata tuples with some combination of the following characteristics:
1. Randomized metadata tuples,
2. Metadata tuples with duplicates,
3. Metadata tuples that include metadata tuples as children of the metadata tuple.


# Naming Conventions

The filename has a suffix comprising the following letter if applicable:

r
: Metadata tuples have been randomized

d
: Metadata tuples may contain duplicates

c<n>
: Metadata tuples are partitioned into <n> metadata chunks where n is from 1 to the number of tuples in the test case
(including duplicates, if any)



# Test Cases

## Intrinsic Metadata (ST 2073-7 Annex B)

### Simple Metadata

simple-c1.xml
: Simple XML file of intrinsic metadata tuples that are not nested, duplicated, or randomized, and do not include streaming data.

intrinsic-rdc1.xml
: All intrinsic metadata tuples that are not nested and do not include streaming data but are randomized and include duplicate tuples.
This test case is intended to verify conformance of a VC-5 implementation to handling duplicate metadata tuples.

simple-c<n>.xml
: A series of 24 files of the same intrinsic metadata tuples that are not nested, duplicated, or randomized, and do not include streaming data,
but are partitioned into n chunks for n from 2 to 25, inclusive, to verify conformance of a VC-5 implementation to handling multiple metadata chunks.

intrinsic-rdc2.xml
: Intrinsic metadata tuples that are not nested and do not include streaming data but are randomized and include duplicate tuples.
This test case is intended to verify conformance of a VC-5 implementation handling duplicate metadata tuples that occur in different chunks.


### Nested Metadata


### Streaming Data (ST 2073-7 Annex C)

streaming-c1.xml
: Streaming data in a single chunk to verify conformance of a VC-5 implementation to ST 2073-7 Annex C.

streaming-c2.xml
: Streaming data partitioned into two chunks to verify conformance of a VC-5 implementation to handling streaming data in multiple chunks.

streaming-c5.xml
: Streaming data partitioned into five chunks to verify conformance of a VC-5 implementation to handling streaming data in multiple chunks.


### Dark Metadata  (ST 2073-7 Annex D)

dark-c1.xml
: Dark metadata in a single chunk to verify conformance of a VC-5 implementation to handling dark metadata.


### XMP (ST 2073-7 Annex E)

extrinsic-xmp-c1.xml
: Extrinsic XMP metadata in a single chunk.

intrinsic-xmp.c1.xml
: Intrinsic metadata tuples and one instance of XMP metadata in single chunk to verify conformance of a VC-5 implementation to handling intrinsic and extrinsic metadata mixed in the same chunk.

### DPX (ST 2073-7 Annex F)


###  (ST 2073-7 Annex G)


###  (ST 2073-7 Annex H)


###  (ST 2073-7 Annex I)


###  (ST 2073-7 Annex J)


# Sources

Some of the Adobe XMP examples may have been cloned from pdfmark [@pdfmark].



# References

[ Adobe-DNG ]: https://helpx.adobe.com/photoshop/digital-negative.html

[ Adobe-XMP ]: https://www.adobe.com/products/xmp.html

[ XMP-Toolkit-SDK ]: https://github.com/adobe/XMP-Toolkit-SDK/

[ pdfmark ]: https://gitlab.com/crossref/pdfmark

