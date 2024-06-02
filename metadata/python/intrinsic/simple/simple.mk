# Make file to generate test cases for simple intrinsic metadata

# Filename of this make file
MAKEFILE = simple.mk

# Default output directory (can be overridden from the command line)
OUTPUT = ./xml

# Root directory for the VC-5 reference software
ROOT = ../../../..

# Python script for creating test cases
GENERATE = $(ROOT)/metadata/python/generate.py

# Random number seed (provides repeatability across runs)
SEED = 47

# Python script for verifying the correctness of metadata test cases in XML format
VERIFY = $(ROOT)/metadata/python/verify.py

# Files used to create the intrinsic metadata test cases
INPUT_FILES = simple.json

# Location for the simple metadata files in JSON format
INPUT_DIR = ./json

# List of intermediate files in JSON format
SIMPLE_JSON_FILES = $(addprefix $(INPUT_DIR)/, $(INPUT_FILES))

# List of simple metadata test cases
OUTPUT_FILES = $(shell cat simple.targets)

# Location for the test cases in XML format
#OUTPUT_DIR = ./xml
#OUTPUT_DIR = ../../testcases/intrinsic/simple/
OUTPUT_DIR = $(OUTPUT)

# List of test cases in XML format generated from the XMP input files
TESTCASES = $(addprefix $(OUTPUT_DIR)/, $(OUTPUT_FILES))

# Write messages to the log file
#LOGFILE = intrinsic.log
LOGFILE = $(MAKEFILE:.mk=.log)


.PHONY: prepare clean


all: prepare testcases


# Initialize the logfile and create any output directories that do not exist
prepare:
	@echo "make -f $(MAKEFILE) `date +\"%Y-%m-%d\"`" >$(LOGFILE)
	@for dir in $(INPUT_DIR) $(OUTPUT_DIR); do if [ ! -d $${dir} ]; then echo "Creating directory: $${dir}"; mkdir -p $${dir}; fi; done


# Generate each  metadata test case
testcases: $(TESTCASES)
	@#echo "Finished generating extrinsic metadata test cases"


# Load the rules for creating simple metadata test cases from the JSON input files
-include simple.rules


verify:
	@#echo "Verifying $(basename $(MAKEFILE)) metadata test cases (XML format)" >$(LOGFILE)
	@echo "Verifying $(basename $(MAKEFILE)) metadata test cases in XML format"
	@for file in $(OUTPUT_DIR)/*.xml; do $(VERIFY) $${file}; done


clean:
	@#echo "Removing JSON files"
	rm -f $(SIMPLE_JSON_FILES)


clean-all: clean
	@#echo "Removing XML and JSON files"
	rm -f $(TESTCASES) $(LOGFILE)

