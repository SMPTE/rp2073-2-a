/*!	@file config.h

	Configuration file for metadata programs and tools

	(c) 2013-2021 Society of Motion Picture & Television Engineers LLC and Woodman Labs, Inc.
	All rights reserved--use subject to compliance with end user license agreement.
*/

#ifndef _CONFIG_H
#define _CONFIG_H


// Control the use of argparse for command-line arguments
#ifndef _ARGPARSE
#define _ARGPARSE   1
#endif

//! Control the use of code for testing the reference decoder (for debugging)
#define _TESTING	1

#if _TESTING
#define _DECODER	1
#endif

#if _DECODER

// The reference decoder uses the proxy database to encapsulate the code for creating XML trees
#ifndef _DATABASE
#define _DATABASE	1
#endif

#else

// Control the use of a database for storing metadata read from binary data
#ifndef _DATABASE
#define _DATABASE	0
#endif

#endif

#endif
