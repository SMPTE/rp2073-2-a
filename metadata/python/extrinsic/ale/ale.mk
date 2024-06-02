# Make file to generate test cases for Avid Log Exchange (ALE) metadata
#
# TODO: Is it necessary to create the intermediate JSON files?

# Filename of this make file
MAKEFILE = ale.mk

# Default output directory (can be overridden from the command line)
OUTPUT = ./xml

# Root directory for the VC-5 reference software
ROOT = ../../../..

# Python script for creating test cases
GENERATE = $(ROOT)/metadata/python/generate.py

# Python script for verifying the correctness of metadata test cases in XML format
VERIFY = $(ROOT)/metadata/python/verify.py

# List of extrinsic metadata files in ALE format
INPUT_FILES = aletest-seq1.ale

# Location for the extrinsic metadata files in ALE format
INPUT_DIR = $(ROOT)/metadata/media/ale

# Location for the test cases in XML format
OUTPUT_DIR = $(OUTPUT)

# List of test cases in XML format generated from the ALE input files
TESTCASES = $(addprefix $(OUTPUT_DIR)/, $(addsuffix .xml, $(basename $(INPUT_FILES))))

# Intermediate extrinsic metadata files in JSON format
JSON_DIR = ./json
JSON_FILES = $(addprefix $(JSON_DIR)/, $(addsuffix .json, $(basename $(INPUT_FILES))))

# Information about the original files used to create the metadata test cases
FILEINFO = ../fileinfo.json

# Write messages to the log file
LOGFILE = $(MAKEFILE:.mk=.log)


all: prepare testcases


.PHONY: prepare clean


# Initialize the logfile and create any output directories that do not exist
prepare:
	@echo "make -f $(MAKEFILE) `date +\"%Y-%m-%d\"`" >$(LOGFILE)
	@for dir in $(JSON_DIR) $(OUTPUT_DIR); do if [ ! -d $${dir} ]; then echo "Creating directory: $${dir}"; mkdir -p $${dir}; fi; done


# Generate each extrinsic metadata test case
testcases: $(JSON_FILES) $(TESTCASES)
	@#echo "Finished generating extrinsic metadata test cases"


# Rule for running the Python generator script to create each test case in XML format
$(OUTPUT_DIR)/%.xml: $(JSON_DIR)/%.json
	@echo "Output file: $@" >>$(LOGFILE)
	$(GENERATE) -E ALE -F $(FILEINFO) $< -o $@ >>$(LOGFILE)


# Rule for generating extrinsic metadata in JSON format from an ALE file
$(JSON_DIR)/%.json: $(INPUT_DIR)/%.ale
	$(GENERATE) -E ALE -F $(FILEINFO) $< -o $@ >>$(LOGFILE)


verify:
	@echo "Verifying $(basename $(MAKEFILE)) metadata test cases in XML format"
	@for file in $(OUTPUT_DIR)/*.xml; do $(VERIFY) $${file}; done


clean:
	@#echo "Removing JSON files"
	rm -f $(JSON_FILES)


clean-all: clean
	@#echo "Removing JSON and XML files"
	rm -f $(TESTCASES) $(LOGFILE)

