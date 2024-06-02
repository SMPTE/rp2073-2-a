# Make file for collecting information about the original files used to create extrinsic metadata test cases

# Filename of this make file
MAKEFILE = fileinfo.mk

# Default output file containing file information (can be overridden from the command line)
OUTPUT = ./fileinfo.json

# Root directory for the VC-5 reference software
ROOT = ../../..

# Tool used to collect information about the original files used to create extrinsic metadata test cases
FILEINFO = $(ROOT)/metadata/python/fileinfo.py

# Original files used to create XMP metadata test cases
XMP_URL = https://github.com/adobe/XMP-Toolkit-SDK/tree/master/samples/testfiles
XMP_DIR = $(ROOT)/../../../XMP/XMP-Toolkit-SDK/samples/testfiles
XMP_EXT = jpg
XMP_FILES = $(shell ls -1 $(XMP_DIR)/*.$(XMP_EXT))

# Original files used to create DPX metadata test cases
DPX_URL = https://github.com/SMPTE/smpte-31fs-hdrdpx/tree/master/examples
DPX_DIR = $(ROOT)/metadata/media/dpx
DPX_EXT = dpx
DPX_FILES = $(shell ls -1 $(DPX_DIR)/*.$(DPX_EXT))

# Original files used to create MXF metadata test cases
MXF_URL = https://github.com/SMPTE/smpte-10e-vc5/tree/master/metadata/media/mxf
MXF_DIR = $(ROOT)/metadata/media/mxf
MXF_EXT = mxf
MXF_FILES = $(shell ls -1 $(MXF_DIR)/*.$(MXF_EXT))

# Original files used to create ACES metadata test cases
ACES_URL = https://www.dropbox.com/sh/9xcfbespknayuft/AACYLWs5QGYGTym07gtGYaOLa/ACES
ACES_DIR = $(ROOT)/metadata/media/aces
ACES_EXT = exr
ACES_FILES = $(shell ls -1 $(ACES_DIR)/*.$(ACES_EXT))

# Original files used to create DMCVT metadata test cases
DMCVT_URL = https://github.com/SMPTE/smpte-10e-vc5/tree/master/metadata/media/dmcvt
DMCVT_DIR = $(ROOT)/metadata/media/dmcvt
DMCVT_EXT = mxf
DMCVT_FILES = $(shell ls -1 $(DMCVT_DIR)/*.$(DMCVT_EXT))

# Original files used to create ALE metadata test cases
ALE_URL = https://github.com/SMPTE/smpte-10e-vc5/tree/master/metadata/media/ale
ALE_DIR = $(ROOT)/metadata/media/ale
ALE_EXT = ale
ALE_FILES = $(shell ls -1 $(ALE_DIR)/*.$(ALE_EXT))

# List of original files for all extrinsic metadata classes
#INPUT_FILES = $(XMP_FILES) $(DPX_FILES)

# Write messages to the log file
LOGFILE = $(MAKEFILE:.mk=.log)


all: $(OUTPUT)


.PHONY: $(OUTPUT) clean


$(OUTPUT):
	@$(FILEINFO) -v $(XMP_FILES) --location $(XMP_URL) --output $(OUTPUT)
	@$(FILEINFO) -v $(DPX_FILES) --location $(DPX_URL) --output $(OUTPUT)
	@$(FILEINFO) -v $(MXF_FILES) --location $(MXF_URL) --output $(OUTPUT)
	@$(FILEINFO) -v $(ACES_FILES) --location $(ACES_URL) --output $(OUTPUT)
	@$(FILEINFO) -v $(DMCVT_FILES) --location $(DMCVT_URL) --output $(OUTPUT)
	@$(FILEINFO) -v $(ALE_FILES) --location $(ALE_URL) --output $(OUTPUT)


clean:
	@#echo "Removing file information: $(OUTPUT)"
	rm -f $(OUTPUT)

