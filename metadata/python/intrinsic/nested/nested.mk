# Make file to create intrinsic metadata test cases for nested tuples

# Filename of this make file
MAKEFILE = nested.mk

# Default output directory (can be overridden from the command line)
OUTPUT = ./xml

# Root directory for the VC-5 reference software
ROOT = ../../../..

# Python script for creating test cases
GENERATE = $(ROOT)/metadata/python/generate.py

# Python script for verifying the correctness of metadata test cases in XML format
VERIFY = $(ROOT)/metadata/python/verify.py

# Location for the nested intrinsic metadata files in JSON format
INPUT_DIR = ./json

# Location for the nested intrinsic metadata files in XML format
#OUTPUT_DIR = ./xml
OUTPUT_DIR = $(OUTPUT)

# Test cases for encoding curve metadata
ENCODING = encoding-LOGA.xml encoding-GAMA.xml encoding-LINR.xml encoding-FSLG.xml encoding-LOGC.xml encoding-PQEC.xml encoding-HLGE.xml

# Test cases for layer metadata
LAYERS = layers-stereo.xml layers-hdr.xml layers-interlaced.xml

# Test cases for conformance testing
#TESTCASES = $(addprefix $(OUTPUT_DIR)/, $(ENCODING))
TESTCASES = $(addprefix $(OUTPUT_DIR)/, $(ENCODING) $(LAYERS))

# Intermediate encoding curve metadata files in JSON format
ENCODING_JSON_FILES = $(addprefix $(INPUT_DIR)/, $(addsuffix .json, $(basename $(ENCODING))))

# Intermediate layers metadata files in JSON format
LAYERS_JSON_FILES = $(addprefix $(INPUT_DIR)/, $(addsuffix .json, $(basename $(LAYERS))))

# list of all JSON files created by this make file
JSON_FILES = $(INPUT_DIR)/encoding.json $(ENCODING_JSON_FILES) $(LAYERS_JSON_FILES)

# Write messages to the log file
LOGFILE = $(MAKEFILE:.mk=.log)


.PHONY: prepare clean


all: prepare testcases


prepare:
	@echo "make -f $(MAKEFILE) `date +\"%Y-%m-%d\"`" >$(LOGFILE)
	@for dir in $(INPUT_DIR) $(OUTPUT_DIR); do if [ ! -d $${dir} ]; then echo "Creating directory: $${dir}"; mkdir -p $${dir}; fi; done


# Generate each nested metadata test case
testcases: $(ENCODING_JSON_FILES) $(LAYERS_JSON_FILES) $(TESTCASES)
	@#echo "Finished generating nested metadata test cases"


$(OUTPUT_DIR)/%.xml: $(INPUT_DIR)/%.json
	@echo "Output file: $@" >>$(LOGFILE)
	$(GENERATE) $< -o $@ >>$(LOGFILE)


$(ENCODING_JSON_FILES): $(INPUT_DIR)/encoding.json
	$(GENERATE) -s $< -f $(INPUT_DIR)/encoding-%s.json


$(INPUT_DIR)/encoding.json: ./encoding.csv
	$(GENERATE) -C -e $< -o $@


$(INPUT_DIR)/layers-%.json: ./layers-%.csv
	$(GENERATE) -C -l $< -o $@


verify:
	@#echo "Verifying $(basename $(MAKEFILE)) metadata test cases (XML format)" >$(LOGFILE)
	@echo "Verifying $(basename $(MAKEFILE)) metadata test cases in XML format"
	@for file in $(OUTPUT_DIR)/*.xml; do $(VERIFY) $${file}; done


clean:
	@#echo "Removing JSON files"
	rm -f $(JSON_FILES)


clean-all: clean
	@#echo "Removing JSON and XML files"
	rm -f $(TESTCASES) $(LOGFILE)

