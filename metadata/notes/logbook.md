# Logbook for work done on VC-5 Part 7 Metadata

Overview of the major modules, list of tasks, and log of work done for each day.


## Modules

1. Tuple Generator (Python Script)
2. XML Parser (C Program)
3. XML Dumper (C Program)
4. Tuple Comparer (Python Script)

May create Python scripts for verifying and managing metadata in various file formats.

May create Python scripts that inject or extract metadata bitstreams into or from existing VC-5 bitstream files.

Regardless, must modify the VC-5 encoder and decoder in the VC-5 codebase.


## Tasks

1. Continue work on the XML parser: clean up the code and add binary output

2. Create a CSV file of all of the metadata tags defined in ST 2073-7 (intrinsic only)

3. Python script to generate XML file of random tuples (with and without duplicates)

4. Finish the CMake build files for the XML parser

5. Test with extrinsic and dark metadata and time series (Annex C)

6. Modify validate.py to generate XML tuples with attributes in a predefined order.

7. Try building the codec with Ninja and CMake to test cross-platform compatibility of the CMake build files.

8. Consider adding JSON as an input file format for generating XML metadata. [DONE]

9. Write up test cases that will be listed in the revision of RP 2073-2.

10. Finish `test_generate.sh` script to test all use cases supported by the `generate.py` script.

Can test `generate.py` by comparing generated files (XML and JSON formats) to the files checked into the repository.
Compare `test_xxx.{xml,json}` against the gold standard `xxx.{xml,json}` from the repository.

11. Copy XML files from $(ROOT)/metadata/python/testcases to $(ROOT)/metadata/testcases

12. Finish DCSV input files for interlaced and HDR layers and convert to JSON.

13. Add new layers testcases to generate.sh script

14. Install Graphviz for diagrams in Doxygen documentation:

<https://spin.atomicobject.com/2013/01/30/making-diagrams-with-graphviz/>



## Notes


### Expat

The expat library was installed using Homebrew on iMac Desktop.

Where and how was the expat library installed on Windows Home and ZenBook?

According to the CMakeLists.txt file in `$HOME/Projects/SMPTE/smpte-10e-vc5\metadata\build\cmake` which was cloned into Windows Home,
the expat library was installed into `C:/Program Files (x86)/Expat 2.2.9` on some Windows machine, probably Windows Professional
on MacBook-Pro.

The expat ibrary is not installed on Windows Home.


### Binary Data

Check that ST 2073-7 allows base64 for binary metadata values.

Not an issue because the value in the bitstream will be a block of binary bytes.
The tools can use whatever conventions that are required.
Metadata type ‘B’ can be an unsigned 8-bit integer (size = 1) or a block of bytes in base62 (size > 1).


### Python XML

<https://lxml.de/>

<https://lxml.de/parsing.html#parsers>

Write xml file using lxml library in Python:
<https://stackoverflow.com/questions/2833185/write-xml-file-using-lxml-library-in-python/2833273>


### Mini-XML

[Mini-XML 3.2 API Reference](https://www.msweet.org/mxml/mxml.html)


## Logbook


### 2019-09-08

Used BBEdit to format the XML in `a2layers.xml` and checked the XML schema using the commmand:

	xmllint --schema metadata.xsd a2layers.xml

There are errors in the use of ComplexType.

Tried the XML and XSD versions provided by John Hurst:

	xmllint --schema metadata-jh.xsd a2layers-jh.xml

and

	xmllint --schema metadata-jh.xsd a2layers-dns.xml

Both versions from John Hurst validate.

Edited the XML files to pass validation by `xmllint`.
Needed to add chunk tag to the schema metadata.

Results of validation:

	a2layers.xml validates
	a3samples.xml:4: element value: Schemas validity error : Element '{https://www.vc5codec.org/xml/metadata}value': This element is not expected. Expected is one of ( {https://www.vc5codec.org/xml/metadata}chunk, {https://www.vc5codec.org/xml/metadata}tuple ).
	a3samples.xml fails to validate
	a4intrinsic.xml validates
	a5extrinsic.xml:6: element xmpmeta: Schemas validity error : Element '{adobe:ns:meta/}xmpmeta': This element is not expected. Expected is one of ( {https://www.vc5codec.org/xml/metadata}chunk, {https://www.vc5codec.org/xml/metadata}tuple ).
	a5extrinsic.xml fails to validate
	a6dark.xml validates
	a7chunk.xml validates

The XML file `a3samples.xml` fails to validate because of the value element.

The XML file `a5extrinsic.xml` fails to validate because the embedded XML is not defined in the VC-5 metadata schema.


Added the expat XML library to the parser build (both debug and release configurations) and
fixed problems with the make file.

Used Homebrew to install expat library on my iMac Desktop. Homebrew did not link the expat library
into /usr/local so avoid conflicts with the library installed with macOS. Explicitly referenced
the include files and libraries for the Homebrew version in the make files.


### 2019-09-09

Added command-line arguments to the XML parser. Can set the input and output filenames on the
command line and enable verbose output and print out of the help message.

The code is still based mostly on the expat sample code. Need to clean up the code and change the
style to match the rest of the codebase. Need to output the tuples in a text-based format such as
CSV or YAML or other human-readable format.

Need to output the tuples as the payload of a binary chunk as specified in ST 2073-7 Metadata.


### 2019-11-13

Started on Python script to generate random tuples in XML format per Annex A.

For debugging, tuples are written to standard output.

Added code using ElementTree to create and output to standard output the metadata in XML format.

Added code to output the XML to a file specified on the command line.


### 2019-11-24

Changes to the script to work with Python 3.

Added code to write the XML header and nest metadata tuples within a chunk element.


### 2019-11-25

Finished the Python generate script. Added code to partition the randomized list of metadata tuples
into one or more chunks as specified by a command-line argument.

Continue work on the C program for parsing the output of the generate script. Fixed problems
with the make files and changed main program to stop if the command-line argument parser returned
an error code.

Added code to the XML parser to print out the element attributes.


TODO:

1. Write code to output the parsed XML in binary according to VC-5 Part 7.

2. Modify the generator script to nest intrinsic tuples in a class tuple for intrinsic metadata.


### 2019-12-03

Added code to nest metadata tuples within an intrinsic metadata class instance.


TODO:

1. Add code to support other metadata classes

2. Add code to support large metadata chunk elements.


### 2019-12-04

Added code to the XML parser to write metadata tuples to a binary file.


TODO:

1. Need to fill in fields in the tuple header that depend on the data such as the size
of an object instance or padding.

2. Need to handle diverse values as attributes and element text.


### 2020-02-04

Added more tuple types to the CSV file that lists intrinsic metadata tuples.

Modified the Python generate script to merge multiple XML files into a single file so that corner cases
that cannot be generated from CSV files can be created manually and combined with generated files to form
a complete dataset for testing.

Do not need to populate the ICC profile metadata tuple since the reference code is not responsible for verfiying
that the ICC profile data is correct.

Reference to [ICC profile](https://en.wikipedia.org/wiki/ICC_profile)


### 2020-02-06

Added code to the Python generate script to process encoding curve metadata. One CSV file is used for test data
for all types of encoding curves. The idea is that the script will create separate XML files for each encoding curve
and one of the encoding curve files will be merged with intrinsic metadata to form a more complete data set for testing.


TODO: Create complete test data for intrinsic metadata.

TODO: Create test data for series, dark, and extrinsic metadata.


### 2020-02-25

Finished changes to Python generate script to output nested encoding curve metadata.


### 2020-05-04

Refactored the toplevel code into subroutines.

Added code to generate XML layer metadata from layer information in a CSV file.

Added code to output intrinsic metadata tuples that are not nested in JSON format instead of XML.
Thought that the JSON format would be more compact, but see no advantage over the XML format.

Did not add JSON output to the code for generating encoding curve and layer metadata (only XML output).

Added code to convert intrinsic metadata tuples that are not nested from CSV format to JSON format.
Given the complexities in using CSV as an input format for nested metadata such as encoding curves and layers,
it may have been better to use JSON as the input format for all use cases rather than CSV format.

JSON format is not as compact as CSV or XML, but may be easier to read and edit and the same file can contain
both simple intrinsic metadata tuples and nested metadata tuples.

Could read intrinsic metadata (simple tuples or nested), as well as extrinsic and dark metadata, in JSON format,
randomize the intrinsic metadata tuples (simple and nested), randomize the assignment of class instances to chunks,
and output the test data in XML format per ST 2073-7 Annex A.

The output files of intrisic metadata, nested in metadata chunks and class instances per ST 2073-7, in JSON format
were checked into the GitHub repository. But in hindsight, do not see an advantage to JSON as an output format over XML,
hence do not plan to modify the code for generating metadata in XML to also output JSON.


TODO: Consider replacing `testdata.json` and `intrinsic.json` in the GitHub repository with JSON for the input files.
In other word, JSON alternatives to the CSV files used as the input to the generate.py script.


Replaced the files `testdata.json` and `intrinsic.json` in the GitHub repository with input files in JSON format
converted from the CSV input files. Much better representation of the input. Not as compact, but easier to read and edit
and more flexible since the JSON input format can include nested tuples.

Added code to convert encoding curve metadata in the CSV input format to JSON as an input format.

Added code to convert layer metadata in the CSV input format to JSON as an input format.


### 2020-05-05

Finished changes to `generate.py` to allow JSON files to be merged with optional randomization and duplicate tuples.
Modified `generate.py` to create XML files from JSON input files that may have nested metadata tuples.
This allows each testcase to be represented by a single JSON file that is translated into a test case in XML format.

Created a make file for generating test cases in XML format from JSON files. Each test case can include more than one JSON input file
but the intent is that there will be one JSON input file per generated test case in XML format. This will simplify the make file.

Improved the make file to generate one JSON input for for each test case.

Wrote script to automate conversion of input files in CSV format to JSON format.

Wrote script to automate testing of `generate.py` script.

Downloaded DNG papers and code to `~/Downloads`, `~/Downloads/Macintosh`, and `~/Downloads/Windows` on iMac Desktop.

Cloned XMP code and sample files into `~/Projects/XMP` on iMac Desktop.


### 2020-05-06

Used Homebrew to install ExifTool. Note that perl (required by ExifTool) was already installed by Homebrew.


### 2020-05-13

Updated GPMF on iMac Desktop and cloned GPMF on Windows Home:
<https://github.com/gopro/gpmf-parser>


#### GPMF on iMac Desktop

Installed `cmake` on iMac Desktop using Homebrew.

Ran `cmake` command which created a make file.

Used the make file to build `gpmf-parser` successfully.

Tried to build demo/makefile but linking failed: library not found for -lasan


#### GPMF Windows Home

Ran `cmake` command which created a Visual Studio 2017 solution and project.

Rebuilt the Visual Studio solution (Debug Win32): Success.

Executed program `.\Debug\gpmf-parser.exe .\samples\hero6.mp4` and got text dump of the GPMF metadata.
Expected a JSON file but there does not appear to be a command-line option for specifying JSON output.


### 2020-05-15

Modified the stereo layers test case to change the format of the description string to something like a YAML key/value pair. The idea is to make the description parsable by software tools. Although not defined in ST 2073-7 or RP 2073-2, this can be a recommended format for description strings.

Added to layers test cases for HDR and interlaced fields.


### 2020-05-19

Added code to GPMF parser to dump all metadata tuples.


### 2020-05-20

Tested the Python generator script on CSV file output by the modified GPMF parser.

Able to convert the CSV fie into a JSON file that appears to be correct.

The input and output files are in the GPMF subdirectory of `~/Projects/SMPTE/VC-5/smpte-10e-vc5/metadata/python`


### 2020-05-21

Modified the GPMF parser and Python test case generator to fix problems in the JSON representation of streaming data.

Looks good now, appears to be correct.


### 2020-05-25

Reorganized the metadata test cases into subfolders per latest draft of RP 2073-2 Conformance.

The reoganization was applied to the generated test cases in `$(ROOT)/metadata/python/metadata`
and the deployed test cases in `$(ROOT)/media/metadata`.

Modified the test case generation script `generate.sh` to output test cases into the new directory structure.

Started to compile list of test cases in `$(ROOT)/metadata/python/metadata/catalog.txt`


The Python directory is becoming cluttered with files from testing code that generates test cases.
Need to remove unused files or organize the files into subfolders.


### 2020-06-19

Moved the files for the XML parser into a parser subdirectory in preparation for starting on the XML dumper.

Started on the XML dumper based on sample code for parsing command-line arguments using Gnu `argp`.


### 2020-07-18

Finished a Bash script for generating GPMF test cases from all of the MP4 sample files provided in the GPMF repository.

Running `gpmf-parser` on the `karma.mp4` sample file causes a segmentation fault.


### 2020-07-19

Cleaned up the Bash script `generate.sh` for generating GPMF test cases.

Created a make file for generating GPMF test cases so that test cases that have been generated do not have to be recreated unless the input file has changed.

When the `generate.sh` script is run the karma sample file produces a segmentation fault, but the same file is processed correctly by the make file even through both methods run the same executable file.

Changed the location for the streaming data intermediate results `smpte-10e-vc5/metadata/python/GPMF/`

Fixed bug in the make file and removed `karma.mp4` from the list of sample files.

Why is the function `create_nested_tuple_list` in the script `generate.py` not used?
It appears to have been created to handle the nesting of tuples for streaming metadata.

Perhaps the function `create_nested_tuple_list` was written to metadata tuples in CSV format directly into the XML or JSON format for metadata test cases.
If so, this is a different path in the workflow that skips JSON as an intermediate representation for tuples on the path from CSV to XML.

Got the code working for converting streaming metadata in CSV format to JSON format:
```bash
./generate.py -C -g ./GPMF/hero5.csv -o test_hero5.json -v -D
```

Got the code working for converting streaming metadata in JSON format into the XML format defined in ST 2073-7 Annex A:
```bash
./generate.py -g -v test_hero5.json -o test_hero5.xml
```

Installed Sphinx using Homebrew:
```bash
brew install sphinx-doc
```
but the tool did not show up as expected.

The command `which sphinx` listed nothing.

Tried to install Sphinx using a different method:
```bash
pip install -U sphinx
```
but the tool still did not show up using the command `which sphinx` to find it.

Now have two versions of Sphinx installed:
```bash
cd /usr/local/bin

ls -l *sphinx*
lrwxr-xr-x  1 brian  admin   41 May 25 03:30 cairo-sphinx -> ../Cellar/cairo/1.16.0_3/bin/cairo-sphinx
-rwxr-xr-x  1 brian  admin  239 Jul 20 01:56 sphinx-apidoc
-rwxr-xr-x  1 brian  admin  253 Jul 20 01:56 sphinx-autogen
-rwxr-xr-x  1 brian  admin  238 Jul 20 01:56 sphinx-build
-rwxr-xr-x  1 brian  admin  243 Jul 20 01:56 sphinx-quickstart

cd /usr/local/opt/sphinx-doc

ls -l bin
total 0
lrwxr-xr-x  1 brian  staff  28 Jul  5 03:43 sphinx-apidoc -> ../libexec/bin/sphinx-apidoc
lrwxr-xr-x  1 brian  staff  29 Jul  5 03:43 sphinx-autogen -> ../libexec/bin/sphinx-autogen
lrwxr-xr-x  1 brian  staff  27 Jul  5 03:43 sphinx-build -> ../libexec/bin/sphinx-build
lrwxr-xr-x  1 brian  staff  32 Jul  5 03:43 sphinx-quickstart -> ../libexec/bin/sphinx-quickstart
```

The first set of tools seems to have been installed by Homebrew,
the second set seems to have been installed by pip.

Note that `cairo-sphinx` was installed two months ago, probably as a by-product of some unrelated install.

The Sphinx tools installed by pip are two weeks old, probably a by-product of some unrelated install.

Leave the pip version of the Sphinx tools in place: Probably needed by some other package installed using pip.

Use the Homebrew version of Sphinx from the command line by adding the Sphinx bin directory to my path:
```bash
brew info sphinx-doc
sphinx-doc: stable 3.1.2 (bottled) [keg-only]
Tool to create intelligent and beautiful documentation
https://www.sphinx-doc.org/
/usr/local/Cellar/sphinx-doc/3.1.2 (3,879 files, 53.8MB)
  Poured from bottle on 2020-07-20 at 01:49:33
From: https://github.com/Homebrew/homebrew-core/blob/HEAD/Formula/sphinx-doc.rb
==> Dependencies
Required: python@3.8 ✔
==> Caveats
sphinx-doc is keg-only, which means it was not symlinked into /usr/local,
because this formula is mainly used internally by other formulae.
Users are advised to use `pip` to install sphinx-doc.

If you need to have sphinx-doc first in your PATH run:
  echo 'export PATH="/usr/local/opt/sphinx-doc/bin:$PATH"' >> /Users/brian/.bash_profile
```

The Sphinx tutorials use the pip version of Sphinx, so considered leaving the Homebrew version unlinked and not adding it to my path.
That would mean that the pip version in `/usr/local/opt/sphinx-doc/bin` would be added to my path.

However, the command `which sphinx-quickstart` showed that the Homebrew version of the Sphinx tools are already in my path,
so will use that version from the command line regardless of whether the tutorials use the Homebrew version or the pip version.

Looking at the Sphinx documentation, there are two separate sets of instructions for installing Sphinx on macOS:
* Use Homebrew
* Use pip

In my case, it does not seem to matter. The pip version was installed months ago and should be left in place since it may be needed by other Python packages.
No harm in using the Homebrew version from the command line. No conflicts since the pip version is not on my path.

This [tutorial](https://docs.readthedocs.io/en/stable/intro/getting-started-with-sphinx.html)
recommends using the Markdown extension:
```bash
pip install recommonmark
```
but since I am not using the pip version, skipped this step and will just use reStructuredText.

Started to document the Python test case generator:
```bash
cd ~/Projects/SMPTE/VC-5/smpte-10e-vc5/metadata/python
mkdir docs
cd docs
sphinx-quickstart
```
and followed these [instructions](https://docs-python2readthedocs.readthedocs.io/en/master/configure-sphinx.html)
for answering the setup questions.

Setup the Sphinx documentation files in `~/Projects/SMPTE/VC-5/smpte-10e-vc5/metadata/python/docs` but this is starting to get complicated.
Wanted something simple like Doxygen, something that would create simple documentation from the source code.


### 2020-07-20

Installed pdoc:
```bash
pip3 install pdoc3
```
which installed `pdoc` and `pdoc3` commands in `/usr/local/bin` (the two commands are scripts and are identical).

Needed to install lxml for Python3 (was already installed for Python2):
```bash
pip3 install lxml
```

Now can create simple documentation for the generator script:
```bash
mkdir pdoc
cd pdoc
pdoc --html ../generate.py
```

Documentation is [here](file:///Users/brian/Projects/SMPTE/VC-5/smpte-10e-vc5/metadata/python/pdoc/html/generate.html).

The result is even simpler than using Doxygen.
No need for a Doxygen-style configuration file and nothing remotely as complex as the setup for Sphinx.

Do not get a diagram of the function call tree, which would be very useful, but the HTML documentation includes the source code
so can open a function to see what functions it calls and the code itself.

Back to work in streaming metadata.

The collection of make files and bash scripts in `smpte-10e-vc5/metadata/python` is starting to get complicated.
Too many scripts and make files for specialized builds and testing.
Need to refactor the collection of build scripts and make files into something simpler and coherent.

But this can be done later. Create separate smaller make files for each category of test cases.
Prefer make files so that files are not re-created unnecessarily.

Copied generate.mk from `/Users/brian/Projects/GPMF/gpmf-parser` to `/Users/brian/Projects/SMPTE/VC-5/smpte-10e-vc5/metadata/python/streaming.mk`
to edit this file into a make file for generating steaming metadata test cases in XML format.

Fixed problem in the GPMF parser with the output of strings that are not terminated by a nul character.

Able to generate all XML test cases for streaming data extracted from the MP4 sample files provided with the GPMF parser:
```bash
make -f streaming.mk
```

### 2020-07-21

Worked around problems with the GPMF sample files including:
* Character strings in UTF-8 had type 'c' that designates ASCII,
* The length of character strings was in characters, not bytes, so the size was wrong if the strings were UTF-8 (multibyte characters).

Finally got the GPMF parser working well enough to output useable test cases in CSV format.
At least the data is syntactically correct, sufficient for use  in testing conformance.

Some test cases for the fusion and max cameras still cannot be converted from CSV to JSON,
but do not need to include conformance testing data for these cameras.

Modified `generate.py` to create XMP extrinsic metadata in JSON format from the original XMP files.

NOTE: Did not record the command used to extract the XMP data but it must have been something like this:
```bash
exiftool -XMP -b BlueSquare.jpg -b >BlueSquare.xmp
```
Tried this command and it produces identical results as the XMP files in `$(ROOT)/metadata/media/xmp` that were
used to generate XMP metadata test cases in XML format.


### 2020-07-22

Cleaned up the code for intrinsic metadata and returned to work in streaming data. Want to resolve the problems with UTF-8 encoding.

The CSV reader in Python rejected the UTF-8 characters.

After using multibyte character routines in C to study the character strings, concluded that the encoding is actually Latin-1 and
was able to specify that encoding when opening the file:
```python
with open(pathname, newline='', encoding='latin_1') as input:
    reader = csv.reader(input, delimiter=',')
```

See this [article](https://blog.feabhas.com/2019/02/python-3-unicode-and-byte-strings/) on Python 3 unicode and byte strings.

NOTE: The CSV files contain extended ASCII characters.
The size and count fields are correct since extended ASCII characters are always a single byte.

The Python script that creates JSON from the streaming data in CSV format should convert the extended ASCII characters to the
numeric representation for non-ASCII characters as defined in the JSON standard.


### 2020-07-23

Added code to the GPMF parser to ignore tuples with tags that do not consist of printable ASCII characters.
This fixed a problem that was causing the Python CSV reader to fail.
Can now generate CSV files from the GPMF MP4 sample files, convert the CSV files to JSON, and convert the JSON to XML test cases.
Very happy with the results: the m/s2 units are displayed with a superscript when viewing the XML files with `more` in the terminal window.

Still need to handle XMP embedded in XML.

Perhaps the XMP can be converted to a string in UTF-8 and represented in the body of the element as a quoted string.


Could try this method for Python 3:
```python
xml_str = ElementTree.tostring(xml, encoding='unicode')
```

Read the XMP file as an XML element tree and convert to string?

Is the header element preserved? If not, could append it to the front of the string.

The JSON file represents the XMP as a string, properly quoted. Perhaps just need to wrap the XMP string in quotation marks.

Look at how XML is embedded in XMP. See link from teleconference yesterday.


### 2020-07-24

Need to modify `generate.py` script to handle the embedded of XMP with VC-5 Part 7 XML:

* See this [thread](https://stackoverflow.com/questions/33814607/converting-a-python-xml-elementtree-to-a-string?rq=1) on Stack Overflow.

* And read the XMP specification for information about how XML is embedded in XMP.

Created new GPMF parser [documentation](file:///Users/brian/Projects/GPMF/gpmf-parser/docs/doxygen/html/index.html) using Doxygen and CMake.

Translated `nested.sh` into a make file `nested.mk` for creating encoding curve and layers metadata test cases.

TODO: Organize the input and intermediate files in the `$(ROOT)/metadata/python directory` by category of metadata.

TODO: Organize the input files for extrinsic metadata in the `$(ROOT)/metadata` directory by extrinsic metadata class.

TODO: Update the file catalog.txt in the testcases directory or replace with a markdown document.

TODO: Add nested metadata test cases created today with the `nested.mk` make file to the intrinsic testcases in `$(ROOT)/metadata/python/testcases`.

Note that the testcases directory duplicates files that will be checked into `$(ROOT)/media` but that is okay.

What is the difference between the `simple.json` and `intrinsic.json` files:
The files seem to have been created from the same file `intrinsic.csv` which does not contain nested tuples.

The new make file `intrinsic.mk` was written to mimic this assumption which may be wrong.

TODO: The file `intrinsic.deps` is unnecessary and no longer used so it should be removed from Git.

TODO: Need to create intrinsic metadata test cases that combine simple metadata with nested metadata.

It may be the case that `intrinsic.csv` was created as the input file for all intrinsic metadata tuples
before it was discovered that nested tuples needed to be handled as a special case.

Should use `simple.csv` and `simple.json` for simple intrinsic metadata tuples (not nested).
Reserve the `intrinsic.json` filename(s) for intrinsic metadata that includes both simple and nested metadata tuples.
In that case, the test case files `intrinsic-rdc1.xml` and similar files should use the `simple` filename
and the `intrinsic` filename should be used for intrinsic metadata that includes simple and nested tuples.

Need to have at least one test case for all simple and nested tuples for all encoding curves and the three examples
of layer metadata that have already been created.

May need to have a comprehensive layer metadata test case that includes layer metadata for all type of layers defined
in ST 2073-5 Layers.

Document the naming convention for the layer metadata description.

NOTE: This documentation may have already been started in `$(ROOT)/metadata/docs` using markdown.

TODO: Add make file targets to create the documentation for `generate.py` using `pdoc` and possibly also Sphinx.

NOTE: Sphinx might be a good tool for documenting all of the Python scripts written for the Software Task Force.

TODO: Rename `intrinsic.csv` to `simple.csv` and then `simple.json` derived from `simple.csv` would be the source file
for all of the simple metadata test cases such as `simple-c10.xml` for example.

Use `intrinsic` as the prefix for filenames containing test cases that include both simple and nested intrinsic metadata.

Might also want to have specialized test cases in JSON and XML for each of the encoding curves and layers.

NOTE: According to the script `testcases.sh`, the file `simple.json` was used to create the simple-c<n>.xml` test cases
for n from 01 to 25 and the file `intrinsic.json` was used to create the `intrinsic-rdc1.xml` and `intrinsic-rdc2.xml` test cases.

Not sure what was the difference between the `simple.json` and `intrinsic.json` input files.

Perhaps `intrisic.json` was intended to include both simple and nested tuples, but there are several types of encoding curves
and layers, so there would have to be many different JSON input files to cover all the combinations.

NOTE: Checked the old intrinsic metadata files in `intrinsic.old/` and the two files `simple.json` and `intrinsic.json` are identical.

This strengthens the theory that `intrinsic.json` was intended to include more than simple metadata tuples.

Need to create several variations of `intrinsic.json` for all combinations of encoding curve and layer metadata
combined with the simple tuples in `simple.json`.

For example: `intrinsic-GAMA-stereo-rdc4.json` containing simple metadata tuples, the gamma encoding curve, and stereo layer metadata,
randomized with duplicates, and partitioned into four chunks. Need to randomize the simple tuples first and then include the encoding curve
and layer metadata so that the encoding curve and layer metadata is present in the test case. Randomize the list of simple tuples,
encoding curve tuple, and layer metadata tuple before conversion to the XML format used for conformance testing.

It is easy to work with JSON files since that format can represented nested tuples.

The CSV file format is convenient for editing metadata tuples by hand.

Good workflow to go from the CSV input representation to the JSON intermediate representation,
randomize (with and without duplicates) the top-level JSON tuples,
partition the top-level JSON tuples into chunks,
and convert the JSON tuples into a test case in XML format.

NOTE: The test cases in XML format are output to the directory `$ROOT)/metadata/python/testcases` for review
before deployment to `$(ROOT)/media/metadata` as test cases for testing conformance.

This two step procedure allows for review before replacing the test cases for conformance testing and
prevents test cases for conformance testing from being overwritten while debugging the test case generation
software and workflow.

TODO: Edit this information into a document in `$(ROOT)/metadata/docs` or the metadata README file.

TODO: Cleanup and document the directory structure for `$(ROOT)/metadata/python` by placing the data used
for creating extrinsic metadata into subfolders in `$(ROOT)/metadata/python/extrinsic`.

Consider placing the make files and other files required to create each category of metadata test cases
into a subdirectory for that category, for example: `intrinsic/`, `streaming/`, `extrinsic/`, and `dark/`
with, for example, `extrinsic.mk` in the `extrinsic/` subdirectory.

Moved files for creating nested test cases to `$(ROOT)/metadata/python/nested/` and modified the `nested.mk` file.

Can use `$(ROOT)/metadata/python/intrinsic/` for other test cases, but may want to move `nested/` into `intrinsic/` and
create new directories for the simple test cases and the complex test cases that combine simple and nested tuples.

In `$(ROOT)/metadata/python` the following directory structure:
```
intrinsic/
  simple/
  nested/
  complex/
```


# 2020-07-25

Added new make file to generate documentation for the Python scripts using `pdoc`.

Improved how the interconnected make file work to generate test cases and clean the test cases and intermediate files.
Tried to edit the make file so that the functionality is consistent across categories of test cases.

Cleaned up the make files for intrinsic metadata in preparation to create test cases for complex intrinsic metadata.

Generated a test case for simple metadata with one encoding curve metadata tuple, randomized, using the following commands:
```bash
../../generate.py -m -j ../simple/json/simple.json ../nested/json/encoding-GAMA.json -o complex-3.json
../../generate.py -r complex-3.json -o random.json

```
The intermediate file `complex-3.json` was my third attempt.
The output file `random.json` contained all of the simple metadata tuples plus one encoding curve metadata tuple,
in random order, with the encoding curve tuple somewhere in the middle of the file of simple metadata tuples.

Created a complex test case in XML format per ST 2073-7 Annex A using the command:
```bash
../../generate.py -r complex-3.json -o random.xml
```

Was not able to generate XML output from the JSON file of complex metadata:
```bash
../../generate.py random.json -o random.xml
```
The command failed because the code for outputting XML cannot handle metadata tuples as a dictionary (from the JSON file).
Could fix the command but not necessary unless need to retain the randomized metadata tuples in JSON format.

Can save the randomized metadata in JSON format for reference and fix the code later if necessary.

Actually, nothing to fix. The file `random.json` was a complete metadata test case with chunk and class elements,
but in JSON format instead of XML format. Can convert a JSON file of metadata tuples (both simple and nested) using
the command:
```bash
../../generate.py -r complex-3.json -o test.xml
```
or with duplicates using the command:
```bash
../../generate.py -r -d -n 20 complex-3.json -o test.xml
```
but there is no guarantee that the nested metadata tuples will be included in the output file.
Need to randomize the simple metadata tuples with duplicates before merging in nested metadata tuples.

TODO: Create JSON files for all combinations of simple and nested metadata like the `complex-3.json` file above.

Finished Python and Bash scripts and make file for generating complex intrinsic metadata test cases from all combinations of
simple and nested metadata test cases.

TODO: Need to change the output format from JSON to XML after debugging the code

Can both the JSON and XML representations be created?

Can create the output files in JSON format and add a make rule for converting JSON to XML

TODO: Add extra steps to create randomized simple metadata test cases, with and without duplicates,
before merging in the nested test cases for encoding curve and layers.


Procedure for creating complex test cases:

1. Randomize the simple intrisic metadata input files with and without duplicates,

2. Merge in the nested intrinsic metadata encoding curves and layers,

3. Randomize the JSON intermediate files for the complex metadata test cases and convert to XML.

NOTE: Step 1 may create multiple files of simple intrinsic metadata that have to be added to the list
of input files for the simple metadata category in the `complex-inputs.json` input file.


TODO: Run the make file `complex-inputs.mk` from the make file `intrinsic.mk` in the parent directory
before running the make file `complex.mk` to create the complex intrinsic metadat test cases.


TODO: Need to modify `complex.mk` to generate JSON files and then convert the JSON files to XML
as outlined in the steps above. Need to fix the naming conventions for `INPUT_DIR`, `JSON_DIR`, and `OUTPUT_DIR`.

Perhaps 'COMPLEX_JSON_DIR` and `XML_OUTPUT_DIR`.

TODO: Modify the `combinate.py` script and regenerate the rules and targets.

Split `complex-inputs.rules` and `complex-inputs.targets` into seperate files for JSON and XML.
For example, `complex-json.rules` and `complex-xml.targets`.

Question: Need complex test cases that are randomlized or are the simple test cases sufficient?

TODO: Added random number seed but need to test whether it produces the same randomized file for every run.


### 2020-07-26

Improved the make files for simple and complex intrinsic metadata test cases.

Split the rules and targets for complex intrinsic metadata into separate files for the JSON and XML formats.

Currently, the make files only generate test cases in JSON format.

TODO: Add steps to generate test cases in XML format.

Not much to do. Can generate complex test cases in XML format from the JSON that was created by `make -f complex.mk`
using the command:
```bash
../../generate.py json/simple-FSLG-stereo.json -o xml/simple-FSLG-stereo.xml
```
or randomized using the command:
```bash
./../generate.py -r json/simple-FSLG-stereo.json -o xml/simple-FSLG-stereo.xml
```

Modified the `combinate.py` script and make files to generate complex intrinsic metadata test cases in XML format
from the intermediate test cases in JSON format.

TODO: Enable randomized XML test cases if that is desirable.

Cleaned up the files and scripts in the directories for generating simple, nested, complex, and streaming metadata test cases.

TODO: Modify make files to optionally output test cases in XML format to `$(ROOT)/metadata/python/testcases/` and
check the metadata test cases into the repository. And modify the top-level make file to deploy testcases to `$(ROOT)/media/metadata`
after the test cases have been reviewed for accuracy and completeness.

Modified the make files for intrinsic and streaming metadata to enable setting the output directory from the command line.
By default, the output files are placed in the `.xml` subdirectory to avoid overwriting good files, but can create test cases
using a command like:
```bash
make -f complex.mk OUTPUT=../../testcases/intrinsic/complex
```

All intrinsic and steaming metadata in `$(ROOT)/metadata/python/testcases/` have been checked into the repository.

TODO: Deploy the metadata to `$(ROOT)/media/metadata` after testing and review.

NOTE: Need to find the `putmedia.py` script to upload the metadata to the VC-5 bucket in AWS S3 storage.

TODO: Update VC-5 Part 2 to reflect changes to directory structure and workflows for creating metadata test cases.


### 2020-07-27

Found these articles on how to embed XML inside XML:

[Escape XML with LXML](https://stackoverflow.com/questions/14616387/how-do-i-escape-special-character-to-write-xml-using-lxml)

[EscapingXml](https://wiki.python.org/moin/EscapingXml)

[LXML Special Characters](https://groups.google.com/forum/#!topic/python-pptx/RgRUYKxaNpg)


Followed the advice in these articles and seemed to be able to read an XML file, create an element tree, and write the tree to a file:
```python
from lxml import etree

with open("BlueSquare.xmp") as input:
  s = input.read()

root = etree.fromstring(s, parser=etree.XMLParser(recover=True))

t = etree.tostring(root, pretty_print=True)

with open("output.xml", 'w') as output:
  output.write(t.decode('utf-8') + "\n")
```
so should be able to read the XML file into an element tree and add that tree as text to its parent element.

Tried the following sequence of commands:
```bash
../generate.py -E XMPD -j ../../media/xmp/BlueSquare.xmp -o BlueSquare.json
../extrinsic.py BlueSquare.json >BlueSquare.xml
```
The XML file is identical to the original XMP file.

Now need to do something similar by either:

1. Reading the XMP file directly, or

2. Using the value in the JSON file as a string and creating an element tree as shown above.


Modified `generate.py` to convert the JSON representation of XMP metadata to the XML format defined by ST 2073-7 Annex A.

Modified `generate.py` to generate XMP metadata in the XML format defined by ST 2073-7 Annex A directly from the XMP input file.


Modified the command-line argument used by `generate.py` for extrinsic metadata to use common acronyms that are mapped by the
script to the metadata class tag. This change prepares the way to add other extrinsic metadata classes to `extrinsic.mk` using
the metadata acronym to customize the variables for the output directory names and command-line arguments.

Revised the draft of RP 2073-2 to describe the three categories of intrinsic metadata (simple, nested, and complex) and update
the tables with the latest filenames for the test cases in XML format up to and including the section on XMP extrinsic metadata.


### 2020-07-28

Started work on DPX metadata. Created Sublime Text project for the `dpxdump` program and added command-line arguments using `argp`.

Modified the DPX file dumper to compute the output filename from the input pathname. Ran the DPX dumper on the one DPX sample file
available from the HDR DPX implementation project.

TODO: Decode the base64 header to binary and compare with the header in the DPX file.


### 2020-07-29

Write Python script `dpxdump.py` to compare the Base64 output from the DPX dumper (in C) to the original DPX file.
The script also provides an alterative to using the DPX dumper.

Still had to struggle with bytes versus strings, which is -- as I recall -- why I did the DPX dunmper in C,
but making progress in understanding bytes and strings in Python 3.

Modified the make file for the DPX dumper to place intermediate build files in a subdirectory and
added a command-line argument to specify the output directory. While performing this work, improved the
use of the GNU Argp library and learned more about how to use it correctly. A bit of a learning curve
but it is a better way to add command-line options to C programs.

Modified the Python script `generate.py` to include optional extrinsic metadata tuples for file information.

Brief information about [File URI](https://en.wikipedia.org/wiki/File_URI_scheme) that was very helpful in getting
the syntax for the `PATH` metadat tuple correct.

Modified `generate.py` to add a seed to the random number generator (default is to not use a specific seed)
and added a command-line argument for providing the seed. This change allows the metadata test cases to be
randomized but still reproduce the same file every time to avoid triggering unnecessary checkins to the
software repository.

Modified `generate.py` to include optional file information in the XML output of extrinsic metadata.


### 2020-07-30

Satisfied with the hierarchy of directories for metadata test cases and the make file naming conventions
(using `*.mk` for the make file in each category of metadata test cases). Like how `Makefile` is used in the
`python` directory and other directories above the hierarchy of metadata test case directories.

Might consider changing `*.mk` for test cases to `Makefile`, but not sure that it would be better to following
that naming convention -- used elsewhere in my source code -- for the special case of making metadata test cases.

The `Makefile` in the `python` directory is a good transition point from one naming convention to another and
preserves the `Makefile` name for uses cases that involve building and documenting software.


### 2020-07-31

Modified the DPX dumper to suppress output if the verbose flag is not set.

Modified `generate.py` to create metadata test cases in JSON format from the DPX header extracted by
the DPX Dumper into a file in Base64 format.

Modified `generate.py` to record commands and the working directory to a log file for use in running
the coverage tool to create an annotated source file showing code coverage.

Added `testing.py` to parse the entries in a log file to test the use of the log file before adding
code to `generate.py` to run the log.


#### 2020-08-01

Installed `pytest` on iMac Desktop.

Modified `generate.py` to format the log file entries as Bash commands so that the log can be re-executed.

Experiments with re-executing the generator script:
```bash
./generate.py -C intrinsic/simple/simple.csv -L simple.log -o simple.json
rm simple.json
sed 's/ -L simple.log//' simple.log | bash
```
runs the command in the log file and re-creates the `simple.json` file.

The `sed` command removes the log file command-line option from the generate command in the log file
to suppress adding more entries to the log file when the commands in the log file are executed.

Modified `generate.py` to take the pathname for the log file from the environment variable `GENERATE_LOG`
if the log file is not provided in the command line.

Running the generate script create a log file that can be executed by Bash but need to unset the environment variable
to suppress adding more entries to the log file:
```bash
export GENERATE_LOG=simple.log
./generate.py -C intrinsic/simple/simple.csv -o simple.json
unset GENERATE_LOG
sed 's/ -L simple.log//' simple.log | bash
```

The commands in the log file were successfully executed by Bash.

Wrote script `verify.py` to compare the metadata test cases in two directory trees.
For example:
```bash
./verify.py intrinsic/ testcases/intrinsic/ -e xml,json,b64
```
with the `xml`, `json`, and `b64` subdirectories omitted from the directory path in the second tree.

Refactored the code for generating XMP metadata test cases in XML format.

Added code to generate DPX metadata test cases in XML format.

Created make file for generating DPX extrinsic metadata test cases.


### 2020-08-02

Modified the make file in the top-level `python` directory to set the environment variable for creating a
log file of commands that can be used to measure code coverage.

Modified `generate.py` and the make file to run the `coverage` tool to measure coverage of the Python script
for generating test cases.

Finished the `verify.py` script used to compare the working copy of the metadata test cases with the files
in the `testcases` directory. The script can also be used to compare the working copy or the files in the
`testcases` directory with the published versions in `$(ROOT)/media/metadata`.

Added seed to `simple.mk` and modified the make rules in `simple.rules` to use the seed so that the results of
creating metadata test cases with randomized tuples are the same for every run. This allows the `verify.py` script
to be used to check that the working copy of the metadata test case files are the same as stored in the `testcases`
directory and checked into GitHub.


### 2020-08-04

Added targets to the make file in the `python` directory to create files of software metrics.

Added code to output dark metadata in JSON format from an input file in Base64 and output dark metadata
in XML format from an input file in JSON.


### 2020-08-05

Modified the scripts and make files to support dark metadata with two different types of identification codes:
* FOURCC
* GUID


### 2020-08-06

Fixed problem in the output of XML for XMP metadata. The payload was encoded using safe characters. Somehow,
the wrong function was being called -- perhaps an accidental change while making other edits. Corrected the
`generate.py` script to call the correct function. Now fine.

Noticed that the pathname in the file info contained my home directory.

Modified the code to use the location of the metadata file on GitHub, but this does not correspond to the specification
in ST 2073-7 which states that the pathname should be the original file from which the metadata was extracted.
For example, the XMP file from which the metadata was obtained. Ditto for the file creation and modification times.

TODO: Modify `generate.py` to insert the correct file information into the metadata output file.

Not sure how to do this as the input to the script is often an intermediate file (for example, in JSON format).

Started work on `fileinfo.py` to create file information in JSON from files listed on the command line.


### 2020-08-07

Fixed bug in `generate.py` for creating DPX metadata test cases.

Updated the XMP and DPX metadata test cases in `$(ROOT)/metadata/python/testcases/` to replace older files
with new XML files that include file information.

TODO: How to fix dark metadata generation to use a repeatable GUID?

TODO: Use `fileinfo.py` to create file information for the original files from which extrinsic metadata
was obtained.

For XMP, the original files are the JPEG files included in the Adobe XMP SDK saved in the directory:
`$(HOME)/Projects/XMP/XMP-Toolkit-SDK/samples/testfiles`

For DPX, the original files are the DPX files in the directory:
`$(HOME)/Projects/SMPTE/VC-5/smpte-10e-vc5/metadata/media/dpx`

Created make file to run the `fileinfo.py` script to collect information about the original files
used to generate extrinsic metadata test cases.

Modified `fileinfo.py` to merge the new file information into the existing file information if the output file exists.

Downloaded and unzipped MXFLib to iMac Desktop in the `~/Software` folder.

Tried to build MXFLib in `/Users/brian/Software/mxflib-1.0.1` but got an error about member reference and a warning
about a method shadowing a virtual method:
```bash
Making all in mxflib
if g++ -DHAVE_CONFIG_H  -I..   -D_FILE_OFFSET_BITS=64 -D_LARGEFILE_SOURCE -DDEFAULT_DICT_PATH=\"/usr/local/share/mxflib\" -Wall -g -O2 -MT crypto.o -MD -MP -MF ".deps/crypto.Tpo" \
	  -c -o crypto.o `test -f 'crypto.cpp' || echo './'`crypto.cpp; \
	then mv -f ".deps/crypto.Tpo" ".deps/crypto.Po"; \
	else rm -f ".deps/crypto.Tpo"; exit 1; \
	fi
In file included from crypto.cpp:31:
In file included from ../mxflib/mxflib.h:85:
../mxflib/smartptr.h:506:48: error: member reference base type 'SmartPtr<T> *' is not a structure or union
                bool operator<(SmartPtr &Other) { return this.operator<(*Other->GetPtr()); }
                                                         ~~~~^~~~~~~~~~
In file included from crypto.cpp:31:
In file included from ../mxflib/mxflib.h:105:
../mxflib/metadata.h:367:8: warning: 'mxflib::DMSegment::MakeLink' hides overloaded virtual functions [-Woverloaded-virtual]
                bool MakeLink(MDObjectPtr DMFramework);
                     ^
../mxflib/metadata.h:165:16: note: hidden overloaded virtual function 'mxflib::Component::MakeLink' declared here: different number of parameters (2 vs 1)
                virtual bool MakeLink(TrackPtr SourceTrack, Int64 StartPosition = 0) { return false; }
                             ^
../mxflib/metadata.h:169:16: note: hidden overloaded virtual function 'mxflib::Component::MakeLink' declared here: different number of parameters (3 vs 1)
                virtual bool MakeLink(UMIDPtr LinkUMID, UInt32 LinkTrackID, Int64 StartPosition = 0) { return false; }
                             ^
1 warning and 1 error generated.
make[2]: *** [crypto.o] Error 1
make[1]: *** [all-recursive] Error 1
make: *** [all] Error 2
```

TODO: May have to use Windows or Linux to build MXFLib.


### 2020-08-08

Modified `generate.py` to use the file information from a JSON file for the optional metadata in extrinsic test cases.

Modified `generate.py` and `dark.rules` to use the same UUID every time for repeatability of the test cases.
The UUID was copied from `fileinfo.json`, which is otherwise not used, and inserted into `dark.rules` for the target
that creates the dark metadata GUID test case.

Downloaded MXFLib for Windows: Precompiled tools (exe files) and supporting files.

Ran `mxfdump` on one of the sample files pulled from the net:
```PowerShell
cd C:\Users\brian\Projects\MXF
mxfdump.exe -v -l -m ..\..\Software\MXFLib\mxflib-1.0.1-win32\dict.xml .\samples\sample_1280x720.mxf > sample_1280x720.txt
```

Can see the textural representation of the RGBA descriptor but not sure where it ends. The elements are dumped in linear order, flattened.

```PowerShell
Element : RGBAEssenceDescriptor
  Attribute : base = "GenericPictureEssenceDescriptor"
  Attribute : detail = "Defines the RGBA Picture Essence Descriptor set"
  Attribute : globalKey = "06 0e 2b 34 02 53 01 01  0d 01 01 01 01 01 29 00"
```

The dump also lists a CDCIEssenceDescriptor and a GenericPictureEssenceDescriptor.


[MXFLib Documentation](file:///Users/brian/Software/mxflib-1.0.1-docs/index.html)

Modified the make file and rules for dark metadata to obtain the GUID from the JSON file `fileinfo.json`
containing information about the source of the dark metadata.

Downloaded [KLVLib](https://sourceforge.net/projects/klvlib/) but needed to make some changes to get it to build.
Created a Git repository to track the changes.

Was able to get the `klvdump` program to build and ran it on one of the MXF sample files downloaded from net:
```bash
cd /Users/brian/Projects/SMPTE/VC-5/smpte-10e-vc5/metadata/media/mxf
klvdump sample_1280x720.mxf
```

Need to figure out how to interpret the output.

Create a branch for modifying `klvdump` to output the RGBA and CDCI descriptors in Base64 or binary.


### 2020-08-09

Added code to update the metadata test cases in the `$(ROOT)/metadata/python/testcases` directory with
the working copies of the metadata test cases in subdirectories of the `$(ROOT)/metadata/python` directory.

Updated the metadata test cases and pushed the updated to the repository.

Added target to the Makefile to deploy the test cases in `$(ROOT)/metadata/python/testcases`
to the top-level media directory for metadata `$(ROOT)/media/metadata` but the target is commented out.
Need to test the target actions before enabling the target in the make file.

Created new Python script `listfiles.py` to create a CSV file of metadata test cases.

Mostly working, but does not seem to handle directory paths that are only one level such as dark metadata.


### 2020-08-10

Used the `metrics.py` script to tabulate software metrics for all Python scripts.

Added to the `README.md` for the Python scripts and test data.


### 2020-08-13

Installed [`xmldiff`](https://pypi.org/project/xmldiff/) to compare XMP files with the corresponding XML test cases:
```bash
pip install xmldiff
```

The tool works great but flags the extra XML elements in the test cases.

TODO: Write Python script to extract the XMP payload from the XML test cases.


### 2020-08-15

Tried to fix problem creating documentation for `testing.py` using `pdoc` but do not understand the error.
Changed the make file to eliminate that file. Able to create documentation for the other Python scripts,
but get a warning about `\s` in `fileinfo.py`.

Checked in `tracking.csv` that lists all files that are tracked and not tracked (others).

The file was used to create a spreadsheet in Numbers that can be filtered and sorted to see what files
should be added or removed from GitHub.


### 2020-08-19

Wrote `extract.py` script to extract the XMP payload from XMP test cases in XML format to aid in using `xmldiff`
to verify that the metadata matches the XMP input.


### 2020-08-21

Added more DPX files from the HDR DPX reference code project. Updated the file information database.

Cleaned up the declarations of ignored files.


### 2020-09-01

Removed files that were checked into GitHub can are not longer needed.

Added files that should be checked into GitHub.

Strategy: Do not check in files from the working directories `metadata/python/{intrinsic, extrinsic, streaming, dark}`
that are easy to recreate.

Checked in the `CSV` files (including in the files in `python/GPMF`). Some of the CSV files were created by hand
and should be checked in. The files in `python/GPMF` depend on the sample files distributed with the GPMF parser and
were checked in to make sure that the files are always available.

Files in CSV and JSON format that can be easily recreated from the original sources (such as XMP and DPX) are
not checked into GitHub.


### 2020-09-04

Wrote a new tool to convert binary files to base64 format.

Need this tool to convert the binary files containing essence descriptors output by `mxfdump` to base64
for insertion into the metadata test cases workflow.


### 2020-09-05

Created make files and modified scripts and tools to produce metadata test cases for MXF files.

Created a new make file to generate text dumps and SVG diagrams from MXF files.


### 2020-09-06

Researched how to use ExifTool to extract ACES metadata.

<https://www.arri.com/en/learn-help/learn-help-camera-system/camera-workflow/image-science/aces/deliverables>

The command:
```bash
exiftool DigitalLAD.2048x1556.exr
```
outputs a text dump of the metadata including the `Aces Image Container Flag`.

How to extract the metadata to a file?

GitHub repository for [ACES sample files](https://github.com/ampas/aces-dev/tree/master/images)

Sample images on [Dropbox](https://www.dropbox.com/sh/9xcfbespknayuft/AAD5SqM-W9RyYiAo8YFsUrqha?dl=0).

ACES Container [standard]()

Tried to install Python OpenEXR package but got build errors:
```bash
pip3 install OpenEXR
```

Installed OpenEXR using Homebrew but this did not install the Python package and still get build errors
when trying to install OpenEXR using `pip3`.

Get build errors when trying to install Python OpenEXR package on both macOS and Windows.

Tried using `sudo` to install OpenEXR:
```bash
sudo pip3 install OpenEXR
```
but still get build errors.


### 2020-09-07

Modified `generate.py` to use the correct tags for the ACES metadata.

Fixed bug in code for dumping ACES attributes and added an option to print the ACES attributes
instead of dumping the data to a file.

Output MXF and ACES metadata test cases to corresponding directories in `python/testcases/extrinsic`
and checked the test cases into GitHub.


### 2020-09-11

Created `klvdump` tool and modified `bintool` to create DMCVT metadata test cases.

Extracted DMCVT KLV tuple and payload in binhex (plain text) from `imf_as02_DMCV.mxf`
using the BinHex dump tool in BBEdit.

Used `bintool` to convert to a binary file.

Used `klvdump` to check that the binary files were correct.

From that point, the workflow was simliar to other extrinsic metadata classes:
bin to b64 to json to xml.

The code works, but the DMCVT metadata representation in XML places the Universal Label
and the payload in separate elements.


TODO: Modify `klvdump` or write a new tool that splits the binary file into JSON with
separate elements for the UL and payload. Perhaps this could be done in the `generate.py`
script but would need to implement BER decoding in Python.


Installed Python `asn1` package using `pip3`.

Write new Python script `dmcvt.py` that reads the binary file and outputs JSON with separate
fields for the Universal Label (UL) and payload as specified by ST 2073-7 Annex J.


### 2020-09-12

#### DMCVT

Cleaned up the code for `klvdump` to use `size_t` for long-form BER encoded tuple length and
make the output more compact.

Can the code for creating DMCVT metadata in JSON format be added to the `generate.py` script?
Not easily since it is a special case and very different from the existing code.

Continue to use a separate script for converting DMCVT binary KLV data to metadata in JSON format.

Modified `dmcvt.py` script and the make file for DMCVT extrinsic metadata.

Can produce XML metadata for DMCVT.


#### ALE

Used Premiere Pro to import two video clips from GoPro Session (bicycling).

Moved each clip to the source window and used the mark and extract tools to add segments to the sequence.

Exported to ALE but only got one line for each clip. Was expecting something more like an EDL.

Changed each segment to a subclip. Cleared the sequence and re-added the subslips to the timeline.

Exported ALE and got one line per subclip, creating a decent ALE metadata test case.

Created make file for ALE and added file information to the database.

Small change to `generate.py` to fix bug in placement of the file information elements relative
to the payload.

Able to create good ALE test case in XML format.

TODO: Need to rerun other test cases that include file information to correct bug in the
hierarchy of dictionaries and lists in the JSON files, then re-create the XML files.


### 2020-10-12

Worked on XML parser and dumper, adding code to handle nested tuples.

TODO: Test the parser and dumper on streaming and intrinsic metadata and dark metadata.


### 2020-10-13

Need to change the XML format for XMP metadata to use CDATA because the expat parser is trying to parse
the embedded XML for the XMP metadata.

Modified XML parser to handle vectors of values.

Need more testing but it appears that the XML parser can write binary files for streaming data.

TODO: Check the results using the XML dumper.


### 2021-01-10

Wrote new Python script `b64string.py` to encode the string representation of binary data as base64.
Updated the XML files for intrinsic metadata to use the new base64 representation.


### 2021-01-11

Modified `generate.py` to use CDATA element for the payload in XMP metadata.

Modified the XML parser to decode base64 representations of binary data into the binary output file.

TODO: Need to add more error handling to the new code in the XML parser.


### 2021-01-12

Looked at the CSV files extracted from the GPMF MP4 files. Some anomalies. Perhaps not serious, but should fix.

Write a Python script to check CSV files for bad syntax or tuple header values that do not make sense.

Use the `verify.py` script that was intended to be written for checking XML files per ST 2073-7 Annex A.
Can use the same program to check CSV and JSON representations of metadata.

Values to check:
1. Is the tuple size correct for the specified data type correct?
2. Is the number of values equal to the product of the size and count?
3. Character strings should have size equal to the string length (including terminating nul byte) and count of 1.


### 2021-01-13

Renamed existing `verify.py` script for comparing test cases in two directory trees to `compare.py`
so that the filename `verify.py` could be used for a new script to verify the correctness of metadata files.

Modified `verify.py` and `generate.py` to open CSV files of streaming GPMF metadata with the encoding
specified explicitly to handle special characters in the file. This change is required for Python3.
Explicit specification of the encoding is not required for Python2.

See Stack Overflow discussion:
<https://stackoverflow.com/questions/19699367/for-line-in-results-in-unicodedecodeerror-utf-8-codec-cant-decode-byte>

TODO: Test the `generate.py` works correctly in converting GPMF metadata in CSV format to JSON and XML.

Use of size and count is inconsistent in the CSV files for GPMF metadata. Sometimes count is 1 and size equals the string length;
other times, size is 1 and count is the string length + 1.

Refactored `verify.py` to improve verification of more metadata categories.

The `sort` command can be used to eliminate duplicate tuples from the output of the `verify.py` script:
```bash
./verify.py GPMF/hero5.csv | sort -u
```


### 2021-01-23

Removed old CSV file of GPMF metadata that is wrong (probably) and no longer needed:
```bash
git rm hero5-bad.csv
```

GPMF metadata contains Unicode strings that should not have the metadata type 'c' since
that data type is reserved for US-ASCCI strings.

TODO: Need to use 'u' for metadata strings that are actually UTF-8 encoded.

And there seems to be a problem reading UTF-8 strings from the the MP4 file.


### 2021-01-24

Modified GPMF parser to change the tuple type 'c' for US-ASCII to 'u' for UTF-8 because many character strings
in the GPMF data extracted from the MP4 sample files are UTF-8 even though the date type is labeled as US-ASCII.
This may be due to changes in SMPTE ST 2073-7 that differ from the original GPMF specification.

TODO: Noticed two anomalies in the CSV representation of the GPMF metadata:
```bash
Tag: SIUN, type: u, size: 10, count: 1, length: 10, value: "m/s²SCALs"
Tag: UNIT, type: u, size: 7, count: 5, length: 7, value: "degdegm"
```

The `SIUN` tuple appears to contain portion of the next tuple (`SCAL`) but note that the number of bytes
in the UTF-8 string appear to be correct since the superscript requires two bytes in a UTF-8 encoding.

Modified GPMF parser to use the size in the header to copy tuple value into a buffer and add a terminating nul
to the characer strings. The `SIUN` tuples now appear to have the correct size, count, and value.

Not handling arrays of character strings such as the `UNIT` tuple listed above.

Changed the GPMF parser to not change the tuple size and count in the `UNIT` tuples.

The count appears to be larger than the number of units in the array of character strings.
Not sure what to do about this. If the XML parser can handle this discrepancy, then okay.

Changed the GPMF parser to output the value of the TSMP tuple as an unsigned 32-bit integer (was float).


### 2021-01-25

Created a comprehensive makefile infrastructure for the XML parser. If an XML test case or any of the programs
or scripts in the XML parser that process the test case change, then the binary file is re-created.

Fixed several problems in the extraction of GPMF metadata from MP4 files. Some remaining problems with the MP4 sample files
for the GoPro MAX camera so omitted the two test cases for that camera model. Not necessary to include metadata from all
of the MP4 sample files provided with the GPMF software.

Changed the data type of UTC date and time metadata to 'c' since 'U' is now used for a SMPTE Universal Label (UL).

TODO: Fix problems with the conversion of 'UNIT' metadata from XML to binary:
```bash
./build/linux/debug/parser -o binary/hero5.bin ../python/streaming/xml//hero5.xml
TupleHeaderSize tag: UNIT, size: 3, actual: 15
TupleHeaderSize tag: UNIT, size: 3, actual: 15
TupleHeaderSize tag: UNIT, size: 3, actual: 15
TupleHeaderSize tag: UNIT, size: 3, actual: 15
TupleHeaderSize tag: UNIT, size: 3, actual: 15
TupleHeaderSize tag: UNIT, size: 3, actual: 15
TupleHeaderSize tag: UNIT, size: 3, actual: 15
TupleHeaderSize tag: UNIT, size: 3, actual: 15
TupleHeaderSize tag: UNIT, size: 3, actual: 15
TupleHeaderSize tag: UNIT, size: 3, actual: 15
TupleHeaderSize tag: UNIT, size: 3, actual: 15
TupleHeaderSize tag: UNIT, size: 3, actual: 15
TupleHeaderSize tag: UNIT, size: 3, actual: 15
TupleHeaderSize tag: UNIT, size: 3, actual: 15
```

TODO: Add more testcases to the XML parser

TODO: Try the XML dumper on some of the binary files created by the XML parser


### 2021-01-26

Started text file `changes.txt` in the GPMF parser top-level directory documenting changes made
on the `vc5` branch for the VC-5 codec.

Minor changes to the `testcases.py` script for the XML parser to add comment line stating
that the `testcases.rules` file is automatically generated and should not be edited.

The `expat` library can parse but not create XML files.

Replaced `expat` with `mxml` in the XML dumper.

Able to dump the metadata and chunk elements from the test file `./parser/binary/simple-c01.bin`
and can dumpe the tuples but the count is zero.

Check the count in the binary files written by the XML parser.

TODO: Need to add the metadat class elements

TODO: Need to handle nested tuples

Create a stack of XML nodes with the current node at the top of the stack.

Compared `../python/intrinsic/simple/xml/simple-c01.xml` with `xml/simple-c01.xml ` and the files are functionally the same
except for the tuple `<tuple tag="CFAP" type="B" size="1" count="0" value="66" />` that should be
`<tuple tag="CFAP" type="B" size="1" count="0" value="RGGB"/>`.

And the count in the XML test case input to the XML parser is zero and should not be in some cases.


### 2021-01-27

Added tasks to GitHub issue tracker to verify conformance of the metadata test cases with ST 2073-7 Metadata.

Note that ST 2073-7 Annex A is informative so departure from the XML specification in ST 2073-7 Annex A is allowed.
For example, can use `CDATA` elements for XMP metadata.

Why are numerical attributes quoted? Because the XML specification requires that all attributes be quoted.
The syntax of JSON allows numbers to be represented without quotes but since JSON is only an intermediate
representation, makes sense to quote all attribute values.

Specification of JSON syntax: <https://www.json.org/json-en.html>

Specification of XML syntax: <https://www.w3schools.com/xml/xml_syntax.asp>

Changed the data type of CFAP to US-ASCII (was bytes) and set the length to 4 characters in
the source file `simple.csv`. Re-generated the XML test cases:
```bash
make -f intrinsic.mk
```
All test cases re-generated successfully.

Modified the XML parser to not write CFAP metadata as a binary block.
Re-ran the XML parser and XML dumper on `simple-c01.xml` and the output of the XML dumper is
identical to the XML test case except for differences in indentation and use of double versus
single quotes in the XML header element.

Modified the XML parser output binary files into a directory tree that mirror the input files.
Ditto for the XML dumper.

Lots of changes to fix minor problems.

Cannot dump streaming data.

Appears that the parser is writing the size and count incorrectly, perhaps swapped.

But the simple test case appears to be correct and the file `hero5.xml` input to the XML parser appears to be correct.


TODO: Look at the hex dump of the binary file `hero5.bin` created by the XML parser carefully.

Found the problem: The size and count depend on the type of the metadata tuple and this was not handled
correctly in either the XML parser or dumper. Fixed the XML dumper but have to apply the same change to the parser.


### 2021-01-28

Started to move common source and header files into the `.../metadata/common/` directory tree.

Moved the base64 routines to the common directory and updated the make files.
Rebuilt the XML dumper and parser successfully.

Moved the metadata source file to the common files and merged the metadata header file for both
the XML parser and dumper into a single header file in the common files.

Able to build the XML parser and dumper successfully.

Fixed problem with dumping vector-valued payloads.

Still have trouble with the `UNIT` tuple in streaming metadata.

Removed processing of UNIT metadata tuples from the GPMF parser.

Reworked the logic for changing the nesting level in the output of the XML dumper.

Reworded the logic for changing the nesting level again to use current and next levels.
This allows the nesting level for the current metadata tuple to be changed and also set
the nesting level for metadata tuples after the current tuple.

Seems to be working correctly for intrinsic simple metadata and streaming metadata.

TODO: Fix segmentation fault in processing the streaming metadata `GYRO` tuple.


TODO: Improve algorithms for indentation

1. Use a table-drive algorithm for intrinsic metadata.

2. Add case 0 to current nested tuple (means that not inside a metadata class instance).

3. Also use table-driven approach for extrinsic and dark metadata.

4. Use a stack for the hierachy of nested tuples (top element is the current nested tuple).


### 2021-01-29

Improved the algorithm for computing the indentation level of intrinsic metadata tuples.
Using a table indexed by the tag of the current nesting tuple and the tag of the new tuple found in the bitstream.
Each table entry specifies the relative change in the current and next nesting levels.
No changes to the current or next nesting level if no entries found in the table.

Worked most of the day on the nesting level for printing the tuples and building the XML tree,
but it is still not working. The nesting for chunk elements and metadata class tuples seems correct,
but nested tuples inside the metadata class element are not nested as expected.


### 2021-01-30

Reworked the logic for updating the XML node stack.

The XML dumper creates a correctly nested XML document tree for the `simple-LOGA-stereo.bin` test case.

Found long-standing bug in the calculation of the metadata tuple padding.
Fixed the bug and now the XML dumper can handle the streaming metadata test case `hero5.bin`
without a segmentation fault and converts the entire binary file in XML format.

Noticed a problem with the nesting of XML nodes for the `STRM` metadata tuple.

Fixed the XML node nesting problem with some small changes to the table that controls node stack operations.

Changed XML output to not include attributes in nested tuples and added more data types to the XML output.

TODO: Run all intrinsic metadata test cases through the XML parser and dumper.

TODO: Run all streaming metadata test cases through the XML parser and dumper. [DONE]

TODO: Add test cases for extrinsic and dark metadata to the XML parser and dumper.


### 2021-01-31

Fixed problems in the code for byte-swapping floating-point numbers.

Comparing the hero5.xml input to the XML parser versus the output from the XML dumper,
see many differences:

1. Count set to one in the input versus zero in the dump. [FIXED]

2. Scaling floating-point numbers appear to be okay. [OKAY]

3. Vectors are not dumped.

4. Problems in the dump of UTF-8 strings that are not US-ASCII.

5. Problems with the output of signed short values for the SCAL tuple.


Modified the code for managing the XML node stack to fix problems with creating a new DEVC tuple
while processing a STRM tuple.

Struggled with UTF-8 encoding.


### 2021-02-01

Added a hack to the XML parser to handle the UTF-8 code point C2 B2 found in a SIUN tuple as a special case.
Now the XML parser creates a correct binary file that is converted to correct XML file by the XML dumper,
except that the size (in bytes) differs from the original string length (in characters).

The GPMF dumper is not properly handling all GPMF sample files.
See `generate.log` for more information.

Fixed the problem or perhaps just needed to rebuild the GPMF code.

Can now process all GPMF test cases through the GPMF parser, XML parser and XML dumper.

TODO: Check that all test cases (CSV, XML, BIN, XML output by the dumper) are correct.

Tried to run all intrinsic metadata test cases through the XML parser and XML dumper.
The XML parser seemed to work correctly but need to check the bin files.

The XML dumper crashed on the test cases that include more than one metadata chunk but
processed all single-chunk intrinsic metadata test cases (both simple and complex).

TODO: Why did the XML dumper fail? Problem handling multiple chunks?

TODO: Need to write the size in the chunk header and the sizes in all nested tuples.

The XML parser wrote the second chunk header so the problem is that the XML dumper fails when it
tries to read the second chunk.

Fixed the problem with multiple chunks in the test cases.


### 2021-02-02

Fixed XML syntax error by modifying the Mini-XML whitespace callback to not add space before the XML declaration.

Modified the XML dumper to write the exact number of bytes specified for character strings.

Modified the XML dumper to add more node stack table entries for encoding curves.

Modified the XML dumper to strip trailing zeros from a floating-point number to match the input
to the XML parser. All intrinsic metadata test cases can be processed by the XML parser and dumper and
the output of the dumper exactly matches the input to the XML parser as verified using `xmllint` and
visually inspecting a few files using BBEdit.

Changes the the `generate.py` script to resolve differences between the streaming test cases
input to the XML parser and the XML files output by the XML dumper.

Very few streaming tuples have differences: `ISOG` and `SHUT` appear to be the only discrepancies.

Close to resolving all differences between the streaming metadata test cases input to the XML parser
and the output of the XML dumper. Tuples with differences: `MTRX` and `WRGB` appear to be the only streaming
test cases with differences.

GPMF parser does not handle vectors of floats with size equal to a multiple of the number size,
for example WRGB and MTRX. A WRGB tuple with size of 12 bytes and count equal to 9 in a CSV file
generated by the GPMF parser only has 9 floats, not 9 times 3 = 27.


### 2021-02-03

Able to round-trip all intrinsic metadata test cases through the XML parser and XML dumper and the output
of the XML dumper matches the input to the XML parser exactly.

Able to round-trip most streaming metadata test cases through the XML parser and XML dumper and the output
of the XML dumper matches the input to the XML parser exactly with a few exceptions:

1. The `hero6+ble.csv` test case contains tuples following the `acc1` metadata that do not appear to be valid.

2. The `ACCL` metadata from the Fusion camera contains floating-point values without a decimal point that
appear to be different but are identical.

3. The streaming metadata from the Karma drone contains `sdeg` and `drad` metadata that is not handled correctly.


### 2021-02-04

Modified the algorithms for normalizing floating-point values in the `generate.py` script and XML dumper
to fix issue #2 from yesterday.

Added code to the GPMF parser to skip bad tuples that contain only digits to resolve issue #1 from yesterday.

Added code to the `generate.py` script to skip metadata that contains spurious `sdeg` and `drad` tuples from
Karma drone metadata to fix issue #3 from yesterday.

Able to round-trip all intrinsic and streaming metadata through the XML parser and XML dumper with no discrepancies.


### 2021-02-05

Added code to the DPX dumper to output information about each DPX file, specifically the header size in bytes.
The size should be in the test case files for use by the XML parser.

Modified the `generate.py` script and the make file in the DPX subdirectory to use the information from the
DPX dumper when creating the DPX metadata test cases.


TODO: Clarify the division of work between the GPMF parser, generator script, and XML parser:

1. The GPMF parser should output tuples identical to the contents of the metadata stream in the MP4 sample files,
except that UTF-8 strings must be converted to type 'u' with the correct length in bytes.

2. The generator script must output complete and valid JSON and XML files with all tuple attributes filled in,
including the padding, with the exception that the value may be carried as element text instead of a value attribute.

3. The XML parser should read the XML input file and convert the metadata to binary without any changes.

4. The XML parser should verify that the input XML is correct and complete and terminate if the metadata fails verification.

All character strings (data type 'c' or 'u') must have size equal to the length of the string in bytes
and count equal to one.


Question: How to compute the true length in bytes of a UTF-8 string in C code?


Answer: Use this code snippet:
```C
int strlen_utf8(char *s)
{
    int i = 0, j = 0;
    while (s[i])
    {
        if ((s[i] & 0xc0) != 0x80) j++;
        i++;
    }
    return j;
}
```
from <https://stackoverflow.com/questions/3911536/utf-8-unicode-whats-with-0xc0-and-0x80>

For every string, compute the number of bytes using the code snippet above. If the number of bytes in the string
equals the length computed using `strlen()` and every byte is US-ASCII, then the string is US-ASCII; otherwise,
the string is UTF-8.


NOTES: US-ASCII strings (data type 'c') and UTF-8 strings (data type 'u') should have count equal to one and
size equal to the length of the string in bytes. This distinguishes a character string from a vector of characters.

Unsigned bytes (data type 'B') could be a scalar byte (size and count both equal to 1),
or a vector of bytes (size equal to 1 and count equal to the number of elements in the vector),
or a vector of vectors of bytes (size equal to the length of each vector and count equal to the number of vectors).

A block of bytes is base64b encoded if all if the following conditions apply:

1. The count is equal to 1, meaning that there is a single block of bytes, and

2. The size is equal to the number of bytes in the block (not the number of characters or bytes in the base64 encoded string), and

3. All characters in the string are characters used for base64 encoding.


A base64 encoding of a block of bytes cannot be confused with a vector of unsigned bytes or a vector of unsigned byte vectors
(both cases with data type 'B'), because a sequence of numerical data would have numbers seperated by whitespace.
The numbers could be decimal, hexadecimal, or any integer literal recognized by both C/C++ and Python
(the two progamming languages used in the VC-5 reference codec).


TODO: The GPMF parser must be simplified to remove changes to the metadata except as noted above for character strings.

TODO: The `generate.py` script should output complete JSON with all metadata tuple attributes included in every tuple
(including padding) and with all values correct. The JSON representation of nested tuples should have the correct size
for the tuple payload (size of the value and padding).

TODO: Since the JSON is complete and correct, the conversion from JSON to XML should be straightforward, except that
the metadata value can be carried as an attribute, element text, or CDATA with the decision determined by the tuple tag
using table-driven code.


Now that the overall workflow is working and testable, it should be easy to simplify the code and perform frequent regression testing.


Tasks:

- Make the code table-drive as much as possible.

- Fewer conditionals and switch statements, more functions from lookup tables.

- Can separate code for scalars and vectors be combined?

- Table to map GPMF data types to VC-5 data types (implemented in the GPMF parser).

- Change US-ASCII (data type 'c') to UTF-8 (data type 'u') for any string that contains multibyte characters
regardless of the tuple tag.

- Walk the tree of the internal representation of JSON metadata to replace missing or incorrect size attributes
in nested tuples.


Update the issues on GitHub to track the proposed changes.


TODO: Clarify that metadata character strings are either US-ASCII or UTF-8. Extended ASCII should be avoided.


### 2021-02-07

Worked yesterday and today to simplify the conversion of extended ASCII to UTF-8 encoding strings.
The code converts all strings to UTF-8 on the assumption that the strings may contain extended ASCII characters
rather than testing for the presence of characters outside the US-ASCCI characer set.

Found and fixed problems with the output of `TICK` metadata tuples and similar tuples.
The code had be outputting the pointer to the metadata value rather than the value itself.


Character strings (data type 'c' and 'u') do not have a repeat count so the count/size swapping in the GPMF parser
should be removed.

Need to remove the repeat count for data type 'B' in ST 2073-7 revision.


Change the XML semantics so that count equal to zero means that there is no repeat count and the size field is three bytes.

Any count greater than zero means that both the size and repeat count are present in the metadata tuple.

If the count attribute is not present in the tuple, then the count is zero.

Remove the phrase in ST 2073-7 Annex A about computing the type from the value (or from the data type) because it is not possible
to know if the value is an array of fixed-size arrays of elements.

If count is zero (or not present) and size is not present, then it can be assumed that the value is a scalar in which case
the size can be computed from the data type.

But note that for conformance testing, all attributes should be present in the tuple and not inferred if missing (including padding).

Modify the GPMF parser to implement this rule.

Modify the `generate.py` script to check for bad use of the count (the presence of a count is determined by the data type)
and fix the error.

Modify the XML parser to check for this error and flag any discrepancies.


Base64 encoding may be more trouble that it is worth. Perhaps better to represent binary data as a vector of decimal or hexadecimal bytes
separated by whitespace as is done for other vectors of numerical data.

Modified the XML parser and dumper to handle a large binary block (dataa type 'B') with the size factored into
a small size that fits in a single byte and a count, for example 2048 = 16 * 128 where 128 is the size and 16 is the count.

This hack only works if the total size can be factored into two integers that fit in a one byte size and a two byte count.

This solution is sufficient for RP 2073-2 but may want to amend ST 2073-7 to add a new data type for a block of bytes
(data type 'X').


TODO: Fix `generate.py` to factor large size into a smaller size and count. [DONE]


TODO: Fix GPMF parser to map tuple count to zero for data types that do not have a repeat count.
Replace the size with the product of the size and count if the count is greater than one. [DONE]


### 2021-02-08

Modified the GPMF parser to set the count to zero in tuples that do not have repeat counts.

Modified the `generate.py` script and the XML parser and dumper to handle DPX metadata files.

Able to successfully round-trip all intrinsic and streaming test cases and all DPX testcases through the
XML parser and XML dumper.


NOTE: [Base64](https://en.wikipedia.org/wiki/Base64) is far more widely used today than older formats
for binary data such as BinHex or uuencode.


Only need to support base64 encoding of binary data.

New data type 'X' for blocks of binary data: Use base64 encoding to represent blocks of binary data in textual
file formats such as JSON or XML. The size field is the true size of the binary block which may be padded with zeros
before base64 encoding. The padding in the base64 encoding does not include any padding that might be required to
round up the total size of the tuple to a multiple of four bytes (one bitstream segment).


NOTE: The binary block can be padded with zeros before base64 encoding if the content of the binary block
includes the actual size of the block.


### 2021-02-09

Changed the filename for the extra file information from `dpxfiles.csv` to `filesize.csv` to generalize the
filename for other classes of extrinsic metadata. Created a new issue on GitHub to merge the file size information
into the file information database `fileinfo.json` used for all extrinsic metadata but this may be hard as some
binary metadata is written by C programs. The CSV file format is simpler than JSON for C programming.

All comparison tests pass for the XML parser and XML dumper round-trip workflow.

Add code to the XML parser to pad the binary output from base64 decoding to the size of the metadata value
specified in the tuple header.

The padding scheme is working for the XML parser. The extra zeros in the metadata value will not affect the
interpretation of the value since the value contains its true size. But the XML dumper cannot distinguish between
actual data and padding so the base64 encoded string includes extra zeros at the end.

Added support for ACES metadata test cases to the XML parser and XML dumper.


### 2021-02-10

Added support to the `generate.py` and `dmcvt.py` scripts and make files for creating DMCVT metadata test cases
with the correct size and count (adjusted to fit in the size and count fields).

Debugged the generation of DMCVT test cases and added support for DMCVT metadata to the XML parser.


TODO: Should carry the DMCVT metadata in base64 format in the element text instead of a value attribute.


TODO: Modify the XML parser to process DMCVT metadata in the element text.


TODO: Check that the XML parser can handle base64 encoded extrinsic metadata in value attributes and element text.


### 2021-02-11

Created a top-level make file in the VC-5 metadata directory for building all programs and generating all test cases.

Added targets to run all test cases through the XML parser and XML dumper and verify the results using
the `diff-testcases.sh` script.

Created separate files for the each category of test cases in the intrinsic and extrinsic test case lists.

Changed the filename for the test case rules to `testrules.mk` to match other make files and avoid filename
conflicts with the testcase files.

Starting to refactor the code to merge separate functions for scalars and arrays into a single function that
handles scalars as arrays of one element.

Modified `dmcvt.py` script and XML parser to handle DMCVT metadata.

The DMCVT metadata is preceded by a 16-byte SMPTE Universal Label (UL) then a 4 byte length (BER encoded)
followed by a series of 24 (18 hex) local tags (two bytes each), a BER length (3 bytes), and the value of
the specified length.

The SMPTE Universal Label (UL) in the sample data provided by Raymong Yeung is wrong.

The DMCVT sample extracted from an MXF file provided by Raymond Yeung correponds to ST 2094-2 Table B.1 Example
DMCVT Application Set for SMPTE ST 2094-10 (application #1).


TODO: Consider manually changing the UL to the correct UL as defined in ST 2094-2 Table 4
```bash
06.0E.2B.34.02.53.01.01.05.31.02.01.00.00.00.00
```


### 2021-02-12

Debugged a discrepancy in the output of the `dmcvt.py` script and the contents of ST 2094-2 Table B.1
about the number of bytes in used for the BER encoding of the length in the KLV metadata. The example
in Table B.1 differs from the sample obtained from the MXF file provided by Raymond Yeung. My code is
working correctly on that sample data.

Added all working test cases for extrinsic metadata to the lists of test cases.

All test cases are working except for the MXF test case `part15-j2c.xml` because the output of the XML dumper
encodes the padding in the metadata value into the output string. Need to properly implement binary blobs.

Eliminated the separate code for dumping vectors of numbers.

Removed large blocks of unused code from the XML parser and the XML dumper.


### 2021-02-13

Modified the algorithm for reducing the size to fit in one byte when there is a repeat count.
The algorithm tries to factor the total size into two integers that fit in the size and repeat count fields.
If that fails, then the old algorithm is used but the total size may be larger by a few bytes.

Regenerated all test cases and ran the round-trip workflow through the XML parser and XML dumper.
All test case results from the XML dumper match the generated test cases.

Modified the generator script to include the size and count for ALE metadata in the JSON and XML files.

Modified the XML parser and XML dumper to work with ALE test cases.

Can round-trip the ALE test case through the XML parser and XML dumper and get results that match to XML file
produced by the `generate.py` script.


### 2021-02-14

Finished changes to the `generate.py` script and the XML parser and XML dumper to handle XMP metadata.

Started work on implementing dark metadata.
Modified the `dark.mk` make file and the `darken.py` and `generate.py` scripts to add the size of the dark metadata value
before encoding into base64 format.

The XML parser and XML dumper are working with both dark metadata test cases.

TODO: Final checks before declaring work on metadata generation complete.

TODO: Review the code in the XML parser and XML dumper in preparation for integrating the code into the
sample encoder and reference decoder.

TODO: Add code to ignore duplicate tuples and test on the simple intrinsic metadata test cases that contain randomized and
duplicate tuples in one or more metadata chunks.

TODO: Review and update the tasks in the GitHub issues tracker and organize the project board.


### 2021-02-15

Created GitHub pages website for the VC-5 Codec: <https://bgschunck.github.io/vc5codec/>

Configured my domain `vc5codec.org` to forward emails to my Gmail account using the Forward Email service:
<https://forwardemail.net/en/my-account/domains/vc5codec.org>

Not working, but perhaps need to wait longer for the DNS information to propagate.


### 2021-02-16

Enabled "Enforce HTTPS" in the GitHub Pages settings late Tuesday evening around midnight.

Now the request `https://vc5codec.org/` works and the request `http://vc5codec.org/` is changed to `https://vc5codec.org/` automatically.

Excellent.

Changed the top-level (root) for the website to `index.html` then changed it to `index.md` so that the website
content can be written in markdown.

The website is working fine.


### 2021-02-17

Website still working and got email forwarding to work again.

Cloned the VC-5 software on Windows.

Several changes to get the software to build on Windows, including adding additional header files,
installing Expat, and fixing the CMakeLists.txt file.

Can build the XML parser but cannot build the XML dumper. Need to build Mini-XML from source.

Cleaned up the `CMakeLists.txt` file and source code for the XML parser on macOS to try to resolve build errors
on Windows. Tracked down warnings after adding the `-Wall` flag to the debug build.

TODO: Clean up the `CMakeLists.txt` files and source code for the XML dumper.

Need a macro like ALLOC_BUFFER(variable, type, length) defined differently on Windows versus macOS versus other platforms.

Why does the C compiler on Windows not accept for loops that define the index variable:
```C
for (int i = 0; i < n; i++) {}
```

Force the Windows compiler to use C++ to compile the XML parser and XML dumper?


### 2021-02-18

Applied changes from the `CMakeLists.txt` file used by the XML parser to the XML dumper and fixed compiler warning
about an unused variable.

Configured Git to use Gedit for drafting commit messages.

Finally able to build XML parser on Windows using Visual Studio 2017.

Visual Studio has a package manager that installs Expat in the build tree.
Apparently, it also installed Expat in my home directory (`~/.nuget`).

Need to force use of the debug library: `libexpatd.lib` in the `CMakeLists.txt` file.

Had to use hexadecimal numbers in place of FOURCC strings and allocate buffers on the stack using `alloca`
as determined by a macro defined per the limitations of the build machine.

Removed the `malloc.h` header files from non-Windows builds since that file is not available (or needed) on macOS.


TODO: Check that the XML parser builds on all platforms.


TODO: Build the XML dumper on all platforms.


### 2021-02-19

Can build the XML parser on macOS and Ubuntu using make files and CMake.

Improved `CMakeLists.txt` to attempt to fix problems on Windows, but the location of the Expat library
is not added to Additional Library Directories in the linker settings. After adding the location manually,
the XML parser builds on Windows and can use the command:
```PS
cmake --build .
```
to build the XML parser from the PowerShell command line.

Fixed problems in the `CMakeLists.txt` file for the XML dumper and aligned the structure between
the XML parser and XML dumper.

Removed argp library from the `CMakeLists.txt` file for Linux builds.

XML dumper is broken: Works on most test cases but not for streaming.

Changes to the `CMakeLists.txt` file for the XML parser solved the problem of the Expat library location
missing from the Additional Library Directories in the linker settings.

The XML parser is not building without errors or warnings in Visual Studio 2017 and from the command line
using CMake to perform the build.

Fixed bug in the XML dumper that affected the processing of nested tuples.
Had added a test for failure to read the tuple payload but nested tuples do not have a payload that can be read.
The payload is the tuples inside the nested tuple that are read when the tuples are processed later.


TODO:

1. Need to test for a nested tuple tag since the size will be changed to be the size of the payload (no repeat count).

2. Undo commment on ALLOC_BUFFER in the XML dumper.

3. Add the size of the payload to nested tuples.


Third task has to be done in the `generate.py` since the XML parser uses Expat to read the XML file,
but the XML parser could tabulate the payload size and update the size even though that tuple would
have already been written to the output file.

If the XML file contains a size for a nested tuple, then the XML parser should use it.
But the XML parser should still calculate the actual size and either report the error or patch the bitstream (or both).


### 2021-02-20

Checked the make file in the metadata top-level directory `.../metadata` and all targets are working.
Can generate all test cases using the `generate.py` script, run the make and CMake builds for all programs,
run all test cases through the XML parser and XLML dumper, and verify the results.

Possible licensing problems with `argp-standalone` and may need to switch to `argparse` which has an MIT license.

If `argparse` works well for the metadata code, then consider using it instead of `argtable2-13` for all of the VC-5 codec software.


TODO: Switch from using `argtable2` and other command-line argument parsers to `argparse` which seems easier to use
and has an MIT license.


### 2021-02-21

Moved Google header files to archive subdirectory since the files are no longer needed for Visual Studio builds
and started compiling list of software licenses used in the VC-5 code base.

Modified the XML dumper to use the `argparse` library built in the `$(ROOT)/external` directory bu the `install.sh` script.

NOTE: CMake links against the static libraries by default but had to modify the make files to force static linking.

Write PowerShell script to build and install the external software on Windows.

Can build the XML dumper on all platforms with all build tools available on each platform.


TODO: Check that argument parsing is working correctly on Windows.


### 2021-02-22

Replaced libmxml installed by Homebrew with clone in external directory.

The XML dumper with new command-line argument parsing is still building and working on all three platforms
with both make files and CMake on macOS and Ubuntu.

The make files and `CMakeLists.txt` file specify use of static libraries to avoid having to add dynamic libraries to the executable path.

Changed the Windows build of the XML dumper to use the Mini-XML library cloned into external but got error:
"fatal error C1083: Cannot open include file: 'stdio.h': No such file or directory"

Changed the Windows SDK to version 10 (was version 8.1).

This fixed the problem.

But Windows is still trying to use the dynamic library for Mini-XML instead of the static library as specified in the `CMakeLists.txt` file.

If the dynamic library is added to the path, then the XML dumper runs correctly.

Modified the XML parser to use the argparse library (same as the XML dumper).
On macOS, Expat builds a dynamic library but do not have to add the library location to the executable path.
The XML parser is linking against `/usr/lib/libexpat.1.dylib` instead of the library in the external directory.

Got the XML parser working on Windows (both the debug and release configurations) after some changes to the `CMakeLists.txt` file
but the Expat libraries are stubs for the dynamic libraries so have to add the locations (absolute pathnames) of the dynamic libraries to the path.

Split the metadata tools into separate directories to prepare for the conversion of the tools from Argp to Argparse.

Started modifying `bintool`.

Added intall target to copy the executables to the `$(ROOT)/metadata/tools/bin` directory.


### 202-02-23

Modified the make files to find the metadata tools in the new `bin` subdirectory of `$(ROOT)/metadata/tools`.

Finished converting `bintool` to use the `argparse` library instead of Argp.

Able to build `bintool` on Ubuntu but not the other tools that still use Argp.

Converted the `dpxdump` program to use the `argparse` library instead of Argp.

Copied `libargparse.dylib` to  `$(ROOT)/metadata/tools/bin`.

Could not run `bintool` after installing in `$(ROOT)/metadata/tools/bin` because the arparse dynamic library was not found.

Used `install_name_tool` to add the dynamic library:
```bash
install_name_tool -add_rpath @executable_path/. bintool
```

Modified the make files in `$(ROOT)/metadata/tools` to install `libargparsedylib` if missing and use
`install_name_tool` to add `$(ROOT)/metadata/tools/bin` to the `@rpath` for each executable tool
in `$(ROOT)/metadata/tools/bin`.


Need to use the command:
```bash
patchelf --set-rpath . bintool
```
to set the `@rpath` on Ubuntu.
Modified the make files to perform the same action for all tools.

Tested the code on Ubuntu and macOS.

Tried an experiment with the XML dumper on Windows. Could not run the dumper because the location of a DLL
was missing from the path. Copied the DLL into the same folder as the executable and it worked.
Did not have to use any tool since the directory containing the executable is the first place that is
searched by the algorithm for resolving DLL locations at runtime.

Only need to install `mxml1.dll` in the same directory as the XML dumper executable.
Apparently, the `argparse` library is linked statically or CMake uses some magic to record its location
in the XML dumper executable.

The XML dumper will not run if `mxml1.dll` is missing from the directory that contains the XML dumper executable.


### 2021-02-24

Modified the `CMakeLists.txt` file for the XML dumper to only copy a DLL to the same directory as the executable
on Windows platforms.

Removed unused code from the XML parser and dumper and the metadata tools.


### 2021-03-01

Performed `make clean-tests` and `make all-tests` in the `$(ROOT)/metadata` directory.

The `generate.py` script regenerated all test cases, the XML parser recreated all of the binary files,
the XML dumper recreated all of the test case results in XML format, and all test case results were verified
against the test cases in XML format generated by the `generate.py` script and input to the XML parser.


TODO: Need to make changes to the XML dumper to correctly nest tuples in extrinsic and dark metadata.


Performed `make clean-all` in the VC-5 software root directory.

Rebuilding the codec and utilities (debug and release configurations) by running `make all`.

Successfully rebuilt the debug and release configurations of the encoder, decoder, and utilities.


Added VC-5 metadata (part 7) to the configuration.

Started to add code to the sample encoder to read metadata files in XML format.

Added command-line option `-M` to read one metadata file in XML format for injection into the bitstream.

Tested that the metadata filename is read from the command-line and available in the parameters returned
to the main program.


TODO:

1. Need a stack to hold the file pointer to chunks and nested tuple headers to set size after the payload is written.

2. How to get the file pointer from the bitstream data structure?

3. Align the bitstream to a segmenet boundary before writing the metadata chunk header.


### 2021-03-02

The bitstream passed to the encoder funtions contains a pointer to the stream which contains a file pointer
(if the stream was opened to a file, otherwise a memory buffer).

The encoding process arguments are the encoder state, unpacked image (output of the image unpacking process),
bitstream, and parameters passed on the command line, including the pathname for the metadata test case in XML format.

The encoding process calls `EncodeSingleImage` to encode the image and that routine calls `EncodeBitstreamTrailer`
to finish writing the bitstream and then calls `FlushBitstream` to force any data remaining in the bitstream to be
written into the encoded bitstream.

Need to write the metadata chunk before writing the trailer and flushing the bitsteam.

Four options:

1. Add the metadata filename an argument to `EncodeSingleImage`.

2. Add the parameters as an argument to `EncodeSingleImage`.

3. Refactor `EncodeSingleImage` to move the calls to `EncodeBitstreamTrailer` and `FlushBitstream`
into the caller (`EncodingProcess`).

4. Add the metadata filename to the encoder state which is passed as an argument to `EncodeSingleImage`.


Option #1 (add the pathname) is a hack.

Option #2 (add parameters) seems okay but other parameters are already copied into the encoder state (see option #4 below).

Option #3 involves more changes and leads to an unbalanced design for `EncodingProcess` and  `EncodeSingleImage`.

Option #4 is consistent with the current code that copies parameters into the encoder state.


Could open the metadata file and pass the pointer in the encoder state and this could be done
in `PrepareEncoder` or in `EncodingProcess`. The later is better as `PrepareEncoder` is already a
large routine devoted to creating the internal encoding data structures.


Chose option #3 because `EncodingProcess` writes the bitstream start marker so it makes sense that it
should write the bitstream trailer and flush the bitstream (after writing the metadata chunk).


TODO: Verify that the encoder still works before making any changes.


Ran all codec tests using `testcases.tcl` and all tests passed:
```bash
 ./testcodec.tcl -v
Testcases {solid gradient boxes} done using release build (38 tests, 0 failures)
```

Did not have to install Tcl or make any changes.

The default arguments (with or without the verify switch) worked brilliantly.


TODO: Copy `metadata.h` into separate include directories for the encoder and decoder.


Can build the encoder on macOS using either make or CMake.

Modified the encoder to add routines that will wrap the routines from the XML parser.


TODO: Add the XML parser routines (excluding the main program) to the encoder build.


Lots of work on adding the XML parser routines to the encoder build.
Fixed many problems with the make files and `CMakeLists.txt` file.
Able to build the encoder on macOS using make or CMake (both debut and release configurations).
The all four combinations of the encoder (build tool and configuration) run and display the help
message for the command-line arguments.


TODO: Run the encoder on existing test cases to verify it still works.

TODO: Run the encoder on a simple metadata test case and check that the metadata is encoded
into a metadata chunk element.

Need to add code to set the metadata value size and count after the payload is written.


TODO: Check the round-trip workflow through the XML parser and XML dumper.


### 2021-03-03

The sample encoder is writing metadata into the bitstream but the metadata chunk size is zero.


TODO: Modify the `generate.py` script to set the size of the metadata chunk and nested tuples.


Walk the JSON tree and record the size of each metadata value and the metadat tuple padding.


### 2021-03-04

Adding tuple size and padding to the JSON output.

It appears that `output_json_metadata` is the only function in `generate.py` that outputs JSON to a file.

Would prefer to keep `output_json_metadata` focussed solely on outputting JSON to a file and
not overload the routine with other responsibilities, but it is a very short routine and easy
to add a call to `update_payload_size`.

Have to add another routine to calculate the size field in a metadata chunk element since the JSON files
contain only tuples, no metadata chunks. Some JSON files have a metadata class instance, others do not.

May be better to leave the JSON files alone and add payload sizes and padding to final XML files.

The routine in `generate.py` that outputs the XML is 'output_xml_metadata' and is a very short routine
that can be modified to call `update_payload_size`.

Decided that the best solution is to add a routine to `output_xml_metadata` called `update_payload_size`
to recursively walk the XML node tree to compute the size of children and updated the parent.
The routine appears to be working.


TODO: Remove the count attribute if the tuple data type does not have a repeat count.


If the tuple data type has a repeat count, then the count should be at least 1.

Fixed `generate.py` to omit the repeat count if the data type does not have one,
otherwise set the repeat count to at least one.


TODO: Need to code the 'PQEC' encoding curve.


### 2021-03-05

Finished first version of `verify.py` script to verify the correctness of the XML files created by the test case generator.

TODO: Add code to check parent-child relationships

TODO: Test all categories of metadata

Need to rerun all metadata test cases through the XML parser and XML dumper and verify the results.


### 2021-03-08

Add tests to `verify.py` script to check that all child tuples below to the parent tuple and all child tuples
that are required to be present in the parent tuple are in the parent tuple payload.

Added information about the parent-child tuples for extrinsic metadata.

Saved existing XML test cases from before the changes to `generate.py` to update size, count, and padding
to `xml.bak` folder, moving existing `xml.bak` folder (if any) to `Archive`.

Re-generated all testcases in JSON and XML formats.

Ran all testcases through the XML parser without any problems.

But the XML dumper fails on the complex metadata test cases with bad tuple tag and type.

Fixed problem in the computation of the metadat tuple sizes: the size of each metadata tuple header is 8 bytes, not 4 bytes.
The solution involved a Numbers spreadsheet saved in `~/Documents/SMPTE/VC-5/VC-5 Part 2 Conformance/Metadata Chunk Size`.

Now the output of the XML parser and the sample encoder match exactly.

But there are differences between the output of the XML dumper and the input to the XML parser.

The XML dumper is not generating XML output with the size and padding, count with value zero is output for tuples
that do not have a repeat count, and the children of the GAMA and LAYR tuples are not output.


TODO: Simplify the state machine table for nested tuples then add necessary entries


### 2021-03-09

Fixing problems in the XML parser and XML dumper with metadata tuples that have type zero.
Need to convert between numeric zero and character '0' and not output the count in nested tuples.

Problems with `LAYR` and `GAMA` seem to have been fixed in both the XML parser and XML dumper,
but need more testing.

Does `make dump` and `make verify` work on all metadata test cases?


TODO: Need to apply the changes for the `GAMA` tuple to the tuples for the other encoding curves.


### 2021-03-10

Fixed problems in the generator script and XML dumper in the formatting of encoding curve elements.

Test case `Fusion.xml` has incorrect chunk size in the binary output of the XML Parser: 21469 (incorrect) versus 87005 (correct value).

Fixed the computation of the chunk size by using unsigned data types with sufficient precision.

Added targets to all make files to verify the test cases in XML format.

Sample encoder fails when part 7 is enabled at runtime but no metadata file is provided on the command line.

Added make file targets to compare the bitstreams with embedded metadata created by the sample encoder
with the binary files created by the XML parser.


TODO: Fix the problem with the sample encoder when part 7 enabled but no metadata file [Done]

TODO: Add command-line switch to the sample encoder to suppress all output to the terminal [Done]


### 2021-03-11

Fixed problems with the sample encoder. Placed terminal output under the verbose switch and added quiet switch
to completely suppress all output to the terminal.

Missing metadata input file is treated as an error.

Wrote `verify-metadata.sh` script to run the encoder, extract the embedded metadat chunk from the bitstream,
and compare that binary file with the corresponding file created by the XML parser.

Added targets to the make files for the sample encoder to verify all metadata test cases and clean up the results.

Added code to the `verify-metadata.sh` script and the make file for the sample encoder to correctly
form the pathname to the binary files output by the XML parser from the pathname to the metadata test case.

Can now verify the binary metadata embedded in all encoded bitstreams.


### 2021-03-12

Better idea for entering metadata tuples by hand.
Add fake tuples with PUSH and POP tags:
```
_PUSH,CFHD,0
_PUSH,LAYR,P
LAYN,s,2,1,0
LAYD,c,9,0,"Layer one"
_POP
_PUSH,LAYR,P
LAYN,s,2,1,1
LAYD,c,9,0,"Layer two"
_POP
_POP
```

Should be easy to translate this flattened representation of nested tuples into JSON or XML.

Could also have other directives such as `_INCLUDE` to insert this CSV-style representation of
metadata from another file.

In general, would have created DSL for metadata tuples that was convenient to enter by hand
but could be translated into JSON or XML using very simple algorithms.


TODO: If any metadata test cases have problems with nesting tuples into the proper hierarchy,
consider using this new CSV scheme or other DSL representation.


Write new Python script `nested.py` that converts new CSV format with push and pop commands
to JSON format.

Regenerated the metadata test cases in XML format and reran the XML parser and dumper tests.
All output files from the XML dumper verified against the metadata test cases.

Improved the encoder make file to pass the root directories for metadata and binary files to
the `verify-metadata.sh` script.

Verified that the encoder correctly embedded all metadata test cases using the `verify-metadata.sh` script
by invoking the make file target:
```bash
make verify-metadata
```

The debug configuration of the encoder was used to encode an input image and embedded each
metadata test case. The `dumper.py` script was used to extract the metadata chunk and the
extracted chunk was compared with the corresponding binary file created by the XML parser.

All test cases passed.


TODO: Need to compute the width and height of the encoded image from the filename.


### 2021-03-13

Created a new make file for generating multiclass metadata test cases.

Had to modify `generate.py` to create new commands for merging diverse class instances.


### 2021-03-14

Added the multiclass metadata category to the list of test cases to generate in `$(ROOT)/metadata/python.makefile`.

Added test cases for multiclass metadata to the XML parser and XML dumper.

Ran the new multiclass test cases through the XML parser and XML dumper workflow:
all tests passed.


### 2021-03-16

Visually inspected the metadata test cases and reported the results in `$(ROOT)/metadata/notes/testcases.md`


TODO: Why are there no duplicates in the intrinsic simple test cases that are supposed to test the handling of duplicate tuples?


Looked at `generate.py` and ran some tests. Nothing wrong with the code. Only idea for the absence of duplicate tuples
is that the number of tuples in the output file was only 10. Seems unlikely that there were at least one or two duplicates.
Change the number of output tuples to 40 and now have lots of duplicate tuples in the test cases.


TODO: Need to add multiclass metadata test cases with more than one class instance from the same metadata class.


### 2021-03-17

Changes to the build scripts for the reference decoder to match changes made some time ago to the
sample encoder to resolve build problems.

Added make file definitions to add the Mini-XML parser and a few changes to the reference decoder
source code to get the decoder to build.

Have not started to add support for part 7 metadata to the reference decoder.

Added command-line arguments for metadata to the reference decoder including flags that were added
to the sample encoder.


Tested the new command-line arguments for the reference decoder and created a new script `test-decoder.sh`
for testing the decoder (similar to the script `test-encoder.sh` for testing the sample encoder).

Cloned the [uthash](https://troydhanson.github.io/uthash/) package from [GitHub](https://github.com/troydhanson/uthash.git).

The `uthash` package is a complete hash table implementation accepting general key-value pairs and appears to be
easy to program in C language. Found several hashing function implementations, but `uthash` was the only package that
provided a complete hash table implementation, ready to use.

Since `uthash` is not a library, only a header. No source code must be built.


### 2021-03-18

Realized that `uthash` cannot be used since need to kep the class instances and tuples (and chunks) in the same
order as in the bitstream; otherwise, will not be able to easily compare the original XML with the output from
the reference decoder.

Need a three-level hierarchy of lists of lists: chunk, class, top-level tuple.

Treat steaming data as a special case since it is normal for there to be multiple instances of the same tuple.


Design of the tuple database:

The tuple database should be built as the chunks, class instances, and tuples are read from the bitstream.
Replace tuples that are duplicates with tuples found later in the bitstream (except for streaming data).

Output the XML tree from the database. This will eliminate from the output XML duplicate tuples but if the
database maintains the order of the chunks, class instances, and tuples read from the bitstream, then the
output XML will be identical except for missing tuples that were duplicates of later tuples.

The reference implementation does not have to be efficient and perhaps should not be. A more straightforward
implementation is preferred.

Algorithm:

1. Read the metadata from the bitstream and create the entire XML tree.

2. Walk the XML tree and create the database.

3. Walk the database and output the XML tree with duplicates removed.

Instead of explicitly creating a database, could just walk the XML tree and remove duplicates
but this would still require create a list of tuples in each class instance and then removing
duplicates from the list before outputting the XML tree. Essentially the same operation.

Explicitly creating the database mirrors what might be done in a more efficient implementation.

Creating the entire tree before creating the database and removing duplicates maintains compatibility
with the current implementation of the XML dumper that does not remove duplicates. Better to keep code
that is working now rather than change it.

Replaced `argtable2-13` with the Getopt code in `iotivity` and fixed problems building the sample encoder
with CMake to prepare for building the encoder on Windows.

Replaced `iotivity` with `ya_getopt` that includes `getopt_long` and has a BSD style license.

Able to build the encoder on macOS and Windows but the Windows build has warnings.

Resolved the build errors and warnings on Windows.

Able to build the encoder on macOS and Windows without errors or warnings using make and CMake.


### 2021-03-19

Editing `ya_getopt.c` to replace unsafe `getenv` with `_dupenv_s`, protected by the compile-time variable `_MSC-VER`
to eliminate a Windows build warning about unsafe functions. Checked the changes into my local clone of the repository
but have not pushed the changes to GitHub.

Edited the `comparer` tool to eliminate warnings from the CMake build on macOS.

Able to build the sample encoder, reference decoder, comparer and converter using CMake by
invoking the top-level make file in `~/Projects/SMPTE/VC-5/smpte-10e-vc5` with the command:
```bash
make TOOL=cmake
```
Can clean all CMake builds using the commands `make TOOL=cmake clean` or `make TOOL=cmake clean-all`.

Updated the test cases in `$(ROOT)/metadata/python/testcases`.

Cleaned and rebuilt all testcases in the Python subdirectories (not the `./testcases`).

Ran the full workflow through the XML parser and XML dumper. All test cases verified.


Ran all test cases through the sample encoder:
```bash
make verify-metadata
```

All test cases passed, including the new multiclass test cases.

Started work on adding code to the XML dumper to walk the XML tree and build the database.

Able to copy the XML tree correctly, some differences due to formatting that should be able to resolve.


TODO: Remove duplicate tuples as the database is created.


### 2021-03-20

Added make files to automate CMake builds for the XML parser and XML dumper.


TODO: Need to create `CMakeLists.txt` files for the metadata tools and then add CMake builds to the make files.


Added CMake build targets to the top-level make files for the XML parser and XML dumper.

Replaced inline code in the XML dumper with calls to the database routines.
Not actually a database but the routines mimic a database API except that the arguments are
Mini-XML nodes rather than the data read from the bitstream.


TODO: Read data from the Mini-XML nodes and call database routines with the data that was read
from the bitstream.


Added skeleton code for a database implementation that would create the database as the metadata
is extracted from the bitstream. The XML tree could be created from the database if it is needed.


### 2021-03-21

Created a stub routine in the XML dumper to read binary metadata into a database. The idea is to
mirror the logic in `DumpBinaryFile` but create a database, not an XML tree, then traverse the
database to create and output the XML tree for comparing the result with the original test case
input to the XML parser.

Fleshed out the skeleton code for reading metadata from a binary file (or bitstream) and inserting the
metadata into a database without creating an XML tree. Need more code and lots of testing.

Split off the main routine for the XML dumper into a separate file in preparation for integrating
the XML dumper into the reference decoder.


### 2021-03-22

Created scripts for testing the database code in the XML dumper. Sticking with the code that creates
the database from the XML tree to get something working quickly.


Starting to modify XML dumper to remove duplicate tuples.

The [mxmlRemove](https://www.msweet.org/mxml/mxml.html#mxmlRemove) to remove a node from its parent.

Code for removing duplicates seems to be working but need more testing.

Added code to search earlier chunks for duplicate tuples.

Added code to not remove duplicate streaming data or layer metadata.


TODO: Need to remove duplicate layer metadata for the same layer number.


### 2021-03-23

Started to integrate the XML dumper into the reference decoder.

Able to connect the database and metadata source files from the XML dumper to the reference decoder,
but not sure the method of creating the XML tree, then the database, will work.

Connected the XML dumper to the reference decoder. Ran the `testcodec.tcl` script and all tests passed.

Ported `display.py` from `~/Projects/Cedoc` to Python3 in the `$(ROOT)/scripts` directory.

The image encoded by teh sample encoder with metadata and decoded by the the reference decoder does not
match the image input to the encoder. Expect an error in the scripts now being used to test the encoder
and decoder because the regression tests performed by the `testcodec.tcl` script all pass.

Able to parse the metadata chunk element in the bitstream after decoding has finished,
skip the metadata chunk payload, and cleanly exit the decoder.


### 2021-03-24

Commented out the code for creating a database from the XML tree created by `DumpBinaryFile`.
Do not need a second XML tree. Can prune duplicate tuples from the XML tree created from the
binary metadata read from a file.

Retain the database API for creating a database (not an XML tree) from the binary metadata read
from a bitstream by the reference decoder because that is what most applications will do.

In the reference decoder, use a database API but create an XML tree and prune duplicate tuples
from that tree because that is what is needed for testing conformance.

TODO: Adapt the code from `DumpBinaryFile` to create the XML tree in the reference decoder and
prune duplicate tuples from that XML tree.

Can remove all of the code included by setting `_NODEBASE` to 1 and may not need the `_DATABASE`
compile-time switch. An application can modify the database API to build a database instead of an
XML tree. The procedure for creating a true database will be similar in structure to the code that
created an XML tree. For the purposes of the reference decoder, an XML tree is a perfectly good
realization of a tuple database.

Continuing refactoring XML dumper the code into three modules: dumper, nodetree, and database.

Nodetree contains routines for manipulating the XML tree including determining the level in the
node hierarchy for the next ruple.

Dumper provides all the routines for formatting values for output as XML attributes and element text.

Database provides an abstraction layer so that the XML-specific code can be replaced by a true metadata database
in the future.

Used compile-time switch to omit code that is not used by the reference decoder.

Able to build the reference decoder using the database proxy provided in the XML dumper.


TODO: Update the `CMakeLists.txt` file to build the reference decoder on Windows.


Next steps: Copy code from `DumpBinaryFile` into the proxy database routines used by the reference decoder.

Can the proxy routines be tested by running the XML dumper?


### 2121-03-25

Added [uthash](https://troydhanson.github.io/uthash/) to the XML dumper and modified the code for the
metadata database to use `uthash` for the table of tuples.

Only need to handle intrinsic metadata. Do not need to handle dark metadata, that is application-specific.
Extrinsic metadata is likely to be a "pass through" with the extrinsic metadata payload written to a file.

Streaming data will most likely be stored in a separate track in the container (need to revise ST 2073-10).
At the very least, streaming data cannot be usefully contained as metadat attached to a single frame so will
need to spread the streaming data across several frames and this is outside the scope of ST 2073-7 Metadata.

Added scaffolding to the XML dumper to test the code used by the reference decoder. Created testing routine
`DecodeBinaryMetadata` that calls the same routines used by `DecodeMetadataChunk` in the reference decoder,
except the binary metadata will be read from the same files used by the other code paths in the XML Dumper
and the reference decoder has already found the metadata chunk before calling `DecodeMetadataChunk`.

Finished coding the scaffolding and the XML dumper builds with the `_TESTING` switch enabled. Reran all of
the metadata test cases through the XML dumper and all passed.

Debugged the code using the ` simple-rdc1.xml` test case. Getting the same results as when the same test case
was processed by `DumpBinaryFile`.


### 2021-03-26

Ready to finish integration of the XML dumper into the reference decoder.

Reran the codec testing script `testcodec.tcl` with no failures.

Finished the coding for integrating the XML dumper into the reference decoder.
Can build the decoder without errors.


TODO: Begin testing the sample decoder on bitstreams created by the encoder that contain metadata chunk elements.


### 2021-03-27

Added `buffer.h` to the encoder to define `ALLOC_BUFFER` and `ALLOC_STRING` as done for the decoder.

Changed `BandValidMask` to declare the output as `uint32_t` (not `bool`)

Can build the sample encoder on macOS using make and CMake.

Able to build the sample encoder on Windows with CMake (using the `make.ps1` script).

Added a database parameter to more routines in the decoder.
May have covered all the cases where a database can be used.

The reference decoder builds on macOS and Windows with and without metadata enabled in the `config.h` file.

Got the reference decoder working with metadata on macOS but the program does not output the XML file.


In the `main` routine:

1. Create the database in the main routine.

2. Pass the database pointer to subroutines.

3. Output the database after the bitstream has been decoded.

4. Destroy the database in the main routine.


When the database is created in the `main` routine, it is ready for use and needs no further initialization.
It is passed down to `DecodeBinaryChunk` to process the metadata chunk elements in the bitstream
(called from `UpdateCodecState`).

Fixed bug that caused no metadata output after decoding.
The root node was not initialized so the tree was not created.


### 2021-03-28

Reran the `testcodec.tcl` script and all tests passed.

TODO: Still need to track down the problem in the output of the decoder in RG48 format.

The tests are passing since the output from the encoder matches the master.
Or does it?
Check that the test script is creating new encoded bitstreams and using the output of the reference decoder
from each bitstream to the compare with the master image computed long ago.

Started on creating a Python version of the `testcodec.tcl` script.


### 2021-03-29

Created a `CMakeLists.txt` file for building the metadata tools and cleaned up the directory layout:
moved the `config.h` file from the `bintool` subdirectory to a new common directory for metadata tool
include files.


### 2021-03-30

Started to code `CMakeLists.txt` file in the top-level directory using `add_subdirectory` to reference
the `CMakeLists.txt` file in the encoder and decoder subdirectories (and more later).

Getting CMake build problems.

Worked on aligning the `CMakeLists.txt` files for the encoder and decoder to combine the best practices
from each file.


PROBLEM: The conditional for debug/release configurations is not work: Always get the settings for a release build.


### 2021-03-31

Fixed problems in the CMake builds of the encoder and decoder.
CMake was not generating code for both the debug and release configurations.
The test for the configuration in the `CMAkeLists.txt` file was not working.

Cleaned up some warnings in the debug and release builds.


### 2021-04-05

Need to change the layout of media subdirectories for metadata to match RP 2073-2.


#### Source files

The files from which the metadata test cases were created are in `$(ROOT)/metadata/media` in subdirectories
by category of test case: `aces`, `ale`, `dmcvt`, `dpx`, `mxf`, `xmp`.

TODO: Need to add `README.md` files documenting the source of each category of file and
how the file was processed to obtain the CSV, JSON, and/or XML files that comprise each test case.

Streaming metadata is a special case: The MP4 files from which the streaming metadata was extracted are in
the directory `~/Projects/GPMF/gpmf-parser/samples`. The GPMF source code and sample files were cloned into
`~/Projects/GPMF/gpmf-parser` from <https://github.com/gopro/gpmf-parser.git>.

The CSV files extracted from the MP4 files by the modified `gpmf-parser` program are in `$(ROOT)/metadata/python/GPMF`.


#### Test Cases

The test cases in RP 2073-2 Annex C are in `$(ROOT)/metadata/python/testcases` with the working copies
in subdirectories of `$(ROOT)/metadata/python`: `intrinsic`, `streaming`, `extrinsic`, `dark`, and `multiclass`.
The working copies are copied into the `testcases` directory tree by the `update.py` script invoked from the `update` target
in the make file in the `python` directory: `$(ROOT)/metadata/python/Makefile`.

The testcases in XML format are listed in Tables 10 through 21 in RP 2073-2.


#### Conformance

The files used for verifying conformance are in `$(ROOT)/media/metadata` in four subdirectories that correspond to
tables 22 through 25 in RP 2073-2 as follows:

Table 22. Input metadata: `$(ROOT)/media/metadata/input`

Table 23. Reference bitstreams: `$(ROOT)/media/metadata/bitstreams`

Table 24. Encoded metadata: `$(ROOT)/media/metadata/encoded`

Table 25. Reference metadata: `$(ROOT)/media/metadata/decoded`

The input metadata is the test cases in XML format input to the sample encoder.

The bitstreams are created by the sample encoder and contain the embedded metadata.

The encoded metadata is the binary metadata extracted from the bitstreams.

The reference metadata is the metadata decoded from the reference bitstreams by the reference decoder
with duplicate metadata tuples removed as specified in ST 2073-7.


#### Test Cases

Move the root directory for the metadata testcases from `$(ROOT)/metadata/python/testcases` to `$(ROOT)/metadata/testcases`
to match the locations of the other files that listed in the tables in  RP 2073-2 Annex D.


### 2021-04-13

Worked on documentation for the VC-5 software. Edited existing README files and created new README files.


### 2021-04-19

Finished the section of the `deploy.py` script to copy the input metadata test cases to the media directory:
`$(ROOT)/media/metadata/input` and copied the input metadata test cases to that directory.

Used `test-encoder.sh` to encode the `simple-c01.xml` test case using the `/boxes-1280x720-0000.rg48` image.
Placed the output bitstream in `$(ROOT)/encoder/metadata/bitstreams` directory.
The new bitstream differed significantly from the corresponding bitstream in the `$(ROOT)/encoder/test` directory.

Decoded the new bitstream using the reference decoder and displayed the original and decoded images using `display.py`.
The two images look the same.

Had seen differences in the decoded images before. Apparently, the problem fixed itself.

Created scripts and make file targets for creating, verifying, and removing encoder metadata test cases.


TODO: Create scripts and make file targets for the decoder metadata


TODO: Deploy the encoder and decoder metadata to `$(ROOT)/media/metadata`


### 2021-04-25

Looking at failures of the decoder in making some test cases.

Observations:

1. The test case `simple-c23.xml` has multiple chunks.
Is the encoder correctly inserting chunks into the bitstream?
Is the decoder correctly handing more than one metadata chunk element?

2. The XML parser generates good correct binary file for the `simple-c23.xml` test case.

3. The output from the XML dumper for the `simple-c23.xml` test case looks correct.

4. All XML files created by the XML dumper pass verification.


Found the bug: Was not handling the case of multiple metadata chunks.
Was not counting the remaining payload size to determine the end of the metadata chunk payload.

Checked the decoded metadata test case `simple-c23.xml` and it looks the same as the original input
created by the Python `generate.py` script.

The command:
```bash
diff metadata/decoded/simple-c23.xml ../metadata/dumper/output/intrinsic/simple/xml/simple-c23.xml
```
shows no differences between the output from the reference decoder and the output from the XML dumper.


Still getting failures for all streaming metadata test cases, all other test cases are passing.

The XMP metadata test cases were not decoded correctly. The output was truncated. Possibly the same
problem that causes the streaming data test cases to fail, but somehow not triggering the assertion.

Looked at the bitstream `BlueSquare.vc5` and it looks correct.

Decoder is not decoding the `BlueSquare.vc5`  bitstream and not getting far enough to output any
debugging information.

Found very old bug in testing chunk elements to determine whether the chunk element is a codeblock.


The `Fusion.vc5` test case is the only test case that is failing.


Found old bug where the large chunk tag contained the high byte from the large chunk size.


### 2021-04-26

Created all metadata test cases for Annex D and verified that the test cases match the output from
the XML parser and XML decoder.

Removed intrinsic nested metadata test cases from the list of deployed test cases.

TODO: Verify that duplicate tuples are removed in all cases where the tuples are supposed to be removed
from the output of the reference decoder.

Edited the general install and release notes in `$(ROOT)/notes`.

Created Doxygen configuration file and edited the make file for the metadata tools to create
documentation for the tools.


### 2021-04-27

Updated the install and release notes and added make file targets to prepare the software for release.

Checked the metadata test cases listed in RP 2073-2 Annex D into the repository using Git LFS for the
bitstreams and encoded (binary) data and ordinary Git (text) for the test cases in XML format.

Worked on the new `testcodec.py` script for testing the codec. The script is far from complete, but the
old `testcodec.tcl` script can be used for testing parts 1-6.

Working on getting the new script to test part 7.

Able to run the encoder and dumper to create the bitstreams with encoded metadata and dump the metadata
to binary files, both set of data in the `$(ROOT)/encoder/temp` directory tree.


TODO: Compare the bitstreams and binary files with the files in `$(ROOT)/media/metadata`


TODO: Add the decoder to the `testcodec.py` script including comparing the decoded metadata with
the corresponding files in `$(ROOT)/media/metadata/decoded`


Finished the `testcodec.py` script for testing metadata. Seems to work but there are small syntactic differences
in the metadata output by the decoder.


TODO: Need to use a more powerful comparison tool


Look at the Python `xmldiff` package:

<https://pypi.org/project/xmldiff/>

<https://xmldiff.readthedocs.io/en/stable/api.html>


### 2021-04-28

Finished script `compare.py` to compare two or more XML files using the [xmldiff](https://xmldiff.readthedocs.io/en/stable/api.html) package.

Finished script `testcodec.py` for running the metadata test cases.

All metadata test cases pass (no failures).


Added test-metadata target to the top-level Makefile to run the new `testcodec.py` script.
After the `testcodec.py` script is finished, it can replace `testcodec.tcl` in the test target.

Added the version number to the XML parser and dumper source code documentation.

Updated the project board on GitHub.






