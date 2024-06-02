/*!	@file nodetree.h

	Definitions for the routines that create an XML tree from the binary metadata
*/

#ifndef _NODETREE_H
#define _NODETREE_H


//! Forward reference
//typedef struct _arguments ARGUMENTS;

//! Mini-XML whitespace callback
const char *whitespace_cb(mxml_node_t *node, int where);

//! Update the current nesting level
CODEC_ERROR UpdateNestingLevel(TUPLE_TAG tuple_tag, TUPLE_TYPE tuple_type, int *current_level, int *next_level);

//! Push a new XML node onto the stack
CODEC_ERROR PushNode(mxml_node_t *node);

//! brief Pop the top XML node from the stack
mxml_node_t *PopNode();

//! Reset the XML node stack
CODEC_ERROR ResetStack();

//! Create a new XML node with the correct nesting in the tuple hierarchy
mxml_node_t *NewTupleNode(TUPLE_TAG new_tag);

//! Prune duplicate tuples from the XML tree
CODEC_ERROR PruneDuplicateTuples(mxml_node_t *root, ARGUMENTS *args);


#if _TESTING

//! Write the tuple value to the output file
CODEC_ERROR DumpTupleValue(const void *payload, const size_t payload_size, const TUPLE_HEADER *tuple_header,
	const int level, mxml_node_t *node, const ARGUMENTS *args);

#endif

#endif
