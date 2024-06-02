#!/usr/bin/env python3
#
# Script to run all generate.py commands used to create metadata test cases
#
# This script can be run using the coverage command to determine code coverage

import sys
import re

# Default log file of generate.py commands used to create metadata test cases
testing_log_pathname = "testing.log"

if len(sys.argv) > 1:
    testing_log_pathname = sys.argv[1]


# Pattern for parsing lines in the testing log
test_pattern = re.compile('^(.+?)\s(.+)$')


# Current working directory read from the testing log
current_directory = None


with open(testing_log_pathname) as testing_log:

    for line in testing_log.readlines():
        #print(line.strip())
        match = re.match(test_pattern, line)
        if match:
            #print("Directory: %s, command: %s" % (match.group(1), match.group(2)))
            directory = match.group(1)
            command = match.group(2)

            if directory != current_directory:
                print("os.chdir(%s)" % directory)
                current_directory = directory

            print("%s" % command)

