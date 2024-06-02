#!/usr/bin/env bash
#
# Script for testing metadata decoded from a bitstreams

# Root directory of the metadata created by the decoder
root=./metadata

# Directory for the metadata extracted from the bitstream by the decoder
#decoded=${root}/decoded

# Directory for the images decoded from the bitstream by the decoder
#images=${root}/images

# Root of the directory tree of metadata files output by the XML dumper
outdir=../metadata/dumper/output

# Define the usage message
usage() { echo "Usage: $0 [-q] [-v] [-z] [-h] [-d <outdir>] [-b <bindir>] bitstream" 1>&2; exit 1; }

# Print the usage message if no command-line arguments
[ $# -eq 0 ] && usage

# Parse the command-line options
while getopts ":hr:d:vzq" arg; do

	case $arg in

	r) # Root directory for the metadata created by the decoder
		bindir=${OPTARG}
		;;

	d) # Root directory for the output of the XML dumper
		outdir=${OPTARG}
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
reference=${1:-${bitstream}}
#echo "$reference"

# Compute the pathname to the metadata output by the decoder
filename=${reference##*/}
decoded=${root}/decoded/${filename%%.*}.xml
#echo "$decoded"

if [ ! -f "$decoded" ]; then
	echo "Missing metadata test case: $decoded"
else
	# Compare the metadata file output by the decoder with the reference
	if diff -q $decoded $reference &>/dev/null; then
		[ -z "$quiet" ] && echo "Passed: $decoded"
		exit 0
	else
		[ -z "$quiet" ] && echo "Failed: $decoded"
		exit 1
	fi
fi

