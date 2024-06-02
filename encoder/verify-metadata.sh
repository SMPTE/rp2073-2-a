#!/usr/bin/env bash
#
# Script for verifying the embedded metadata in the encoded bitstreams

# Root directory of the metadata test cases created by the generator script
root=../metadata/python
#root=../media/metadata/input

# Default metadata test case in XML format
metadata=${root}/intrinsic/simple/xml/simple-c01.xml
#metadata=${root}/simple-c01.xml

# Default input image file
#imagefile=../media/boxes/1280x720/rg48/boxes-1280x720-0000.rg48
#imagefile=../media/boxes/master/part1/release/boxes-1280x720-0000-part1-1280x720-byr4-vc5.byr4

# Tool for extracting chunk elements from an encoded bitstream
#dumper=../scripts/dumper.py

# Location of the bitstreams created by the sample encoder
#outdir=./test
outdir=./metadata/bitstreams

# Location of the binary data extracted from the bitstreams
#bindir=../metadata/parser/binary
bindir=./metadata/encoded

# Root directory for the reference files created by the XML parser
refdir=../metadata/parser/binary

# Define the usage message
#usage() { echo "Usage: $0 [-q] [-v] [-z] [-h] [-i <imagefile>] [-o <bitstream>] [-d <outdir>] [-b <bindir>] metadata" 1>&2; exit 1; }
usage() { echo "Usage: $0 [-q] [-v] [-z] [-h] [-d <outdir>] [-b <bindir>] metadata" 1>&2; exit 1; }

# Print the usage message if no command-line arguments
[ $# -eq 0 ] && usage

# Parse the command-line options
#while getopts ":hi:o:m:d:b:r:vzq" arg; do
while getopts ":hm:d:b:r:vzq" arg; do

	case $arg in

# 	i) # Specify the input image file
# 		imagefile=${OPTARG}
# 		;;
#
# 	o) # Specify the output bitstream file
# 		bitstream=${OPTARG}
# 		;;

	m) # Root directory of the metadata test cases
		root=${OPTARG}
		;;

	d) # Specify the output directory
		outdir=${OPTARG}
		;;

	b) # Location of the binary files output by the encoder
		bindir=${OPTARG}
		;;

	r) # Root directory of the binary files output by the XML parser
		refdir=${OPTARG}
		;;

	v) # Enable verbose output
		verbose=yes
		;;

	z) # Enable extra output for debugging
		debug=yes
		;;

	q) # Suppress all output
		quiet=yes
		;;

	h) # Display the usage message
		usage
		exit 0
		;;

	esac

done

shift $((OPTIND-1))
metadata=${1:-${metadata}}
#echo "$metadata"

# Compute the pathname to the binary file extracted from the bitstream by the encoder
encoded=$(echo ${bindir}/$(basename $metadata) | sed s/xml/bin/g)
#echo "$encoded"

# Compute the pathname to the reference file for comparison with the binary file
binary=$(echo ${refdir}/${metadata##$root/} | sed s/xml/bin/g)
#echo "$binary"

if diff -q -b $encoded $binary &>/dev/null; then
	[ -z "$quiet" ] && echo "Passed: $metadata"
	exit 0
else
	[ -z "$quiet" ] && echo "Failed: $metadata"
	exit 1
fi

