#!/usr/bin/env bash
#
# Commands for debugging the sample encoder
#
# TODO: Compute the input image width and height from the pathname

# Default location of the metadata test cases
root=../metadata/python

# Default metadata test case in XML format
#metadata=${root}/intrinsic/simple/xml/simple-c01.xml

# Default input image file
#imagefile=../media/boxes/master/part1/release/boxes-1280x720-0000-part1-1280x720-byr4-vc5.byr4
imagefile=../media/boxes/1280x720/rg48/boxes-1280x720-0000.rg48

# Location for the output bitstream files
outdir=./test
#outdir=./metadata/bitstreams

# Default output bitstream file
#bitstream=${outdir}/$(basename ${metadata} .xml).vc5

# Define the usage message
usage() { echo "Usage: $0 [-q] [-v] [-z] [-h] [-i <imagefile>] [-o <bitstream>] [-m metadata] [-d <outdir>] [-r <root>]" 1>&2; exit 1; }

# Print the usage message if no command-line arguments
[ $# -eq 0 ] && usage

# Parse the command-line options
while getopts ":hi:o:m:d:r:vzq" arg; do

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
		filename=${imagefile##*/}
		bitstream=${outdir}/${filename%%.*}.vc5
	else
		# Derive the output filename from the metadata pathname
		#bitstream=${outdir}/$(basename ${metadata} .xml).vc5
		filename=${metadata##*/}
		bitstream=${outdir}/${filename%%.*}.vc5
	fi
fi

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
	./build/linux/debug/encoder $flags -P 3 -w 1280 -h 720 $imagefile $bitstream
else
	# Encode the image file with the specified metadata embedded in the bitstream
	./build/linux/debug/encoder $flags -P 7 -w 1280 -h 720 -M $metadata $imagefile $bitstream
fi

