# Status of Metadata Test Cases

Checked samples of the metadata test cases to look for bad syntax or content in the XML files.


## Inspection Results

### 2021-03-16


#### Intrinsic Simple Test Cases

Looked at `simple-rdc4.xml`.

The XML structure looks correct. Checked the size calculation for the first `CFHD` class instance: Okay.
The test cases contains 10 simple tuples, randomized with duplicates. The test case should have duplicates,
but did not see any.

Did not see any duplicates in `simple-rdc1.xml` or `simple-rdc2.xml` or `simple-rdc7.xml`.
None of the test cases have any duplicate metadata tuples.

Modified `simple.rules` to increase the number of tuples in the generated test cases to force the
inclusion of duplicate tuples.


#### Intrinsic Complex Test Cases

Looked at `simple-LOGC-hdr.xml` using Brackets. The structure looks perfect. Brackets shows the
matching start and end elements so easy to see that the structure is correct.


#### Streaming Data Test Cases

Looked at `hero5.xml` and `hero8.xml` test cases. Nesting structure looks good. In the `hero5.xml` file,
the `TICK` tuples are outside the `STRM` tuples but within the `DEVC` tuples.


#### Extrinsic XMP Test Cases

Looked at the `Image1.xml ` test case. The XML was well-formed and the XMP payload was carried in a CDATA element.


#### Extrinsic DPX Test Cases

Look at the `ctest_12bpcfr_BGRA444p_msbf_r2l_mb_rle.xml` test case.
The XML is well-formed. The base64 encoded payload is carried as element text.


#### Extrinsic MXF Test Cases


Looked at the `yuv422_10b_p15.xml` test case.
The XML is well-formed. The base64 encoded payload is carried as element text.


#### Extrinsic ACES Test Cases

Looked at the `SonyF35.StillLife.xml` test case.
The XML is well-formed. The base64 encoded payload is carried as element text.


#### Extrinsic ALE Test Cases

Looked at the `aletest-seq1.xml` test case.
The XML is well-formed. The tab-delimited metadata is carried as element text.


#### Extrinsic DMCVT Test Cases

Looked at the `imf_as02_DMCVT.xml` test case.
The XML is well-formed.
The base64 encoded payload is carried as a value attribute.

Consider changing the XML representation to carry the payload as element text
but this test case shows that a base64 encoded payload can be a value attribute
and does not have to be element text.


#### Dark Metadata Test Cases

Looked at the `dark-uuid.xml` test case.
The XML is well-formed.
The opaque payload is carried as a value attribute.


#### Multiclass Metadata Test Cases

Looked at the `multiclass-01.xml` test case.
The XML is well-formed.
Four different metadata classes: `CFHD`, `XMPD`, `MXFD`, and `ACES` with the payloads carried
by the same means as for the individual metadata classes described above.


