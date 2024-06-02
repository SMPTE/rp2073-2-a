# Comprehensive codec test suite
#
# (c) 2013-2017 Society of Motion Picture & Television Engineers LLC and Woodman Labs, Inc.
# All rights reserved--use subject to compliance with end user license agreement.

# Initialize the list of tests
set test_suite {}

# Set default values for the test cases and parameter combinations to test
set default(testcases) {boxes gradient solid}
set default(parts) {1 3 4 5 6}
set default(sections) {2 3 4 5 6}


# Create a test for the specified testcase and part number
proc create_test {testcase number {sections {}}} {
	global default

	set part [format "part%d" $number]

	# Initialize the test with the required parameters
	set codec_test [dict create testcase $testcase part $part number $number]

	# Must specify which sections are enabled if testing VC-5 Part 6 Sections
	if {[string compare $part part6] == 0} {
		# Set sections to default values if not set already
		if {[llength $sections] == 0} {
			set sections $default(sections)
		}
		dict append codec_test sections $sections
	}

	return $codec_test
}


# Create a test for every testcase and combination of VC-5 parts
foreach testcase $default(testcases) {
	foreach number $default(parts) {
		lappend test_suite [create_test $testcase $number]
	}
}

# Add a test case for image sections
lappend test_suite [create_test sections 6 {1}]
