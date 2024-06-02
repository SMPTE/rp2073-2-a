# Make file to generate test cases for dark metadata

# Filename of this make file
MAKEFILE = dark.mk

# Default output directory (can be overridden from the command line)
OUTPUT = ./xml

# Root directory for the VC-5 reference software
ROOT = ../../..

# Tool for creating dark metadata test cases in JSON and XML formats
GENERATE = $(ROOT)/metadata/python/generate.py

# Tool for creating input files for dark metadata test cases
DARKEN = $(ROOT)/metadata/python/darken.py

# Python script for verifying the correctness of metadata test cases in XML format
VERIFY = $(ROOT)/metadata/python/verify.py

# List of dark metadata input files in JSON format
INPUT_FILES = dark-fourcc.json dark-uuid.json

# Location for the test cases in XML format
OUTPUT_DIR = $(OUTPUT)

# List of test cases in XML format generated from the dark metadata input files
TESTCASES = $(addprefix $(OUTPUT_DIR)/, $(addsuffix .xml, $(basename $(INPUT_FILES))))

# Intermediate dark metadata files in JSON format
JSON_DIR = ./json
JSON_FILES = $(addprefix $(JSON_DIR)/, $(addsuffix .json, $(basename $(INPUT_FILES))))

# Data files in Base64 format used to create the dark metadata test cases
DATA_DIR = ./b64
DATA_FILES = $(DATA_DIR)/dark-metadata.b64

# Identification code used for dark metadata test cases that do not use a FOURCC as the identifier
GUID = $(shell awk '/UUID/{print $$2}' fileinfo.json)

# Information about the contents of each file in base64 format
FILESIZE = $(DATA_DIR)/filesize.csv

# Write messages to the log file
LOGFILE = $(MAKEFILE:.mk=.log)


all: prepare testcases


.PHONY: prepare clean


# Initialize the logfile and create any output directories that do not exist
prepare:
	@echo "make -f $(MAKEFILE) `date +\"%Y-%m-%d\"`" >$(LOGFILE)
	@for dir in $(DATA_DIR) $(JSON_DIR) $(OUTPUT_DIR); do if [ ! -d $${dir} ]; then echo "Creating directory: $${dir}"; mkdir -p $${dir}; fi; done


# Generate each dark metadata test case
testcases: $(DATA_FILES) $(JSON_FILES) $(TESTCASES)
	@#echo "Finished generating dark metadata test cases"


# Rule for running the Python generator script to create each test case in XML format
$(OUTPUT_DIR)/%.xml: $(JSON_DIR)/%.json
	@echo "Output file: $@" >>$(LOGFILE)
	$(GENERATE) -D $< -X $(FILESIZE) -o $@ >>$(LOGFILE)


# Load the rules for creating intermediate JSON files for the dark metadata test cases
-include dark.rules


# Rule for creating data files in Base64 format
$(DATA_DIR)/%.b64: $(DARKEN)
	$(DARKEN) -s "vendor:xyz,camera:sportscam,id:123456789" -f $(FILESIZE) -o $@


verify:
	@echo "Verifying $(basename $(MAKEFILE)) metadata test cases in XML format"
	@for file in $(OUTPUT_DIR)/*.xml; do $(VERIFY) $${file}; done


clean:
	@#echo "Removing intermediate JSON files and data files in Base64 format"
	rm -f $(JSON_FILES) $(DATA_FILES)
	rm -f $(FILESIZE)


clean-all: clean
	@#echo "Removing XML test cases and all input files (JSON and Base64)"
	rm -f $(TESTCASES) $(LOGFILE)

