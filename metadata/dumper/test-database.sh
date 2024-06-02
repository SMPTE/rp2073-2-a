#!/usr/bin/env bash
#
# Script for testing database code in the XML dumper

# Duplicates as removed if the -d flag is passed to the dumper on the command line
flags="-d"

#testcases="intrinsic/simple/bin/simple-rdc1.bin intrinsic/simple/bin/simple-rdc2.bin intrinsic/simple/bin/simple-rdc4.bin intrinsic/simple/bin/simple-rdc7.bin"
testcases="intrinsic/simple/bin/simple-rdc1.bin"
#testcases="intrinsic/simple/bin/simple-rdc2.bin"

# Source of the test cases in XML format input to the XML parser
#source_dir="../python"
source_dir="./source"

# Root of the directory tree containing the results created by the XML dumper
output_dir="./results"

# Directory for the temporary files created for the comparison
results_dir="./temp"

# Create the output directories
if [ ! -d ${output_dir} ]; then mkdir -p ${output_dir}; fi
if [ ! -d ${results_dir} ]; then mkdir -p ${results_dir}; fi


diff_testcase() {
	testcase=$1

	testcase=`echo $testcase | sed -e 's/bin/xml/g'`
	#echo $testcase

	#source_file=${source_dir}/${testcase}
	source_file=${source_dir}/$(basename $testcase)
	output_file=${output_dir}/$(basename $testcase)

	basename=$(basename ${output_file})
	#printf "Processing: ${basename}\n"

	#echo ${source_file}
	#echo ${output_file}

	xmllint --c14n --noblanks "${source_file}" > ${results_dir}/${basename}.source
	xmllint --c14n --noblanks "${output_file}" > ${results_dir}/${basename}.output

	diff -w ${results_dir}/${basename}.source ${results_dir}/${basename}.output >>differences.log || echo "Differences: ${basename}"
}


for testcase in $testcases; do
	output=${output_dir}/$(basename ${testcases} .bin).xml
	#echo $output
	./build/linux/debug/dumper $flags -o $output ../parser/binary/$testcase
	diff_testcase $testcase
done
