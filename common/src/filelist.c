/*! @file common/src/filelist.c
 
	@brief Implementation of data structure for representing lists of files.
	
    This module abstracts away the details of keeping track of lists of files to be processed.

    Files can be listed explicitly or implicitly using a format string to enumerate the file list.
 
    (c) 2016 Society of Motion Picture & Television Engineers LLC and Woodman Labs, Inc.
	All rights reserved--use subject to compliance with end user license agreement.
 */

#include <string.h>
#include "headers.h"
#include "filelist.h"


/*! @brief Initialize a file list.
 
    Allocated memory is not released before the data structure is initialized.
 */
CODEC_ERROR InitFileList(FILELIST *filelist, ALLOCATOR *allocator)
{
    assert(filelist != NULL);
    if (! (filelist != NULL)) {
        return CODEC_ERROR_NULLPTR;
    }
    
    // Clear all fields and set the memory allocator
    memset(filelist, 0, sizeof(FILELIST));
    filelist->allocator = allocator;
    return CODEC_ERROR_OKAY;
}

/*! @brief Release memory allocated for a file list.
 
    Allocated memory is not released before the data structure is initialized.
 */
CODEC_ERROR ReleaseFileList(FILELIST *filelist)
{
    int index;
    
    assert(filelist != NULL);
    if (! (filelist != NULL)) {
        return CODEC_ERROR_NULLPTR;
    }

    for (index = 0; index < MAX_FILELIST_PATHNAME_COUNT; index++) {
        Free(filelist->allocator, filelist->pathname_list[index]);
    }
    
    return CODEC_ERROR_OKAY;
}

/*! @brief Add a pathname to the end of the file list
 
 */
CODEC_ERROR AddFileListPathname(FILELIST *filelist, const char *pathname)
{
    assert(filelist != NULL);
    if (! (filelist != NULL)) {
        return CODEC_ERROR_NULLPTR;
    }
    
    // Cannot add a non-template pathname after a pathname template has been added to the file list
    if (filelist->template_flag) {
        return CODEC_ERROR_BAD_ARGUMENT;
    }
 
    if (filelist->pathname_count < MAX_FILELIST_PATHNAME_COUNT)
    {
        size_t pathname_string_size = PATH_MAX * sizeof(char);
        filelist->pathname_list[filelist->pathname_count] = Alloc(filelist->allocator, pathname_string_size);
        
        assert(filelist->pathname_list[filelist->pathname_count] != NULL);
        if (! (filelist->pathname_list[filelist->pathname_count] != NULL)) {
            return CODEC_ERROR_OUTOFMEMORY;
        }
        
        // Copy the pathname string with a terminating nul character
        strncpy(filelist->pathname_list[filelist->pathname_count], pathname, PATH_MAX);
        filelist->pathname_list[filelist->pathname_count][PATH_MAX - 1] = '\0';

        // Update the count of pathnames in the file list
        filelist->pathname_count++;
        return CODEC_ERROR_OKAY;
    }
    
    // Not enough space to add enother pathname string
    return CODEC_ERROR_OUTOFMEMORY;
}

CODEC_ERROR AddFileListTemplate(FILELIST *filelist, const char *string)
{
    CODEC_ERROR error = CODEC_ERROR_OKAY;
    
    assert(filelist != NULL);
    if (! (filelist != NULL)) {
        return CODEC_ERROR_NULLPTR;
    }

    // Cannot add another pathname template after a pathname template has been added to the file list
    if (filelist->template_flag) {
        return CODEC_ERROR_BAD_ARGUMENT;
    }
    
    error = AddFileListPathname(filelist, string);
    if (error != CODEC_ERROR_OKAY) {
        return error;
    }
    
    // Can only have at most one pathname template in a file list
    filelist->template_flag = true;
    
    return CODEC_ERROR_OKAY;
}

/*!
    @brief Return true of the pathname is a format string
*/
bool IsPathnameTemplate(const char *pathname)
{
    return strchr(pathname, '%') != NULL;
}

CODEC_ERROR GetNextFileListPathname(FILELIST *filelist, char *pathname, size_t size)
{
    int last_pathname_index;
    
    assert(filelist != NULL);
    if (! (filelist != NULL)) {
        return CODEC_ERROR_NULLPTR;
    }
    
    assert(pathname != NULL);
    if (! (pathname != NULL)) {
        return CODEC_ERROR_NULLPTR;
    }
    
    // Initialize the last pathname with a placeholder (for debugging)
    strcat(filelist->last_pathname, "(unknown)");
    
    // The last pathname in the file list may be a template
    last_pathname_index = filelist->pathname_count - 1;
    
    // Is the next pathname an instance of a pathname template?
    if (filelist->template_flag && filelist->pathname_index == last_pathname_index)
    {
        snprintf(pathname, size, filelist->pathname_list[last_pathname_index], filelist->template_index);
        filelist->template_index++;
    }
    else
    {
        if (filelist->pathname_index >= filelist->pathname_count)
        {
            // No more pathnames remaining in the file list
            return CODEC_ERROR_FILELIST_MISSING_PATHNAME;
        }
        
        // Copy the next pathname to the output buffer
        memset(pathname, 0, size);
        strncpy(pathname, filelist->pathname_list[filelist->pathname_index], size);
        pathname[size - 1] = '\0';
        
        // Increment the pathname index
        filelist->pathname_index++;
    }

    // Save the pathname so that the calling routine can use it in an error message
    memset(filelist->last_pathname, 0, sizeof(filelist->last_pathname));
    strncpy(filelist->last_pathname, pathname, sizeof(filelist->last_pathname));
    filelist->last_pathname[sizeof(filelist->last_pathname) - 1] = '\0';

    return CODEC_ERROR_OKAY;
}

/*! @brief Return true if the filelist provides exactly one pathname
*/
bool FileListHasSinglePathname(const FILELIST *filelist)
{
    assert(filelist != NULL);
    if (! (filelist != NULL)) {
        return false;
    }

    // A filelist represents exactly one pathname if there is one pathname that is not a template
    return (filelist->pathname_count == 1 && !filelist->template_flag);
}

/*! @brief Return the single pathname provided by the file list
 
    This routine does not check whether there is a single pathname so the result is unpredictable.
    Calling routine should use the predicate @ref FileListHasSinglePathname to determine whether
    the file list contains only a single pathname.
 */
const char *SingleFileListPathname(const FILELIST *filelist)
{
    assert(filelist != NULL);
    if (! (filelist != NULL)) {
        return NULL;
    }

    return filelist->pathname_list[0];
}
