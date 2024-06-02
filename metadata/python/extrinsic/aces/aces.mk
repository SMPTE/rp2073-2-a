# Make file to generate test cases for extrinsic ACES metadata
#
# TODO: Is it necessary to create the intermediate JSON files?

# Filename of this make file
MAKEFILE = aces.mk

# Default output directory (can be overridden from the command line)
OUTPUT = ./xml

# Root directory for the VC-5 reference software
ROOT = ../../../..

# Python script for creating test cases
GENERATE = $(ROOT)/metadata/python/generate.py

# Program for extracting metadata from image files in OpenEXR format
EXRDUMP = $(ROOT)/metadata/tools/bin/exrdump

# Program for converting binary files to base64
BINTOOL = $(ROOT)/metadata/tools/bin/bintool

# Python script for verifying the correctness of metadata test cases in XML format
VERIFY = $(ROOT)/metadata/python/verify.py

# Location for the extrinsic metadata files in EXR format
INPUT_DIR = $(ROOT)/metadata/media/aces

# List of extrinsic metadata files in EXR format
INPUT_FILES = $(notdir $(shell ls -1 ${INPUT_DIR}/*.exr))

# Location for the test cases in XML format
OUTPUT_DIR = $(OUTPUT)

# List of test cases in XML format generated from the ACES input files
TESTCASES = $(addprefix $(OUTPUT_DIR)/, $(addsuffix .xml, $(basename $(INPUT_FILES))))

# Intermediate extrinsic metadata files in JSON format
JSON_DIR = ./json
JSON_FILES = $(addprefix $(JSON_DIR)/, $(addsuffix .json, $(basename $(INPUT_FILES))))

# Intermediate extrinsic metadata files in Base64 format
BASE64_DIR = ./b64
BASE64_FILES = $(addprefix $(BASE64_DIR)/, $(addsuffix .b64, $(basename $(INPUT_FILES))))

# Intermediate directory for the binary data extracted by the EXR dump tool
BIN_DIR = ./bin
BIN_FILES = $(addprefix $(BIN_DIR)/, $(addsuffix .bin, $(basename $(INPUT_FILES))))

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
	@for dir in $(BIN_DIR) $(BASE64_DIR) $(JSON_DIR) $(OUTPUT_DIR); do if [ ! -d $${dir} ]; then echo "Creating directory: $${dir}"; mkdir -p $${dir}; fi; done


# Generate each extrinsic metadata test case
testcases: $(BIN_FILES) $(BASE64_FILES) $(JSON_FILES) $(TESTCASES)
	@#echo "Finished generating extrinsic metadata test cases"


# Rule for running the Python generator script to create each test case in XML format
$(OUTPUT_DIR)/%.xml: $(JSON_DIR)/%.json
	@echo "Output file: $@" >>$(LOGFILE)
	$(GENERATE) -E ACES -F $(FILEINFO) -X $(FILESIZE) $< -o $@ >>$(LOGFILE)


# Rule for generating extrinsic metadata in JSON format from a base64 file
$(JSON_DIR)/%.json: $(BASE64_DIR)/%.b64
	$(GENERATE) -E ACES -F $(FILEINFO) -X $(FILESIZE) $< -o $@ >>$(LOGFILE)


# Rule for generating extrinsic data in base64 format from a binary file
$(BASE64_DIR)/%.b64: $(BIN_DIR)/%.bin
	$(BINTOOL) $< -d $(BASE64_DIR)


# Rule for extracting ACES attributes from EXR files
$(BIN_DIR)/%.bin: $(INPUT_DIR)/%.exr
	$(EXRDUMP) -d $(BIN_DIR) $<


verify:
	@echo "Verifying $(basename $(MAKEFILE)) metadata test cases in XML format"
	@for file in $(OUTPUT_DIR)/*.xml; do $(VERIFY) $${file}; done


clean:
	@#echo "Removing intermediate files"
	rm -f $(BASE64_FILES) $(JSON_FILES) $(FILESIZE)


clean-all: clean
	@#echo "Removing intermediate and XML files"
	rm -f $(TESTCASES) $(LOGFILE)

