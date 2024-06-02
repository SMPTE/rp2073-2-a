#!/usr/bin/env bash
#
# Script for generating the encoder metadata test cases
#
# TODO: Compute the input image width and height from the pathname

# Default location of the metadata test cases
#root=../metadata/python
root=../media/metadata/input

# Default metadata test case in XML format
#metadata=${root}/intrinsic/simple/xml/simple-c01.xml

# Default input image file
#imagefile=../media/boxes/master/part1/release/boxes-1280x720-0000-part1-1280x720-byr4-vc5.byr4
imagefile=../media/boxes/1280x720/rg48/boxes-1280x720-0000.rg48

# Tool for extracting chunk elements from an encoded bitstream
dumper=../scripts/dumper.py

# Location for the output bitstream files
#outdir=./test
outdir=./metadata/bitstreams

# Location for the output binary files
bindir=./metadata/encoded

# Default output bitstream file
#bitstream=${outdir}/$(basename ${metadata} .xml).vc5

# Define the usage message
usage() { echo "Usage: $0 [-q] [-v] [-z] [-h] [-i <imagefile>] [-o <bitstream>] [-m metadata] [-d <outdir>] [-r <root>]" 1>&2; exit 1; }

# Print the usage message if no command-line arguments
[ $# -eq 0 ] && usage

# Parse the command-line options
while getopts ":hi:o:m:d:r:fvzq" arg; do

	case $arg in

	i) # Specify the input image file
		imagefile=${OPTARG}
		;;

	o) # Specify the output bitstream file
		bitstream=${OPTARG}
		;;

	m) # Metadata test case
		metadata=${OPTARG}
		;;

	d) # Specify the output directory
		outdir=${OPTARG}
		;;

	r) # Root directory of the metadata test cases
		root=${OPTARG}
		;;

	f) # Force remaking the test case
		force=yes
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

# Was the output bitstream file specified?
if [ -z "$bitstream" ]; then
	if [ -z "$metadata" ]; then
		# Derive the output filename from the input image
		#bitstream=${outdir}/$(basename ${imagefile} .xml).vc5
		basename=$(basename -- "$bitstream")
		filename=${basename%*/}
		bitstream=${outdir}/${filename%.*}.vc5
	else
		# Derive the output filename from the metadata pathname
		#bitstream=${outdir}/$(basename ${metadata} .xml).vc5
		basename=$(basename -- "$metadata")
		filename=${basename%*/}
		bitstream=${outdir}/${filename%.*}.vc5
	fi
fi

# Is it necessary to make the test case?
if [ -z "$force" ] && [ -f "$bitstream" ] && [ -f "$metadata" ] && [ "$metadata" -ot "$bitstream" ]; then
[ -n "$verbose" ] && echo "Skipping test case: $bitstream"
exit 0
fi

# Detailed logic for skipping the test case (for debugging)
# if [ -z "$force" ]; then
# 	echo "Force flag not set"
# 	if [ -f "$bitstream" ]; then
# 		echo "Bitstream file exists: $bitstream"
# 		if [ -f "$metadata" ]; then
# 			echo "Metadata file exists: $metadata"
# 			if [ "$metadata" -ot "$bitstream" ]; then
# 				echo "Metadata test case $metadata older than the bitstream file $bitstream"
# 				echo "Skipping test case: $metadata"
# 				exit 0
# 			fi
# 		fi
# 	fi
# fi

if [ -z "$quiet" ]; then
	if [ -n "$verbose" ]; then
		# The sample encoder will print the input and output files if the verbose flag is set
		flags="-v"
	else
		# Print the input and output files if the encoder is not passed the verbose flag
		echo "Input image: ${imagefile}"
		echo "Output file: ${bitstream}"
		[ -n "$metadata" ] && echo "Inject data: ${metadata}"
	fi

	if [ -n "$debug" ]; then
		# Enable extra output for debugging
		[ -z "$flags" ] && flags="-z" || flags="$flags -z"
	fi
fi

#echo "$flags"

if [ -z "$metadata" ]; then
	# Encode the image file without embedded metadata
	#./build/linux/debug/encoder $flags -P 3 -w 1280 -h 720 $imagefile $bitstream
	echo "Must provide a metadata file in XML format on the command line"
	exit 1
else
	# Encode the image file with the specified metadata embedded in the bitstream
	./build/linux/debug/encoder $flags -q -P 7 -w 1280 -h 720 -M $metadata $imagefile $bitstream
fi

if [ -n "$metadata" ]; then
	# Derive the binary filename from the bitstream filename
	binary=${bindir}/$(basename ${bitstream} .vc5).bin

	#if [ -z "$quiet" ] && [ -n "$verbose" ]; then
	if [ -z "$quiet" ]; then
		echo "Binary file: ${binary}"
	fi

	# Extract the metadata chunk from the bitstream into a binary file
	$dumper --chunk metadata $bitstream --output $binary
fi

