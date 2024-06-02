# Make file to generate test cases with instances of more than one metadata class

# Filename of this make file
MAKEFILE = multiclass.mk

# Default output directory (can be overridden from the command line)
OUTPUT = ./xml

# Root directory for the VC-5 reference software
ROOT = ../../..

# Python script for creating test cases
GENERATE = $(ROOT)/metadata/python/generate.py

# Random number seed (provides repeatability across runs)
#SEED = 47

# Python script for verifying the correctness of metadata test cases in XML format
VERIFY = $(ROOT)/metadata/python/verify.py

# List of multiclass metadata test cases in JSON format
JSON_FILES = multiclass-01.json multiclass-02.json

# Location for the test cases in JSON format
JSON_DIR = ./json

# List of multiclass metadata test cases in XML format
OUTPUT_FILES = multiclass-01.xml multiclass-02.xml

# Location for the test cases in XML format
OUTPUT_DIR = $(OUTPUT)

# List of test cases in both JSON and XML formats
TESTCASES = $(addprefix $(JSON_DIR)/, $(JSON_FILES)) $(addprefix $(OUTPUT_DIR)/, $(OUTPUT_FILES))

# Write messages to the log file
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


$(OUTPUT_DIR)/multiclass-01.xml: $(shell cat multiclass-01.list)
	$(GENERATE) -M $^ -o $@ >>$(LOGFILE)


$(OUTPUT_DIR)/multiclass-02.xml: $(shell cat multiclass-02.list)
	$(GENERATE) -M $^ -o $@ >>$(LOGFILE)


# NOTE: The multiclass test cases in JSON format are not required for generating the test cases in XML format

$(JSON_DIR)/multiclass-01.json: $(shell cat multiclass-01.list)
	$(GENERATE) -M $^ -o $@ >>$(LOGFILE)


$(JSON_DIR)/multiclass-02.json: $(shell cat multiclass-02.list)
	$(GENERATE) -M $^ -o $@ >>$(LOGFILE)


verify:
	@#echo "Verifying $(basename $(MAKEFILE)) metadata test cases (XML format)" >$(LOGFILE)
	@echo "Verifying $(basename $(MAKEFILE)) metadata test cases in XML format"
	@for file in $(OUTPUT_DIR)/*.xml; do $(VERIFY) $${file}; done


clean:
	@echo "Removing JSON files"
	@rm -f $(INPUT_DIR)/*.json


clean-all: clean
	@echo "Removing XML and JSON files"
	@rm -f $(TESTCASES) $(LOGFILE)

