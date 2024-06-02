#!/usr/bin/env bash
#
# Commands for debugging the reference decoder
#
# TODO: Compute the input image width and height from the pathname

# Default location of the metadata test cases
root=../metadata/python

# Location for the output metadata files
outdir=./test

# Default input bitstream file
#bitstream=${outdir}/$(basename ${metadata} .xml).vc5

# Default image width and height
width=1280
height=720

# Default output image format
output=rg48

# Define the usage message
#usage() { echo "Usage: $0 [-q] [-v] [-z] [-h] [-m metadata] [-d <outdir>] [-r <root>] bitstream.vc5 <imagefile 1> [<imagefile 2> [...]]" 1>&2; exit 1; }
usage() { echo "Usage: $0 [-v] [-z] [-q] [-e] [-w width] [-h height] [-o format] [-m metadata] [-d <outdir>] [-r <root>] bitstream.vc5" 1>&2; exit 1; }

# Print the usage message if no command-line arguments
[ $# -eq 0 ] && usage

# Parse the command-line options
#while getopts ":hi:o:m:d:r:vzq" arg; do
while getopts "w:h:o:m:d:r:evzq" arg; do

	case $arg in

#     i) # Specify the input image file
# 		imagefile=${OPTARG}
# 		;;
#
#     o) # Specify the output bitstream file
#     	bitstream=${OPTARG}
# 		;;

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

	e) # Extract metadata
		extract=yes
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
	echo "Creating directory: $outdir"
	mkdir -p $outdir
fi

# Was the output image file specified?
if [ -z "$imagefile" ] && [ -n "$output" ]; then
	if [ -z "$metadata" ]; then
		# Derive the output image filename from the bitstream pathname
		filename=${bitstream##*/}
		imagefile=${outdir}/${filename%%.*}.${output}
	else
		# Derive the output image filename from the metadata pathname
		filename=${metadata##*/}
		imagefile=${outdir}/${filename%%.*}.${output}
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
if [ -z "$metadata" ] && [ -n "$extract" ]; then
	metadata=${outdir}/${filename%%.*}.xml
fi

#echo "$metadata"

# Was a metadata file provided on the command line or derived from the bitstream filename?
if [ -n "$metadata" ]; then
	# Does the output metadata file have a directory path?
	directory=${metadata%/*}
	#echo $directory
	if [ "$directory" == "$metadata" ]; then
		# Prefix the metadata file with the output directory
		metadata=${outdir}/${metadata}
	fi

	#echo "Metadata:   $metadata"
fi

if [ -z "$quiet" ]; then
	if [ -n "$verbose" ]; then
		# The reference decoder will print the input and output files if the verbose flag is set
		flags="-v"
	else
		# Print the input and output files if the decoder is not passed the verbose flag
		echo "Input bitstream: ${bitstream}"
		echo "Output pathname: ${imagefile}"
		[ -n "$metadata" ] && echo "Metadata output: ${metadata}"
	fi

	if [ -n "$debug" ]; then
		# Enable extra output for debugging
		[ -z "$flags" ] && flags="-z" || flags="$flags -z"
	fi
fi

#echo "$flags"

if [ -z "$metadata" ]; then
	# Decode the bitstream and output the decoded image
	#echo "./build/linux/debug/decoder $flags -P 3 -w $width -h $height $bitstream $imagefile"
	./build/linux/debug/decoder $flags -P 3 -w $width -h $height -o $output $bitstream $imagefile
else
	# Decode the bitstream and extract the metadata into the output file
	#echo "./build/linux/debug/decoder $flags -P 7 -w $width -h $height -M $metadata $bitstream $imagefile"
	./build/linux/debug/decoder $flags -P 7 -w $width -h $height -o $output -M $metadata $bitstream $imagefile
fi

