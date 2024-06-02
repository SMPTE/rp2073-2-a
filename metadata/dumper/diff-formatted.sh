#!/usr/bin/env bash
#
# Compare formatted output from the XML dump program with the corresponding files input to the XML parser

inputroot='../python'

filelist=`find ./output -name '*,formatted'`

for file in $filelist
do
	#echo $file
	basename=${file##*/}
	#echo $basename
	filename=(${basename//,/ })
	#echo $filename
	directory=`dirname $file`
	#echo $directory
	subpath=`echo $directory | cut -d'/' -f3-`
	#echo $subpath
	#echo "$inputroot/$subpath"
	input="$inputroot/$subpath/$filename"
	echo $input

	#diff -w $input $file

done
