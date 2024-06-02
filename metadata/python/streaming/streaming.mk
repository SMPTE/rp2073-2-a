# Make file to generate test cases for streaming metadata

# Filename of this make file
MAKEFILE = streaming.mk

# Default output directory (can be overridden from the command line)
OUTPUT = ./xml

# Root directory for the VC-5 reference software
ROOT = ../../..

# Python script for creating test cases
GENERATE = $(ROOT)/metadata/python/generate.py

# Python script for verifying the correctness of metadata test cases in XML format
VERIFY = $(ROOT)/metadata/python/verify.py

# List of streaming metadata files in JSON format
#INPUT_FILES = hero5.json hero6.json hero7.json
#INPUT_FILES = hero5.json hero6+ble.json hero6.json hero7.json hero8.json karma.json
#INPUT_FILES = Fusion.json hero5.json hero6+ble.json hero6.json hero7.json hero8.json karma.json max-360mode.json max-heromode.json
INPUT_FILES = hero5.json hero6.json hero7.json hero8.json hero6+ble.json karma.json Fusion.json max-360mode.json max-heromode.json

# Location for the streaming metadata files in JSON format
INPUT_DIR = ./json

# Location for the streaming metadata test cases in XML format
#OUTPUT_DIR = ../testcases/streaming
#OUTPUT_DIR =  ./xml
OUTPUT_DIR =  $(OUTPUT)

# List of test cases in XML format generated from the JSON input files
TESTCASES = $(addprefix $(OUTPUT_DIR)/, $(addsuffix .xml, $(basename $(INPUT_FILES))))

# Intermediate streaming metadata files in JSON format
JSON_FILES = $(addprefix $(INPUT_DIR)/, $(INPUT_FILES))

# Write messages to the log file
LOGFILE = streaming.log

# Location of the streaming metadata files in CSV format obtained from the GPMF sample files
GPMF_DIR = ../GPMF


.PHONY: prepare clean


all: prepare testcases


# Initialize the logfile and create any output directories that do not exist
prepare:
	@echo "make -f $(MAKEFILE) `date +\"%Y-%m-%d\"`" >$(LOGFILE)
	@if [ ! -d $(INPUT_DIR) ]; then echo "Creating directory: $(INPUT_DIR)"; mkdir -p $(INPUT_DIR); fi
	@if [ ! -d $(OUTPUT_DIR) ]; then echo "Creating directory: $(OUTPUT_DIR)"; mkdir -p $(OUTPUT_DIR); fi


# Generate each streaming metadata test case
testcases: $(JSON_FILES) $(TESTCASES)
	@#echo "Finished generating streaming metadata test cases"


# Rule for running the Python generator script to create each test case in XML format
$(OUTPUT_DIR)/%.xml: $(INPUT_DIR)/%.json
	@echo "Output file: $@" >>$(LOGFILE)
	$(GENERATE) -g $< -o $@ >>$(LOGFILE)


# Rule for generating streaming metadata in JSON format from the CSV files output by the GPMF parser
$(INPUT_DIR)/%.json: $(GPMF_DIR)/%.csv
	@echo "Output file: $@" >>$(LOGFILE)
	$(GENERATE) -C -g $< -o $@ >>$(LOGFILE)


verify:
	@#echo "Verifying $(basename $(MAKEFILE)) metadata test cases (XML format)" >$(LOGFILE)
	@echo "Verifying $(basename $(MAKEFILE)) metadata test cases in XML format"
	@for file in $(OUTPUT_DIR)/*.xml; do $(VERIFY) --category streaming $${file}; done


clean:
	rm -f $(JSON_FILES)


clean-all: clean
	rm -f $(TESTCASES) $(LOGFILE)

