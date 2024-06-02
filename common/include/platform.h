/*! @file common/include/platform.h

	Platform-specific definitions.
	
	Encapsulate platform-specific changes that affect the header files in this file to
	simplify porting to other architectures and build tools.

	Source files may require platform-specific code in that module.  For example, the
	timer implementation requires inclusion of Windows-specific facilities that should
	not be exposed to the larger codebase.

	(c) 2013 Society of Motion Picture & Television Engineers LLC and Woodman Labs, Inc.
	All rights reserved--use subject to compliance with end user license agreement.
*/

#ifndef _PLATFORM_H
#define _PLATFORM_H

#ifdef _WIN32

// Turn off warnings about deprecated functions
#pragma warning(disable: 4996)

// Map the maximum pathname length on Windows to a platform-independent definition
#define PATH_MAX _MAX_PATH

#ifndef inline
#define inline __inline
#endif

#endif

#endif
