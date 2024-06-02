#!/usr/bin/env bash
#
# Script for testing embedded metadata in the encoded bitstreams
#
# TODO: Compute the input image width and height from the pathname

# Root directory of the metadata test cases
root=../metadata/python
#root=../media/metadata/input

# Default metadata test case in XML format
metadata=${root}/intrinsic/simple/xml/simple-c01.xml
#metadata=${root}/simple-c01.xml

# Default input image file
imagefile=../media/boxes/1280x720/rg48/boxes-1280x720-0000.rg48
#imagefile=../media/boxes/master/part1/release/boxes-1280x720-0000-part1-1280x720-byr4-vc5.byr4

# Tool for extracting chunk elements from an encoded bitstream
dumper=../scripts/dumper.py

# Location for the output bitstream files
outdir=./test
#outdir=./metadata/bitstreams

# Directory containing the binary files created by the XML parser
bindir=../metadata/parser/binary

# Define the usage message
usage() { echo "Usage: $0 [-q] [-v] [-z] [-h] [-i <imagefile>] [-o <bitstream>] [-d <outdir>] [-b <bindir>] metadata" 1>&2; exit 1; }

# Print the usage message if no command-line arguments
[ $# -eq 0 ] && usage

# Parse the command-line options
while getopts ":hi:o:d:b:m:vzq" arg; do

	case $arg in

	i) # Specify the input image file
		imagefile=${OPTARG}
		;;

	o) # Specify the output bitstream file
		bitstream=${OPTARG}
		;;

	d) # Specify the output directory
		outdir=${OPTARG}
		;;

	b) # Location of the binary files output by the XML parser
		bindir=${OPTARG}
		;;

	m) # Root directory of the metadata test cases
		root==${OPTARG}
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

# Was the output bitstream file specified?
if [ -z "$bitstream" ]; then
	# Derive the output filename from the metadata pathname
	bitstream=${outdir}/$(basename ${metadata} .xml).vc5
fi

if [ -z "$quiet" ]; then
	if [ -n "$verbose" ]; then
		# The sample encoder will print the input and output files if the verbose flag is set
		flags="-v"
	else
		# Print the input and output files if the encoder is not passed the verbose flag
		echo "Input image: ${imagefile}"
		echo "Output file: ${bitstream}"
		#echo "Inject data: ${metadata}"
	fi

	if [ -n "$debug" ]; then
		# Enable extra output for debugging
		[ -z "$flags" ] && flags="-z" || flags="$flags -z"
	fi
fi

#echo "$flags"

# Encode the image file with the specified metadata embedded in the bitstream
#./build/linux/debug/encoder ${flags} -P 7 -w 1280 -h 720 -M ${metadata} ${imagefile} ${bitstream}
./build/linux/debug/encoder $flags -P 7 -w 1280 -h 720 -M $metadata $imagefile $bitstream

# Extract the metadata chunk from the bitstream into a binary file
binary=${outdir}/$(basename ${bitstream} .vc5).bin
$dumper --chunk metadata $bitstream --output $binary

# Compute the pathname of the reference file for comparison with the binary file created by the dumper
#dir=$(echo $metadata | sed s/xml/bin/g)
#echo "$dir"

#echo "$binary"
#echo "${bindir}/$(basename ${metadata} .xml).bin"

# Compute the pathname to the binary file created by the XML parser
reference=$(echo ${bindir}/${metadata##$root/} | sed s/xml/bin/g)
#echo $reference

if diff -q -b $binary $reference &>/dev/null; then
	[ -z "$quiet" ] && echo "Passed case: $metadata"
	exit 0
else
	[ -z "$quiet" ] && echo "Failed case: $metadata"
	exit 1
fi

