/*!	@file nodetree.c

	@brief Implementation of the routines for creating an XML tree from the binary metadata
*/

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <limits.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <float.h>
#include <assert.h>


#if _DECODER && !_TESTING

// Use relative paths to avoid conflicts with the decoder include paths
#include "../../common/include/config.h"
#include "../../common/include/error.h"
#include "../../common/include/swap.h"
#include "../../common/include/base64.h"
#include "../../common/include/metadata.h"

#include "mxml.h"

#include "../include/params.h"
#include "../include/nodetree.h"
#include "../include/database.h"

#else

#include "config.h"
#include "error.h"
#include "swap.h"
#include "base64.h"
#include "params.h"
#include "metadata.h"

#include "mxml.h"

#include "database.h"

#endif


#ifdef __clang__
//! Maximum depth of the XML node stack
const int MAXIMUM_XML_NODE_STACK_DEPTH = 12;
#else
//! Maximum depth of the XML node stack (defined as a macro for compilers other than Clang)
#define MAXIMUM_XML_NODE_STACK_DEPTH 12
#endif


//! Global flag that controls debug output
extern bool debug_flag;


//! Log file for reporting errors detected in the XML test cases
extern FILE *logfile;


//! Operations applied to the XML node stack
typedef enum _stack_ops
{
	STACK_UNCHANGED = 0,		//!< Leave the stack unchanged
	STACK_PUSH = 1,				//!< Push a new node onto the stack
	STACK_POP = 2,				//!< Pop the specified number of nodes from the stack
	STACK_REPLACE = 3,			//!< Replace the node at the top of the stack with a new node

} STACK_OPERATION;


// Define the tags that are nested and the tags in the nested payloads
static const struct _nested_tag_table_entry
{
	FOURCC tuple_tag;				//!< Current tuple found in the bitstream
	FOURCC nested_tag;				//!< Parent tuple of the current tuple
	int current_level_offset;		//!< Relative change in the nesting level of the current tuple
	int next_level_offset;			//!< Relative change in the nesting level after the current tuple
	int stack_change;				//!< Change applied to the XML node stack (push, pop, or replace the top element)
	int stack_pop;					//!< Number of times to pop the stack before performing the specified operation

} nested_tag_table[] = {

#if _MSC_VER

	// Table entries for encoding curve metadata

	{ 'LOGA', 'CFHD',  0,  1, STACK_PUSH,      0 },
	{ 'LOGA', 'LOGA', -1,  0, STACK_REPLACE,   0 },
	{ 'LOGA', 'LAYR', -1,  0, STACK_REPLACE,   0 },

	{ 'GAMA', 'CFHD',  0,  1, STACK_PUSH,      0 },
	{ 'GAMA', 'GAMA', -1,  0, STACK_REPLACE,   0 },
	{ 'GAMA', 'LAYR', -1,  0, STACK_REPLACE,   0 },

	{ 'LINR', 'CFHD',  0,  1, STACK_PUSH,      0 },
	{ 'LINR', 'LINR', -1,  0, STACK_REPLACE,   0 },
	{ 'LINR', 'LAYR', -1,  0, STACK_REPLACE,   0 },

	{ 'FSLG', 'CFHD',  0,  1, STACK_PUSH,      0 },
	{ 'FSLG', 'FSLG', -1,  0, STACK_REPLACE,   0 },
	{ 'FSLG', 'LAYR', -1,  0, STACK_REPLACE,   0 },

	{ 'LOGC', 'CFHD',  0,  1, STACK_PUSH,      0 },
	{ 'LOGC', 'LOGC', -1,  0, STACK_REPLACE,   0 },
	{ 'LOGC', 'LAYR', -1,  0, STACK_REPLACE,   0 },

	{ 'PQEC', 'CFHD',  0,  1, STACK_PUSH,      0 },
	{ 'PQEC', 'PQEC', -1,  0, STACK_REPLACE,   0 },
	{ 'PQEC', 'LAYR', -1,  0, STACK_REPLACE,   0 },

	{ 'HLGE', 'CFHD',  0,  1, STACK_PUSH,      0 },
	{ 'HLGE', 'HLGE', -1,  0, STACK_REPLACE,   0 },
	{ 'HLGE', 'LAYR', -1,  0, STACK_REPLACE,   0 },


	// Table entries for layer metadata

	{ 'LAYR', 'CFHD',  0,  1, STACK_PUSH,      0 },
	{ 'LAYR', 'LAYR', -1,  0, STACK_REPLACE,   0 },
	{ 'LAYR', 'LOGA', -1,  0, STACK_REPLACE,   0 },
	{ 'LAYR', 'GAMA', -1,  0, STACK_REPLACE,   0 },
	{ 'LAYR', 'LINR', -1,  0, STACK_REPLACE,   0 },
	{ 'LAYR', 'FSLG', -1,  0, STACK_REPLACE,   0 },
	{ 'LAYR', 'LOGC', -1,  0, STACK_REPLACE,   0 },
	{ 'LAYR', 'PQEC', -1,  0, STACK_REPLACE,   0 },
	{ 'LAYR', 'HLGE', -1,  0, STACK_REPLACE,   0 },


	// Table entries for streaming metadata

	{ 'DEVC', 'GPMF',  0,  1, STACK_PUSH,      0 },
	{ 'STRM', 'DEVC',  0,  1, STACK_PUSH,      0 },
	{ 'STRM', 'STRM', -1,  0, STACK_REPLACE,   0 },
	{ 'TICK', 'STRM', -1,  0, STACK_POP,       1 },
	{ 'STRM', 'TICK', -1,  0, STACK_UNCHANGED, 0 },
	{ 'DEVC', 'STRM', -2, -1, STACK_REPLACE,   1 },

#else

	// Table entries for encoding curve metadata

	{ FOURCC_VALUE("LOGA"), FOURCC_VALUE("CFHD"),  0,  1, STACK_PUSH,      0 },
	{ FOURCC_VALUE("LOGA"), FOURCC_VALUE("LOGA"), -1,  0, STACK_REPLACE,   0 },
	{ FOURCC_VALUE("LOGA"), FOURCC_VALUE("LAYR"), -1,  0, STACK_REPLACE,   0 },

	// { FOURCC_VALUE("LOGb"), FOURCC_VALUE("LOGA"),  0,  1, STACK_PUSH,      0 },

	{ FOURCC_VALUE("GAMA"), FOURCC_VALUE("CFHD"),  0,  1, STACK_PUSH,      0 },
	{ FOURCC_VALUE("GAMA"), FOURCC_VALUE("GAMA"), -1,  0, STACK_REPLACE,   0 },
	{ FOURCC_VALUE("GAMA"), FOURCC_VALUE("LAYR"), -1,  0, STACK_REPLACE,   0 },

	// { FOURCC_VALUE("GAMp"), FOURCC_VALUE("GAMA"),  0,  1, STACK_UNCHANGED, 0 },

	{ FOURCC_VALUE("LINR"), FOURCC_VALUE("CFHD"),  0,  1, STACK_PUSH,      0 },
	{ FOURCC_VALUE("LINR"), FOURCC_VALUE("LINR"), -1,  0, STACK_REPLACE,   0 },
	{ FOURCC_VALUE("LINR"), FOURCC_VALUE("LAYR"), -1,  0, STACK_REPLACE,   0 },

	{ FOURCC_VALUE("FSLG"), FOURCC_VALUE("CFHD"),  0,  1, STACK_PUSH,      0 },
	{ FOURCC_VALUE("FSLG"), FOURCC_VALUE("FSLG"), -1,  0, STACK_REPLACE,   0 },
	{ FOURCC_VALUE("FSLG"), FOURCC_VALUE("LAYR"), -1,  0, STACK_REPLACE,   0 },

	//{ FOURCC_VALUE("FSCL"), FOURCC_VALUE("FSLG"),  0,  1, STACK_PUSH,      0 },

	{ FOURCC_VALUE("LOGC"), FOURCC_VALUE("CFHD"),  0,  1, STACK_PUSH,      0 },
	{ FOURCC_VALUE("LOGC"), FOURCC_VALUE("LOGC"), -1,  0, STACK_REPLACE,   0 },
	{ FOURCC_VALUE("LOGC"), FOURCC_VALUE("LAYR"), -1,  0, STACK_REPLACE,   0 },

	// { FOURCC_VALUE("LOGt"), FOURCC_VALUE("LOGC"),  0,  1, STACK_PUSH,      0 },
	// { FOURCC_VALUE("LOGa"), FOURCC_VALUE("LOGC"),  0,  1, STACK_PUSH,      0 },
	// { FOURCC_VALUE("LOGb"), FOURCC_VALUE("LOGC"),  0,  1, STACK_PUSH,      0 },
	// { FOURCC_VALUE("LOGc"), FOURCC_VALUE("LOGC"),  0,  1, STACK_PUSH,      0 },
	// { FOURCC_VALUE("LOGd"), FOURCC_VALUE("LOGC"),  0,  1, STACK_PUSH,      0 },
	// { FOURCC_VALUE("LOGe"), FOURCC_VALUE("LOGC"),  0,  1, STACK_PUSH,      0 },
	// { FOURCC_VALUE("LOGf"), FOURCC_VALUE("LOGC"),  0,  1, STACK_PUSH,      0 },

	{ FOURCC_VALUE("PQEC"), FOURCC_VALUE("CFHD"),  0,  1, STACK_PUSH,      0 },
	{ FOURCC_VALUE("PQEC"), FOURCC_VALUE("PQEC"), -1,  0, STACK_REPLACE,   0 },
	{ FOURCC_VALUE("PQEC"), FOURCC_VALUE("LAYR"), -1,  0, STACK_REPLACE,   0 },

	{ FOURCC_VALUE("HLGE"), FOURCC_VALUE("CFHD"),  0,  1, STACK_PUSH,      0 },
	{ FOURCC_VALUE("HLGE"), FOURCC_VALUE("HLGE"), -1,  0, STACK_REPLACE,   0 },
	{ FOURCC_VALUE("HLGE"), FOURCC_VALUE("LAYR"), -1,  0, STACK_REPLACE,   0 },


	// Table entries for layer metadata

	{ FOURCC_VALUE("LAYR"), FOURCC_VALUE("CFHD"),  0,  1, STACK_PUSH,      0 },
	{ FOURCC_VALUE("LAYR"), FOURCC_VALUE("LAYR"), -1,  0, STACK_REPLACE,   0 },
	{ FOURCC_VALUE("LAYR"), FOURCC_VALUE("LOGA"), -1,  0, STACK_REPLACE,   0 },
	{ FOURCC_VALUE("LAYR"), FOURCC_VALUE("GAMA"), -1,  0, STACK_REPLACE,   0 },
	{ FOURCC_VALUE("LAYR"), FOURCC_VALUE("LINR"), -1,  0, STACK_REPLACE,   0 },
	{ FOURCC_VALUE("LAYR"), FOURCC_VALUE("FSLG"), -1,  0, STACK_REPLACE,   0 },
	{ FOURCC_VALUE("LAYR"), FOURCC_VALUE("LOGC"), -1,  0, STACK_REPLACE,   0 },
	{ FOURCC_VALUE("LAYR"), FOURCC_VALUE("PQEC"), -1,  0, STACK_REPLACE,   0 },
	{ FOURCC_VALUE("LAYR"), FOURCC_VALUE("HLGE"), -1,  0, STACK_REPLACE,   0 },

	//{ FOURCC_VALUE("LAYN"), FOURCC_VALUE("LAYR"),  0,  1, STACK_PUSH,      0 },
	//{ FOURCC_VALUE("LAYD"), FOURCC_VALUE("LAYR"),  0,  1, STACK_PUSH,      0 },
	//{ FOURCC_VALUE("LAYN"), FOURCC_VALUE("LAYD"),  0,  1, STACK_REPLACE,   0 },
	//{ FOURCC_VALUE("LAYD"), FOURCC_VALUE("LAYN"),  0,  1, STACK_REPLACE,   0 },

	//{ FOURCC_VALUE("LAYR"), FOURCC_VALUE("LAYD"),  0,  1, STACK_REPLACE,   1 },
	//{ FOURCC_VALUE("LAYR"), FOURCC_VALUE("LAYN"),  0,  1, STACK_REPLACE,   1 },

	//{ FOURCC_VALUE("LAYR"), FOURCC_VALUE("GAMA"),  0,  1, STACK_REPLACE,   1 },


	// Table entries for streaming metadata

	{ FOURCC_VALUE("DEVC"), FOURCC_VALUE("GPMF"),  0,  1, STACK_PUSH,      0 },
	{ FOURCC_VALUE("STRM"), FOURCC_VALUE("DEVC"),  0,  1, STACK_PUSH,      0 },
	{ FOURCC_VALUE("STRM"), FOURCC_VALUE("STRM"), -1,  0, STACK_REPLACE,   0 },
	{ FOURCC_VALUE("TICK"), FOURCC_VALUE("STRM"), -1,  0, STACK_POP,       1 },
	{ FOURCC_VALUE("STRM"), FOURCC_VALUE("TICK"), -1,  0, STACK_UNCHANGED, 0 },
	{ FOURCC_VALUE("DEVC"), FOURCC_VALUE("STRM"), -2, -1, STACK_REPLACE,   1 },

	//TODO: List more combinations for streaming metadata

	//TODO: List combinations for extrinsic and dark metadata

#endif

};

static const size_t nested_tag_table_length = sizeof(nested_tag_table)/sizeof(nested_tag_table[0]);

//! Metadata tuple that contains the tuples currently being processed
static FOURCC current_nested_tuple_tag = 0;

//TODO: Use the tag of the top node in the XML node stack instead of the variable current_nested_tuple_tag


//! Current metadata class instance
static FOURCC current_metadata_class_instance = 0;


//! Nexting level of the metadata class instances
static int metadata_class_instance_nesting_level = 0;


//! Nesting level of the metadata chunk element
static int chunk_element_nesting_level = 0;


//! Stack of XML nodes in the metadata tuple hierarchy
static mxml_node_t *xml_node_stack[MAXIMUM_XML_NODE_STACK_DEPTH];


//! Current depth of the XML node stack
static int xml_node_stack_depth = 0;


//! Dump node information if the debug flag is true
static bool dump_node_info_flag = false;


/*!
	@brief Mini-XML whitespace callback

	Do not add whitespace before the XML header
*/
const char *
whitespace_cb(mxml_node_t *node, int where)
{
	static char string[1024];
	int level = -1;

	const char *element = mxmlGetElement(node);
	//fprintf(stderr, "Element: %s\n", element);

	// Add white space before and after elements
	if (where == MXML_WS_BEFORE_OPEN)
	{
		if (strcmp(element, "metadata") == 0)
		{
			level = 0;
		}
		if (strcmp(element, "chunk") == 0)
		{
			level = 1;
		}
		else if (strcmp(element, "tuple") == 0)
		{
			//const char *tag = mxmlElementGetAttr(node, "tag");
			const char *type = mxmlElementGetAttr(node, "type");

			if (type != NULL && strcmp(type, "E") == 0) {
				level = 2;
			}
			else {
				level = 3;
			}
		}

		if (level >= 0)
		{
			snprintf(string, sizeof(string), "\n%s", Indentation(level));
			return string;
		}
	}

	if (where == MXML_WS_AFTER_CLOSE)
	{
	      return "\n";
	}

	return NULL;
}


/*!
	@brief Update the current nesting level
*/
CODEC_ERROR UpdateNestingLevel(TUPLE_TAG tuple_tag, TUPLE_TYPE tuple_type, int *current_level, int *next_level)
{
	int level = *current_level;

	// Is this a metadata class instance?
	if (tuple_type == 'E')
	{
		// Compute the current and next nesting levels relative to the chunk element
		metadata_class_instance_nesting_level = chunk_element_nesting_level + 1;
		*current_level = metadata_class_instance_nesting_level;
		*next_level = metadata_class_instance_nesting_level + 1;

		current_metadata_class_instance = tuple_tag;

		current_nested_tuple_tag = tuple_tag;

		return CODEC_ERROR_OKAY;
	}

	//if (current_metadata_class_instance == FOURCC_VALUE("CFHD"))
	{
		// All instrinsic metadata tuples are defined in SMPTE ST 2073-7 and that standard specifies the nesting level

		// Find the tuple tag in the table for intrinsic metadata
		for (int i = 0; i < nested_tag_table_length; i++)
		{
			// Is this tuple a nested tuple?
			if (tuple_tag == nested_tag_table[i].tuple_tag &&
				current_nested_tuple_tag == nested_tag_table[i].nested_tag)
			{
				*current_level = level + nested_tag_table[i].current_level_offset;
				*next_level = level + nested_tag_table[i].next_level_offset;
				current_nested_tuple_tag = tuple_tag;
				break;
			}
		}

		// The current and next nesting levels are unchanged
	}

	return CODEC_ERROR_OKAY;
}


/*!
	@brief Return the top node in the XML node stack
*/
mxml_node_t *TopNode()
{
	if (xml_node_stack_depth > 0) {
		return xml_node_stack[xml_node_stack_depth - 1];
	}

	return NULL;
}


/*!
	@brief Return the tuple tag for the top node in the XML node stack

	The function returns zero if the top node is not a metadata tuple or the stack is empty
*/
TUPLE_TAG TopNodeTag()
{
	mxml_node_t *top_node = TopNode();
	assert(top_node != NULL);
	if (! (top_node != NULL)) {
		return 0;
	}

	const char *tag = mxmlElementGetAttr(top_node, "tag");

	return FOURCC_VALUE(tag);
}

/*!
	@brief Push a new XML node onto the stack
*/
CODEC_ERROR PushNode(mxml_node_t *node)
{
	assert(xml_node_stack_depth < MAXIMUM_XML_NODE_STACK_DEPTH);
	if (! (xml_node_stack_depth < MAXIMUM_XML_NODE_STACK_DEPTH)) {
		return CODEC_ERROR_STACK_ERROR;
	}

	xml_node_stack[xml_node_stack_depth++] = node;

	return CODEC_ERROR_OKAY;
}


/*!
	@brief Pop the top XML node from the stack
*/
mxml_node_t *PopNode()
{
	mxml_node_t *node = NULL;

	if (xml_node_stack_depth > 0) {
		node = TopNode();
		xml_node_stack_depth--;
	}

	return node;
}


/*!
	@brief Reset the XML node stack
*/
CODEC_ERROR ResetStack()
{
	 xml_node_stack_depth = 0;
	 return CODEC_ERROR_OKAY;
}


/*!
	@brief Print the contents of the XML node stack
*/
void DumpNodeStack()
{
	fprintf(stderr, "xml_node_stack_depth: %d\n", xml_node_stack_depth);

	for (int i = xml_node_stack_depth - 1; i >= 0; i--)
	{
		mxml_node_t *node = xml_node_stack[i];
		const char *element = mxmlGetElement(node);
		const char *tag = mxmlElementGetAttr(node, "tag");

		if (strcmp(element, "chunk") == 0)
		{
			fprintf(stderr, "0x%04lX", strtol(tag, NULL, 0));
		}
		else
		{
			fprintf(stderr, "%s\n", tag);
		}
	}

	fprintf(stderr, "\n");
}


/*!
	@brief Print information about the XML node
*/
void DumpNodeInfo(mxml_node_t *node, const char *label)
{
	const char *element = mxmlGetElement(node);
	const char *tag = mxmlElementGetAttr(node, "tag");
	fprintf(stderr, "%s: %s, tag: %s\n", label, element, tag);
}


/*!
	@brief Return the FOURCC for the tag attribute in the node
*/
static TUPLE_TAG GetNodeTag(mxml_node_t *node)
{
	if (node != NULL)
	{
		const char *string = mxmlElementGetAttr(node, "tag");
		return TupleTag(string);
	}

	return 0;
}


/*!
	@brief Create a new XML node with the correct nesting in the tuple hierarchy
*/
mxml_node_t *NewTupleNode(TUPLE_TAG new_tag)
{
	TUPLE_TAG top_tag = TopNodeTag();
	assert(top_tag != 0);
	if (! (top_tag != 0)) {
		return NULL;
	}

	// New node created by this routine
	mxml_node_t *new_node = NULL;

	// The default operation is no change if the new tag and top tag are not found in the nested tag table
	int stack_change = STACK_UNCHANGED;
	int stack_pop = 0;

	if (debug_flag && dump_node_info_flag) DumpNodeInfo(TopNode(), "Top node");

	// Determine where the new node fits in the metadata tuple hierarchy
	for (int i = 0; i < nested_tag_table_length; i++)
	{
		// Is this tuple a nested tuple?
		if (new_tag == nested_tag_table[i].tuple_tag &&
			top_tag == nested_tag_table[i].nested_tag)
		{
			if (debug_flag && dump_node_info_flag) {
				fprintf(stderr, "Found table entry: %c%c%c%c %c%c%c%c\n", FOURCC_CHARS(new_tag), FOURCC_CHARS(top_tag));
			}
			stack_change = nested_tag_table[i].stack_change;
			stack_pop = nested_tag_table[i].stack_pop;
			break;
		}
	}

	if (debug_flag && dump_node_info_flag) fprintf(stderr, "Stack change: %d, pop count: %d\n", stack_change, stack_pop);

	// Pop the specified number of nodes from the stack
	for (int count = stack_pop; count > 0; count--) {
		PopNode();
	}

	// Initialize the parent node to the top node in the stack before any changes are applied to the stack
	mxml_node_t *parent_node = TopNode();

	switch (stack_change)
	{
		case STACK_UNCHANGED:
			parent_node = TopNode();
			new_node = mxmlNewElement(parent_node, "tuple");
			mxmlElementSetAttrf(new_node, "tag", "%c%c%c%c", FOURCC_CHARS(new_tag));
			break;

		case STACK_PUSH:
			new_node = mxmlNewElement(parent_node, "tuple");
			mxmlElementSetAttrf(new_node, "tag", "%c%c%c%c", FOURCC_CHARS(new_tag));
			PushNode(new_node);
			parent_node = new_node;
			break;

		case STACK_POP:
			// The stack pop operation was performed before this switch statement
			new_node = mxmlNewElement(parent_node, "tuple");
			mxmlElementSetAttrf(new_node, "tag", "%c%c%c%c", FOURCC_CHARS(new_tag));
			break;

		case STACK_REPLACE:
			PopNode();
			parent_node = TopNode();
			new_node = mxmlNewElement(parent_node, "tuple");
			mxmlElementSetAttrf(new_node, "tag", "%c%c%c%c", FOURCC_CHARS(new_tag));
			PushNode(new_node);
			parent_node = new_node;
			break;

		default:
			assert(0);
			fprintf(stderr, "Unknown stack change: %d\n", stack_change);
			break;
	}

	assert(parent_node != NULL);
	if (! (parent_node != NULL)) {
		return NULL;

		// Could use the current class instance
	}

	assert(new_node != NULL);
	if (! (new_node != NULL)) {
		return NULL;
	}

	if (debug_flag && dump_node_info_flag) DumpNodeInfo(new_node, "New node");

	if (debug_flag && dump_node_info_flag) DumpNodeInfo(parent_node, "Parent node");

	return new_node;
}


/*!
	@brief Remove all earlier tuples with the same tag and class from all chunks
*/
static CODEC_ERROR RemoveDuplicateTuples(mxml_node_t *chunk, mxml_node_t *class, mxml_node_t *tuple)
{
	CODEC_ERROR error = CODEC_ERROR_OKAY;

	assert(chunk != NULL && class != NULL && tuple != NULL);

	// Get the tags for the class instance and the tuple
	TUPLE_TAG class_tag = GetNodeTag(class);
	TUPLE_TAG tuple_tag = GetNodeTag(tuple);

	// Streaming data can have duplicate tuples for a series of measurements
	if (class_tag == FOURCC_VALUE("GPMF")) {
		return CODEC_ERROR_OKAY;
	}

	// There can be more than one layer metadata item (one for each layer)
	if (tuple_tag == FOURCC_VALUE("LAYR")) {
		return CODEC_ERROR_OKAY;
	}

	//TODO: Need to remove earlier layer metadata items that refer to the same layer number

	// Start the search for duplicates at the next earlier tuple
	tuple = mxmlGetPrevSibling(tuple);

	// Search all chunks for an earlier tuple with the same tag
	while (chunk != NULL)
	{
		// Search this class instance for an earlier tuple with the same tag
		while (class != NULL)
		{
			if (GetNodeTag(class) == class_tag)
			{
				while (tuple != NULL)
				{
					if (GetNodeTag(tuple) == tuple_tag) {
						printf("%sDuplicate: %c%c%c%c\n", Indentation(3), FOURCC_CHARS(tuple_tag));
						mxml_node_t *prev_tuple = mxmlGetPrevSibling(tuple);
						mxmlRemove(tuple);
						tuple = prev_tuple;
					}
					else
					{
						tuple = mxmlGetPrevSibling(tuple);
					}
				}
			}

			// Search earlier class instances for a tuple with the same tag
			class = mxmlGetPrevSibling(class);
			if (class != NULL) {
				// Resume the search with the last tuple in the earlier class instance
				tuple = mxmlGetLastChild(class);
			}
		}

		chunk = mxmlGetPrevSibling(chunk);

		// Search ealier metadata chunks for a tuple with the same tag
		if (chunk != NULL) {
			// Resume the search with the last class instance in the chunk
			class = mxmlGetLastChild(chunk);
		}

		if (class != NULL) {
			// Resume the search with the last tuple in the earlier class instance
			tuple = mxmlGetLastChild(class);
		}
	}

	return error;
}


/*!
	@brief Prune duplicate tuples from the XML tree
*/
CODEC_ERROR PruneDuplicateTuples(mxml_node_t *root, ARGUMENTS *args)
{
	CODEC_ERROR error = CODEC_ERROR_OKAY;

	//bool verbose_flag = (args != NULL) ? args->verbose_flag : false;
	//bool debug_flag = (args != NULL) ? args->debug_flag : false;

	// Force debug output from this routine (for debugging)
	bool debug_flag = true;

	// Use the global debug flag
	//bool debug_flag = args->debug_flag;

	// Get the top-level metaadata node
	mxml_node_t *metadata = mxmlGetFirstChild(root);
	if (debug_flag) {
		//const char *name = mxmlGetElement(metadata);
		//printf("Metadata: %s\n", name);
	}

	// Process each metadata chunk in reverse order
	mxml_node_t *chunk = mxmlGetLastChild(metadata);
	while (chunk != NULL)
	{
		if (debug_flag) {
			const char *tag = mxmlElementGetAttr(chunk, "tag");
			const char *size = mxmlElementGetAttr(chunk, "size");
			printf("Chunk tag: %s, size: %s\n", tag, size);
		}

		// Process each metadata class instances in the chunk
		mxml_node_t *class = mxmlGetLastChild(chunk);
		while (class != NULL)
		{
			if (debug_flag) {
				const char *tag = mxmlElementGetAttr(class, "tag");
				const char *size = mxmlElementGetAttr(class, "size");
				printf("%sClass tag: %s, size: %s\n", Indentation(1), tag, size);
			}

			mxml_node_t *tuple = mxmlGetLastChild(class);
			while (tuple != NULL)
			{
				if (debug_flag) {
					const char *tag = mxmlElementGetAttr(tuple, "tag");
					printf("%sTuple tag: %s\n", Indentation(2), tag);
				}

				// Remove earlier instances of this tuple from this or earlier class instances
				error = RemoveDuplicateTuples(chunk, class, tuple);
				assert(error == CODEC_ERROR_OKAY);
				if (! (error == CODEC_ERROR_OKAY)) {
					return error;
				}

				tuple = mxmlGetPrevSibling(tuple);
			}

			class = mxmlGetPrevSibling(class);
		}

		chunk = mxmlGetPrevSibling(chunk);
	}

	return error;
}
