#!/usr/bin/env bash
#
# Script for testing the code for generating multiclass metadata test cases

# Merge the test cases in JSON format into a single JSON file
../generate.py -M \
	../intrinsic/complex/json/simple-LOGA-stereo.json \
	../extrinsic/xmp/json/BlueSquare.json \
	../extrinsic/mxf/json/part15-j2c.json \
	../extrinsic/aces/json/syntheticChart.01.json \
	-o test-multiclass.json

# Merge the test cases in JSON format into a single XML file
../generate.py -M \
	../intrinsic/complex/json/simple-LOGA-stereo.json \
	../extrinsic/xmp/json/BlueSquare.json \
	../extrinsic/mxf/json/part15-j2c.json \
	../extrinsic/aces/json/syntheticChart.01.json \
	-o test-multiclass.xml
