# Make file to generate test cases for extrinsic DPX metadata
#
# TODO: Is it necessary to create the intermediate JSON files?

# Filename of this make file
MAKEFILE = dpx.mk

# Default output directory (can be overridden from the command line)
#OUTPUT = ./dpx
OUTPUT = ./xml

# Root directory for the VC-5 reference software
ROOT = ../../../..

# Python script for creating test cases
GENERATE = $(ROOT)/metadata/python/generate.py

# Python script for verifying the correctness of metadata test cases in XML format
VERIFY = $(ROOT)/metadata/python/verify.py

# Program for extracting the header from DPX files
DPXDUMP = $(ROOT)/metadata/tools/bin/dpxdump

# Location for the extrinsic metadata files in DPX format
INPUT_DIR = $(ROOT)/metadata/media/dpx

# List of extrinsic metadata files in DPX format
INPUT_FILES = $(notdir $(shell ls -1 ${INPUT_DIR}/*.dpx))

# Location for the test cases in XML format
OUTPUT_DIR = $(OUTPUT)

# List of test cases in XML format generated from the DPX input files
TESTCASES = $(addprefix $(OUTPUT_DIR)/, $(addsuffix .xml, $(basename $(INPUT_FILES))))

# Intermediate extrinsic metadata files in JSON format
JSON_DIR = ./json
JSON_FILES = $(addprefix $(JSON_DIR)/, $(addsuffix .json, $(basename $(INPUT_FILES))))

# Intermediate extrinsic metadata files in base64 format
BASE64_DIR = ./b64
BASE64_FILES = $(addprefix $(BASE64_DIR)/, $(addsuffix .b64, $(basename $(INPUT_FILES))))

# Information about the original files used to create the metadata test cases
FILEINFO = ../fileinfo.json

# Information about the contents of each file in base64 format
FILESIZE = $(BASE64_DIR)/filesize.csv

# Write messages to the log file
LOGFILE = $(MAKEFILE:.mk=.log)


all: prepare testcases


.PHONY: prepare clean


# Initialize the logfile and create any output directories that do not exist
prepare:
	@echo "make -f $(MAKEFILE) `date +\"%Y-%m-%d\"`" >$(LOGFILE)
	@for dir in $(BASE64_DIR) $(JSON_DIR) $(OUTPUT_DIR); do if [ ! -d $${dir} ]; then echo "Creating directory: $${dir}"; mkdir -p $${dir}; fi; done


# Generate each extrinsic metadata test case
testcases: $(BASE64_FILES) $(JSON_FILES) $(TESTCASES)
	@#echo "Finished generating extrinsic metadata test cases"


# Rule for running the Python generator script to create each test case in XML format
$(OUTPUT_DIR)/%.xml: $(JSON_DIR)/%.json
	@echo "Output file: $@" >>$(LOGFILE)
	$(GENERATE) -E DPX -F $(FILEINFO) -X $(FILESIZE) $< -o $@ >>$(LOGFILE)


# Rule for generating extrinsic metadata in JSON format from a base64 file
$(JSON_DIR)/%.json: $(BASE64_DIR)/%.b64
	$(GENERATE) -E DPX -F $(FILEINFO) -X $(FILESIZE) $< -o $@ >>$(LOGFILE)


# Rule for generating extrinsic data in base64 format from a DPX file
$(BASE64_DIR)/%.b64: $(INPUT_DIR)/%.dpx
	$(DPXDUMP) $< -d $(BASE64_DIR)


verify:
	@echo "Verifying $(basename $(MAKEFILE)) metadata test cases in XML format"
	@for file in $(OUTPUT_DIR)/*.xml; do $(VERIFY) $${file}; done


clean:
	@#echo "Removing intermediate files"
	@rm -f $(BASE64_FILES) $(JSON_FILES) $(FILESIZE)


clean-all: clean
	@#echo "Removing intermediate and XML files"
	@rm -f $(TESTCASES) $(LOGFILE)

