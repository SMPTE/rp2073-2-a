#!/usr/bin/env bash
#
# Script for generating the decoder metadata test cases
#
# TODO: Compute the input image width and height from the pathname

# Default location of the metadata bitstreams created by the sample encoder
#root=../media/metadata
root=../encoder/metadata/bitstreams

# Default output image file
#imagefile=../media/boxes/master/part1/release/boxes-1280x720-0000-part1-1280x720-byr4-vc5.byr4
#imagefile=./test/boxes/1280x720/rg48/boxes-1280x720-0000-vc5.rg48

# Location for the metadata test cases output by the decoder
#outdir=./test
outdir=./metadata

# Default image width and height
width=1280
height=720

# Default output image format
output=rg48

# Define the usage message
usage() { echo "Usage: $0 [-v] [-z] [-q] [-f] [-w width] [-h height] [-o format] [-m metadata] [-d <outdir>] [-r <root>] bitstream.vc5" 1>&2; exit 1; }

# Print the usage message if no command-line arguments
[ $# -eq 0 ] && usage

# Parse the command-line options
#while getopts ":hi:o:m:d:r:vzq" arg; do
while getopts ":w:h:o:m:d:r:fvzq" arg; do

	case $arg in

#	  i) # Specify the input image file
#		imagefile=${OPTARG}
#		;;
#
#	  o) # Specify the output bitstream file
#		bitstream=${OPTARG}
#		;;

	w) # Image width
		width=${OPTARG}
		;;

	h) # Image height
		height=${OPTARG}
		;;

	o) # Output image format
		output=${OPTARG}
		;;

	m) # Metadata output file
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

	?) # Display the usage message
		usage
		exit 0
		;;

	esac

done

shift $((OPTIND - 1))

# Get the required bitstream argument
bitstream=$1
#echo "Bitstream:  $bitstream"

# Get the first output image argument
#shift $((OPTIND - 1))
#imagefile=$2
#echo $imagefile

#TODO: Get the remaining image files from the command line

# Create the output directory
if [ ! -d "$outdir" ]; then
	echo "Creating directory tree: $outdir"
	mkdir -p $outdir/decoded
	mkdir -p $outdir/images
fi

# Was the output image file specified?
if [ -z "$imagefile" ] && [ -n "$output" ]; then
	if [ -z "$metadata" ]; then
		# Derive the output image filename from the bitstream pathname
		filename=${bitstream##*/}
		imagefile=${outdir}/images/${filename%%.*}.${output}
	else
		# Derive the output image filename from the metadata pathname
		filename=${metadata##*/}
		imagefile=${outdir}/images/${filename%%.*}.${output}
	fi
fi

#echo "Bitstream:  $bitstream"

if [ -z "$imagefile" ]; then
	echo "Must provide an output image file or format"
	exit 1
#else
#	echo "Image file: $imagefile"
fi

# Derive the metadata output filename from the bitstream
if [ -z "$metadata" ]; then
	metadata=${outdir}/decoded/${filename%%.*}.xml
fi

#echo "$metadata"

# Was a metadata file provided on the command line or derived from the bitstream?
if [ -n "$metadata" ]; then
	# Does the output metadata file have a directory path?
	directory=${metadata%/*}
	#echo $directory
	if [ "$directory" == "$metadata" ]; then
		# Prefix the metadata file with the output directory
		metadata=${outdir}/decoded/${metadata}
	fi

	#echo "Metadata:   $metadata"
fi

# Is it necessary to make the test case?
if [ -z "$force" ] && [ -f "$metadata" ] && [ -f "$bitstream" ] && [ "$bitstream" -ot "$metadata" ]; then
	[ -n "$verbose" ] && echo "Skipping test case: $metadata"
	exit 0
fi

# Detailed logic for skipping the test case (for debugging)
# if [ -z "$force" ]; then
#	echo "Force flag not set"
#	if [ -f "$metadata" ]; then
#		echo "Metadata file exists: $metadata"
#		if [ -f "$bitstream" ]; then
#			echo "Bitstream file exists: $bitstream"
#			if [ "$bitstream" -ot "$metadata" ]; then
#				echo "Bitstream file $bitstream older than metadata test case $metadata"
#				echo "Skipping test case: $metadata"
#				exit 0
#			fi
#		fi
#	fi
# fi

if [ -z "$quiet" ]; then
	if [ -n "$verbose" ]; then
		# The reference decoder will print the input and output files if the verbose flag is set
		flags="-v"
	else
		# Print the input and output files if the decoder is not passed the verbose flag
		echo "Input bitstream: ${bitstream}"
		echo "Output pathname: ${imagefile}"
		[ -n "$metadata" ] && echo "Metadata output: ${metadata}"
		echo ""
	fi

	if [ -n "$debug" ]; then
		# Enable extra output for debugging
		[ -z "$flags" ] && flags="-z" || flags="$flags -z"
	fi
fi

#echo "$flags"

if [ -z "$metadata" ]; then
	# Decode the bitstream and output the decoded image
	[ -n "$verbose" ] && echo "./build/linux/debug/decoder $flags -P 3 -w $width -h $height $bitstream $imagefile"
	./build/linux/debug/decoder $flags -P 3 -w $width -h $height -o $output $bitstream $imagefile
else
	# Decode the bitstream and extract the metadata into the output file
	[ -n "$verbose" ] && echo "./build/linux/debug/decoder $flags -P 7 -w $width -h $height -M $metadata $bitstream $imagefile"
	./build/linux/debug/decoder $flags -P 7 -w $width -h $height -o $output -M $metadata $bitstream $imagefile
fi

