#!/usr/bin/env tclsh
#
# Script for testing the VC-5 sample encoder and reference decoder
#
# (c) 2013-2017 Society of Motion Picture & Television Engineers LLC and Woodman Labs, Inc.
# All rights reserved--use subject to compliance with end user license agreement.

#TODO: Need to add support for testing image sections

package require cmdline

# Set the root location of the VC-5 test materials
#set root [file normalize [file join [pwd] ..]]
set root [file normalize [file join [pwd] [file dirname $argv0] ..]]

# Choose the default build configuration to run
set configuration release

# Include the platform-specific definitions file
set platform $tcl_platform(platform)
set pathname [format "%s-%s.%s" [file root [info script]] $platform tcl]
if [file isfile $pathname] {
	source $pathname
}

# Include the machine-specific definitions file
set hostname [file rootname [info hostname]]
set pathname [format "%s-%s.%s" [file root [info script]] $hostname tcl]
if [file isfile $pathname] {
	source $pathname
}

# Use the filename of this script as the name for this test run
set name [file rootname [file tail $argv0]]

# Default values for test parameters
set default(sections) {2 3 4 5 6}

# Define the command-line arguments
set options {
	{r.arg  ""       "set the root directory that contains the codec"}
	{m.arg  ""       "set the location of the test cases"}
    {b.arg  release  "set the build configuration (debug or release)"}
    {c.arg  make     "set the type of build used to create the encoder and decoder"} 
	{t.arg  ""       "list of test cases separated by commas with no spaces"}
	{p.arg  1        "part of VC-5 standard to test"}
	{s.arg  ""       "comma-separated list of sections enabled during testing (part 6 only)"}
	{l.arg  ""       "log output to the specified file"}
	{d      0        "output a file containing a displayable picture (for debugging)"}
    {v      0        "enable verbose output"}
    {e      0        "extra output for debugging"}
    {x      0        "do not execute the codec (for debugging this script)"}
    {a      0        "perform all tests"}
}

# Set the usage string for the command-line options
set usage ": tclsh $argv0 \[options] filename ...\noptions:"

# Parse the command-line options
array set params [::cmdline::getoptions argv $options $usage]

# Replace default settings with options from the command line
if {[string length $params(r)] > 0} {set root [file normalize $params(r)]}
if {[string length $params(m)] > 0} {set media [file normalize $params(m)]}

if {[string length $params(t)] > 0} {
	# Use the list of test cases provided on the command line
	set testlist [split $params(t) ","]
} else {
	# Run the default test cases
	set testlist {solid gradient boxes}
}
set configuration [string tolower $params(b)]
set compiler [string tolower $params(c)]
set display $params(d)
set verbose $params(v)
set debug $params(e)
set dryrun $params(x)
set number $params(p)
set part [format "part%d" $number]

# Create list of section numbers enabled for testing if VC-5 Part 6 is under test
if {[string compare $part part6] == 0} {
	# Split the comma-separated list of section numbers into a list
	set sections [split $params(s) ","]
	if [expr [llength $sections] == 0] {set sections $default(sections)}
}  else {
	# Ignore the sections argument if VC-5 Part 6 is not enabled
	set sections {}
}

# Set the pathnames to the encoder and decoder
switch -exact $compiler {
	cmake {
		set encoder [file join $root encoder build cmake [string totitle $configuration] $encoder]
		set decoder [file join $root decoder build cmake [string totitle $configuration] $decoder]
	}
	xcode {
		set encoder [file join $root encoder Xcode [string totitle $configuration] $encoder]
		set decoder [file join $root decoder Xcode [string totitle $configuration] $decoder]
	}
	make -
	default {
		set encoder [file join $root encoder build $build $configuration $encoder]
		set decoder [file join $root decoder build $build $configuration $decoder]
	}
}

# Verify that the executables exist
if {! [file executable $encoder]} {
	puts "Encoder does not exist: $encoder"
	exit
}

if {! [file executable $decoder]} {
	puts "Decoder does not exist: $decoder"
	exit
}

# Open a file for logging information about the test results
if {[string length $params(l)] == 0} {
	set logfile stdout
} else {
	set logfile [open $params(l) "w"]
}

set all_tests $params(a)

# Create the directory if it does not already exist
proc check {pathname} {
	if {! [file isdir $pathname]} {
		file mkdir $pathname
	}
	return $pathname
}

# Return true if two binary files are equal
proc compare {pathname1 pathname2} {
	global debug
	if $debug {print_log "Compare: $pathname1"}
	if $debug {print_log "         $pathname2"}
	set file1 [open $pathname1 rb]
	set file2 [open $pathname2 rb]
	if $debug {print_log "Open file: $file1"}
	if $debug {print_log "           $file2"}
	set result [expr [string compare [read $file1] [read $file2]] == 0]
	close $file1
	close $file2
	return $result
}

# Check whether the pixel format is supported by the specified part
proc supported {format part} {
	# Only NV12 images are supported for testing VC-5 Part 4
	if {[string compare $part part4] == 0} {
		return [expr [string compare $format nv12] == 0]
	} else {
		return [expr [string compare $format nv12] != 0]
	}
}

# Print command in a readable format
proc print_command command_list {
	global logfile
	#set command_list [split $command_line]
	foreach argument $command_list {
		puts $logfile $argument
	}
	puts $logfile ""
}

# Execute command passed as a list of program pathname and arguments
proc execute_command command_list {
	global verbose
	if $verbose {print_command $command_list}
	#puts [join [concat exec $command_list]]
	eval [join [concat exec $command_list]]
}

# Print output to the logfile and the terminal
proc print_and_log {message} {
	global logfile
	puts $message
	if {[string compare $logfile stdout] != 0} {
		puts $logfile $message
	}
}

# Print output to logfile and to the terminal if verbose output enabled
proc print_verbose {message} {
	global logfile verbose
	if $verbose {puts $message}
	if {[string compare $logfile stdout] != 0} {puts $logfile $message}
}

# Print output to the logfile only
proc print_log {message} {
	global logfile
	if {[string compare $logfile stdout] != 0} {
		puts $logfile $message
	}
}

# Print elements of a list in a readable format
proc print_list {list {before 1} {after 1}} {
	if [expr $before > 0] {puts ""}
	foreach element $list {
		puts $element
	}
	if [expr $after > 0] {puts ""}
}

# Assemble a string of command-line options for enabled parts and sections
proc parts_and_sections {part_number section_list} {
	if [expr $part_number == 6] {
		return [list "-P $part_number" "-S [join $section_list ","]"]
	} else {
		return [list "-P $part_number"]
	}
}

# Test the encoder and decoder using a sinble image (the most simple test case)
proc test_codec_single {testcase part number sections dimension format results} {
	global decoder encoder configuration media name logfile debug verbose dryrun display failures missing testcount

	# Test each image for this combination of image dimensions and pixel format
	set directory [file join $media $testcase $dimension $format]
	foreach image [glob -tails -directory $directory *.$format] {
	
		# Set the full pathname to the input image for testing the encoder
		set input [file join $directory $image]
		
		# Set the full pathname for the bitstream output by the encoder
		set output [file join $media $testcase $results [format "%s-%s-%s-%s.%s" [file rootname $image] $part $dimension $format vc5]]
		
		# Set the full pathname for the decoded image
		set result [file join $media $testcase $results [format "%s-%s-%s-%s-%s.%s" [file rootname $image] $part $dimension $format vc5 $format]]
		
		if $display {
			# Set the full pathname for the displayable picture
			set picture [file join $media $testcase $results [format "%s-%s-%s-%s-%s.%s" [file rootname $image] $part $dimension $format vc5 dpx]]
		}
		
		#print_verbose "Testing input: $input"

		# Split the image dimensions into the width and height
		lassign [split $dimension x] width height

		set enabled [parts_and_sections $number $sections]

		set encoder_command [concat "$encoder" [list "-w $width"] [list "-h $height"] "$enabled" "$input" "$output"]
		set decoder_command [concat "$decoder" [list "-p $format"] "$enabled" "$output" "$result"]

		if $display {
			lappend $decoder_command "$picture"
		}

		# Apply the encoder to the input image to produce an encoded bitstream
		if $dryrun {
			print_command $encoder_command
		} else {
			execute_command $encoder_command
		}

		# Decode the bitstream output by the encoder
		if $dryrun {
			print_command $decoder_command
		} else {
			execute_command $decoder_command
		}

		if $dryrun {
			incr testcount
		} else {
			# Compare the encoded bitstream file with the master file
			#set filename [format "%s-%s-%s.%s" [file rootname $image] $dimension $format vc5]
			#set master [file join $media $testcase master $part $configuration $filename]
			set filename [file tail $output]
			set master [file join $media $testcase master $part $configuration $filename]
			# if $debug {
			# 	puts $output
			# 	puts $master
			# }
			if [file exists $master] {
				#print_verbose "Checking result against master: $master"
				if {! [compare $output $master]} {
					print_and_log "Failed bitstream comparison: [file tail $master] ($configuration)"
					incr failures
				}
			} else {
				if $debug {print_and_log "Missing bitstream master file: $master"}
				incr missing
			}

			# Compare the decoded image file with the master file
			set filename [file tail $result]
			set master [file join $media $testcase master $part $configuration $filename]
			# if $debug {
			# 	puts $result
			# 	puts $master
			# }
			if [file exists $master] {
				#print_verbose "Checking result against master: $master"
				if {! [compare $result $master]} {
					print_and_log "Failed decoded image comparison: [file tail $master] ($configuration)"
					incr failures
				}
			} else {
				if $debug {print_and_log "Missing decoded image master file: $master"}
				incr missing
			}

			incr testcount
		}
	}
}

# Test encoding and decoding using all files with the same dimensions and format
proc test_codec_layers {testcase part number sections dimension format results} {
	global decoder encoder configuration media name logfile debug verbose dryrun display failures missing testcount

	# Encode all files in the directory as layers
	set directory [file join $media $testcase $dimension $format]
	set input [glob -directory $directory *]

	# Set the full pathname for the bitstream output by the encoder
	set output [file join $media $testcase $results [format "%s-%s-%s-%s.%s" $testcase $part $dimension $format vc5]]
	
	# Assemble a list of decoded image files from the encoder input file list
	set result {}
	foreach image $input {
		set basename [file rootname [file tail $image]]
		set filename [format "%s-%s-%s-%s-%s.%s" $basename $part $dimension $format vc5 $format]
		set pathname [file join $media $testcase $results $filename]
		lappend result $pathname
	}

	#puts $result

	# if $display {
	# 	# Set the full pathname for the displayable picture
	# 	set picture [file join $media $testcase $results [format "%s-%s-%s-%s-%s.%s" [file rootname $image] $part $dimension $format vc5 dpx]]
	# }
		
	#print_verbose "Testing input: $input"

	# Split the image dimensions into the width and height
	lassign [split $dimension x] width height

	set enabled [parts_and_sections $number $sections]

	set encoder_command [concat "$encoder" [list "-w $width"] [list "-h $height"] "$enabled" "$input" "$output"]
	set decoder_command [concat "$decoder" [list "-p $format"] "$enabled" "$output" "$result"]

	# if $display {
	# 	lappend $decoder_command "$picture"
	# }

	# Apply the encoder to the input images to produce an encoded bitstream
	if $dryrun {
		print_command $encoder_command
	} else {
		execute_command $encoder_command
	}

	# Decode the bitstream output by the encoder
	if $dryrun {
		print_command $decoder_command
	} else {
		execute_command $decoder_command
	}

	if $dryrun {
		incr testcount
	} else {
		# Compare the encoded bitstream file with the master file
		#set filename [format "%s-%s-%s.%s" [file rootname $image] $dimension $format vc5]
		#set master [file join $media $testcase master $part $configuration $filename]
		set filename [file tail $output]
		set master [file join $media $testcase master $part $configuration $filename]
		# if $debug {
		# 	puts $output
		# 	puts $master
		# }
		if [file exists $master] {
			#print_verbose "Checking result against master: $master"
			if {! [compare $output $master]} {
				print_and_log "Failed bitstream comparison: [file tail $master] ($configuration)"
				incr failures
			}
		} else {
			if $debug {print_and_log "Missing bitstream master file: $master"}
			incr missing
		}

		# Compare the decoded image file with the master file
		foreach image $result {
			set filename [file tail $image]
			set master [file join $media $testcase master $part $configuration $filename]
			#puts [format "Master: %s" $master]
			# if $debug {
			# 	puts $result
			# 	puts $master
			# }
			if [file exists $master] {
				#print_verbose "Checking result against master: $master"
				if {! [compare $image $master]} {
					print_and_log "Failed decoded image comparison: [file tail $master] ($configuration)"
					incr failures
				}
			} else {
				if $debug {print_and_log "Missing decoded image master file: $master"}
				incr missing
			}
		}

		incr testcount
	}
}

# Traverse the tree of image resolutions and pixel formats to process every file in each of the test cases in the list
proc test_codec {testlist part number {sections {}}} {
	global decoder encoder configuration media name logfile debug verbose dryrun display failures missing testcount

	foreach testcase $testlist {

		#if $debug {puts "Test case: $testcase"}
		print_log "\nTest case: $testcase\n"
		
		# Set the working directory for any debug output from the programs
		cd [check [file join $media $testcase temp]]

		# Use the filename of this script and date for the results folder
		set testrun [clock format [clock seconds] -format "$name-%Y-%m-%d"]

		# Create the directory for the test results
		set results [file join results $testrun [string tolower $part] $configuration]
		check [file join $media $testcase $results]
		
		set directory [file join $media $testcase]
		if $debug {puts "Directory: $directory"}

		# Test each image width and height in this test case
		foreach dimension [glob -tails -directory $directory {[0-9]*}] {
		
			if $debug {puts "Dimension: $dimension"}
			
			# Split the image dimensions into the width and height
			#lassign [split $dimension x] width height
			
			set directory [file join $media $testcase $dimension]
			if $debug {puts "Directory: $directory"}

			# Test each pixel format for this image width and height
			foreach format [glob -tails -nocomplain -directory $directory *] {

				# Check whether this format is supported by this part
				if {! [supported $format $part]} {
					# Skip this pixel format
					continue
				}
			
				if $debug {puts "Format: $format"}
			
				if {[string compare $part part5] == 0} {
					test_codec_layers $testcase $part $number $sections $dimension $format $results
				} else {
					test_codec_single $testcase $part $number $sections $dimension $format $results
				}
			}
		}
	}
}


proc test_image_sections {testlist sections} {
	global decoder encoder configuration media name logfile debug verbose dryrun display failures missing testcount

	set test_dir [file join $media sections]
	set enabled [parts_and_sections 6 $sections]

	foreach test [glob [file join $test_dir "*"]] {
		if [file isdir $test] {
			set testcase [file tail $test]

			# Skip over the directories that contains test results and master files
			if {[lsearch -exact {results master} $testcase] >= 0} {
				#puts "Skipping sections testcase: $testcase"
				continue
			}

			#print_and_log "Image sections testcase: $testcase, sections: [list $sections]"
			print_verbose "Image sections testcase: $testcase, sections: [list $sections]"

			set inputs [glob [file join $test "*"]]
			set filename [format "%s.%s" [file rootname [file tail $test]] vc5]
			set testrun [clock format [clock seconds] -format "$name-%Y-%m-%d"]
			set outdir [file join $media sections results $testrun $testcase $configuration]
			check $outdir
			set output [file join $outdir $filename]
			set results {}
			foreach file $inputs {
				set rootname [file rootname $file]
				set basename [file tail $rootname]
				set extension [file extension $file]
				set filename [format "%s-%s%s" $basename vc5 $extension]
				lappend results [file join $outdir $filename]
			}

			if $debug {print_list $results}

			set encoder_command [concat "$encoder" "$enabled" "$inputs" "$output"]
			set decoder_command [concat "$decoder" "$enabled" "$output" "$results"]

			if $display {
				lappend $decoder_command "$picture"
			}

			# Apply the encoder to the input image to produce an encoded bitstream
			if $dryrun {
				print_command $encoder_command
			} else {
				execute_command $encoder_command
			}

			# Decode the bitstream output by the encoder
			if $dryrun {
				print_command $decoder_command
			} else {
				execute_command $decoder_command
			}

			if $dryrun {
				incr testcount
			} else {
				# Compare the encoded file with the master file
				#set filename [format "%s-%s-%s.%s" [file rootname $image] $dimension $format vc5]
				#set master [file join $media $testcase master $part $configuration $filename]
				set filename [file tail $output]
				set master [file join $media sections master $testcase $configuration $filename]
				# if $debug {
				# 	puts $output
				# 	puts $master
				# }
				if [file exists $master] {
					#print_verbose "Checking result against master: $master"
					if {! [compare $output $master]} {
						print_and_log "Failed bitstream comparison: [file tail $master] ($configuration)"
						incr failures
					}
				} else {
					if $debug {print_and_log "Missing bitstream master file: $master"}
					incr missing
				}

				# Compare the decoded image files with the master files
				foreach result $results {
					set filename [file tail $result]
					set master [file join $media sections master $testcase $configuration $filename]
					# if $debug {
					# 	puts $result
					# 	puts $master
					# }
					if [file exists $master] {
						#print_verbose "Checking result against master: $master"
						if {! [compare $result $master]} {
							print_and_log "Failed decoded image comparison: [file tail $master] ($configuration)"
							incr failures
						}
					} else {
						if $debug {print_and_log "Missing decoded image master file: $master"}
						incr missing
					}
				}

				incr testcount
			}
		}
	}
}


# Output information to the logfile and to the terminal if verbose enabled
print_verbose "Root directory for test materials: $root"
print_verbose "Root directory for the test media: $media"
print_verbose "Platform: $platform"
print_verbose "Build system: $compiler"
print_verbose "Configuration: $configuration"


if $all_tests {
	print_verbose "Performing all tests"
} else {
	# Image section testing uses a different set of test cases
	if {[string compare $part part6] == 0 && [lsearch -exact $sections 1] >= 0} {
		print_verbose "Codec part: $part"
		print_verbose "Sections: $sections"
		print_verbose "Running image section testcases"
	} else {
		print_verbose "Test cases: $testlist"
		print_verbose "Codec part: $part"

		if {[string compare $part part6] == 0} {
			print_verbose "Sections: $sections"
		}
	}
}


### Tabulate the test results ###

# Initialize the count of failed test cases
set failures 0

# Initialize the count of missing master files
set missing 0

# Initialize the count of tests performed
set testcount 0


### This is the main loop of the test script ###

if $all_tests {
	# Load the list of tests in the comprehensive test suite
	set pathname [format "%s-%s.%s" [file root [info script]] suite tcl]
	source $pathname

	# Perform all tests in the comprehensive test suite
	foreach test $test_suite {
		#puts $test
		set testcase [dict get $test testcase]
		if {[string compare $testcase sections] == 0} {
			set testlist {}
			set sections [dict get $test sections]
			test_image_sections $testlist $sections
		} else {
			set part [dict get $test part]
			set number [dict get $test number]
			set testlist [list $testcase]
			if {[string compare $part part6] == 0} {
				set sections [dict get $test sections]
				#print_and_log "Test case: $testcase, part: $part, sections: [list $sections]"
				print_verbose "Test case: $testcase, part: $part, sections: [list $sections]"
				test_codec $testlist $part $number $sections
			} else {
				#print_and_log "Test case: $testcase, part: $part"
				print_verbose "Test case: $testcase, part: $part"
				test_codec $testlist $part $number
			}
		}
	}

	if $dryrun {
		print_and_log "Dry run count: $testcount"
	} else {
		if [expr $missing > 0] {
			print_and_log "Finished $testcount tests with $missing missing master files for $configuration build ($failures failures)"
		} else {
			print_and_log "Finished $testcount tests using $configuration build ($failures failures)"
		}
	}

} elseif {[string compare $part part6] == 0 && [lsearch -exact $sections 1] >= 0} {
	# Image sections are handled as a special case
	test_image_sections $testlist $sections

	if $dryrun {
		print_and_log "Dry run count: $testcount"
	} else {
		if [expr $missing > 0] {
			print_and_log "Finished all image sections tests with $missing missing master files for $configuration build ($failures failures)"
		} else {
			print_and_log "Image sections tests done using $configuration build ($failures failures)"
		}
	}

} else {
	# Perform only the tests specified by the command-line parameters
	test_codec $testlist $part $number $sections

	if $dryrun {
		print_and_log "Dry run count: $testcount"
	} else {
		if [expr $missing > 0] {
			print_and_log "Finished testcases [list $testlist] with $missing missing master files for $configuration build ($failures failures)"
		} else {
			print_and_log "Testcases [list $testlist] done using $configuration build ($testcount tests, $failures failures)"
		}
	}
}


# Close the log file
if {[string compare $logfile stdout] != 0} {
	close $logfile
}
