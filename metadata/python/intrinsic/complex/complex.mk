# Make file to generate test cases for complex intrinsic metadata

# Filename of this make file
MAKEFILE = complex.mk

# Default output directory (can be overridden from the command line)
OUTPUT = ./xml

# Root directory for the VC-5 reference software
ROOT = ../../../..

# Python script for creating test cases
GENERATE = $(ROOT)/metadata/python/generate.py

# Python script to create make file rules for complex intrinsic metadata test cases
COMBINATE = ./combinate.py

# Python script for verifying the correctness of metadata test cases in XML format
VERIFY = $(ROOT)/metadata/python/verify.py

# Files used to create the complex intrinsic metadata test cases
#INPUT_FILES = simple.json intrinsic.json

# Locations of the complex metadata files in JSON or XML format (used by the make rules)
COMPLEX_JSON_DIR = ./json
#COMPLEX_XML_DIR = ./xml
COMPLEX_XML_DIR = $(OUTPUT)

# Locations of the simple and nested metadata files used to generate the complex test cases
SIMPLE_JSON_DIR = ../simple/json
NESTED_JSON_DIR = ../nested/json

# Intermediate files for complex intrinsic metadata test cases in JSON format
JSON_OUTPUT_FILES = $(shell cat complex-json.targets)

# Location of the complex intrinsic metadata test cases in JSON format
JSON_OUTPUT_DIR = ./json

# Complex intrinsic metadata test cases in XML format
XML_OUTPUT_FILES = $(shell cat complex-xml.targets)

# Location of the complex intrinsic metadata test cases in XML format
#XML_OUTPUT_DIR = ./xml
XML_OUTPUT_DIR = $(OUTPUT)

# List of test cases for complex intrinsic metadata in JSON format
COMPLEX_JSON_FILES = $(addprefix $(JSON_OUTPUT_DIR)/, $(JSON_OUTPUT_FILES))

# List of the XML files used for testing conformance
TESTCASES = $(addprefix $(XML_OUTPUT_DIR)/, $(XML_OUTPUT_FILES))

# Write messages to the log file
LOGFILE = $(MAKEFILE:.mk=.log)


.PHONY: prepare clean


all: prepare testcases


# Load the dependencies between the input and output files
#-include intrinsic.deps


# Initialize the logfile and create any output directories that do not exist
prepare:
	@echo "make -f $(MAKEFILE) `date +\"%Y-%m-%d\"`" >$(LOGFILE)
	@for dir in $(JSON_OUTPUT_DIR) $(XML_OUTPUT_DIR); do if [ ! -d $${dir} ]; then echo "Creating directory: $${dir}"; mkdir -p $${dir}; fi; done


# Generate complex intrinsic metadata test cases in JSON format
intermediate: $(COMPLEX_JSON_FILES)
	@#echo "Finished generating complex metadata test cases (JSON format)"


# Generate each  metadata test case
testcases: $(COMPLEX_JSON_FILES) $(TESTCASES)
	@#echo "Finished generating complex metadata test cases (XML format)"


# Load the rules for creating complex metadata test cases in JSON format
-include complex-json.rules

# Load the files for creating complex metadata test cases in XML format
-include complex-xml.rules


verify:
	@#echo "Verifying $(basename $(MAKEFILE)) metadata test cases (XML format)" >$(LOGFILE)
	@echo "Verifying $(basename $(MAKEFILE)) metadata test cases in XML format"
	@for file in $(XML_OUTPUT_DIR)/*.xml; do $(VERIFY) $${file}; done


clean:
	@#echo "Removing JSON files"
	@rm -f $(COMPLEX_JSON_FILES)


clean-all: clean
	@#echo "Removing XML and JSON files"
	rm -f $(TESTCASES) $(LOGFILE)

