#!/usr/bin/env bash
#
# Compare the results of test cases read from test cases list

# File containing the list of test cases
#testcases="./testcases.intrinsic"
#testcases="./testcases.streaming"
#testcases="./testcases.list"

# Use the default list of test cases if none provided on the command line
testcases=${1:-./testcases.intrinsic}
#echo ${testcases}

# Source of the test cases in XML format input to the XML parser
source_dir="../python"

# Root of the directory tree containing the results created by the XML dumper
output_dir="./output"

# Directory for the temporary files created for the comparison
results_dir="./temp"

if [ ! -d ${results_dir} ]; then mkdir -p ${results_dir}; fi

# Clear the log of detailed differences between test cases
echo -n "" >differences.log

# Pipe the list of processed test case files to this script
while IFS= read -r testcase
do
	#printf "${source_dir}/${testcase} ${output_dir}/${testcase}\n"

	source_file=${source_dir}/${testcase}
	output_file=${output_dir}/${testcase}

	basename=$(basename ${output_file})
	#printf "Processing: ${basename}\n"

	xmllint --c14n --noblanks "${source_file}" > ${results_dir}/${basename}.source
	xmllint --c14n --noblanks "${output_file}" > ${results_dir}/${basename}.output

	#if [ diff ${results_dir}/${basename}.source ${results_dir}/${basename}.output ]; then echo "Differences: ${basename}"; fi
	#cmp  ${results_dir}/${basename}.source ${results_dir}/${basename}.output || echo "Differences: ${basename}"
	diff -w ${results_dir}/${basename}.source ${results_dir}/${basename}.output >>differences.log || echo "Differences: ${basename}"

done < <(sed -e 's/bin/xml/g' ${testcases})
