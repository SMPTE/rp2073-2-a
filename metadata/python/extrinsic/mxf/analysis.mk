# Make file to generate test cases for extrinsic MXF metadata

# Filename of this make file
MAKEFILE = analysis.mk

# Default output directory (can be overridden from the command line)
OUTPUT = ./analysis

# Root directory for the VC-5 reference software
ROOT = ../../../..

# MXF tools and default dictionary
MXFDUMP = $(HOME)/Projects/MXF/mxflib/bin/mxfdump
MXF2DOT = $(HOME)/Projects/MXF/mxflib/bin/mxf2dot
MXFDICT = $(HOME)/Projects/MXF/mxflib/dict.xml

# Graphviz program for transforming dot files into SVG diagrams
DOT = dot

# Location for the extrinsic metadata files in MXF format
INPUT_DIR = $(ROOT)/metadata/media/mxf

# List of extrinsic metadata files in MXF format
INPUT_FILES = $(notdir $(shell ls -1 ${INPUT_DIR}/*.mxf))

# Location for the test cases in XML format
OUTPUT_DIR = $(OUTPUT)

# Output files (text dump and diagrams in SVG format)
TXT_FILES = $(addprefix $(OUTPUT_DIR)/, $(addsuffix .txt, $(basename $(INPUT_FILES))))
SVG_FILES = $(addprefix $(OUTPUT_DIR)/, $(addsuffix .svg, $(basename $(INPUT_FILES))))
OUTPUT_FILES = $(TXT_FILES) $(SVG_FILES)


all: $(OUTPUT_FILES)


$(OUTPUT_DIR)/%.txt: $(INPUT_DIR)/%.mxf
	$(MXFDUMP) --m $(MXFDICT) $< > $@


$(OUTPUT_DIR)/%.dot: $(INPUT_DIR)/%.mxf
	$(MXF2DOT) -m $(MXFDICT) $< $@


$(OUTPUT_DIR)/%.svg: $(OUTPUT_DIR)/%.dot
	$(DOT) -Tsvg $< -o $@

