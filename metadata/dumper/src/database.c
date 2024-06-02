/*!	@file database.c

	@brief Implementation of the metadata database

	The database routines used by the XML dumper are a proxy for an actual metadata database
	that is created as tuples are read from the metadata chunk elements. Duplicate tuples are
	removed as the same tuples are inserted into the database.

	An application may want to use an actual database to manage tuples and for more efficient
	retrieval of metadata. This proxy uses the XML tree created by Mini-XML as a proxy for an
	actual database since the XML dumper only needs to output the metadata in XML format and
	duplicate tuples can be removed after the XML tree is created. In other words, the XML tree
	is a suitable database for the goals of the XML dumper.

	If the _DATABASE switch is set, then the XML dumper can be used as a testbed for developing
	a metadata database for applications that do not want to use the Mini-XML tree as a proxy
	for a database.
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
#include "../../common/include/buffer.h"
#include "../../common/include/metadata.h"

#include "mxml.h"

#include "../include/params.h"
#include "../include/nodetree.h"
#include "../include/database.h"
#include "../include/dumper.h"

#else

#include "config.h"
#include "error.h"
#include "swap.h"
#include "base64.h"
#include "buffer.h"
#include "params.h"
#include "metadata.h"

#include "mxml.h"

#include "nodetree.h"
#include "database.h"
#include "dumper.h"

#endif


//! Global flag that controls debug output
extern bool debug_flag;


//! Log file for reporting errors detected in the XML test cases
extern FILE *logfile;


#if _DATABASE

#if _DECODER

/*!
	@brief Data structure for the database proxy used by the decoder
*/
typedef struct _database
{
	mxml_node_t *root;			//!< Root of the XML tree
	mxml_node_t *metadata;		//!< Top-level metadata element
	mxml_node_t *chunk;			//!< Current metadata chunk element
	mxml_node_t *class;			//!< Current metadata class instance
	mxml_node_t *tuple;			//!< Most recent metadata tuple

	int current_level;			//!< Current nesting level in the metadata tuple hierarchy
	int next_level;				//!< Nesting level for metadata tuples that follow the current tuple

	bool verbose_flag;			//!< Control verbose output
	bool debug_flag;			//!< Control extra output for debugging
	bool duplicates_flag;		//!< Control removal of duplicate tuples		

} DATABASE;

#else

#include "uthash.h"

/*!
	@brief Data structure for the database entries
*/
typedef struct _tuple_record
{
	TUPLE_HEADER header;
	TUPLE_PAYLOAD *payload;
	UT_hash_handle hh;

} TUPLE_RECORD;

/*!
	@brief Data structure for the metadata database
*/
typedef struct _database
{
	TUPLE_RECORD *tuple_table;

} DATABASE;

#endif


/*!
	@brief Initialize the database with the root and metadata nodes
*/
static CODEC_ERROR InitializeMetadataProcessing(DATABASE *database)
{
	CODEC_ERROR error = CODEC_ERROR_OKAY;

	assert(database != NULL);
	if (! (database != NULL)) {
		return CODEC_ERROR_UNEXPECTED;
	}

    // database->verbose_flag = verbose_flag;
    // database->debug_flag = debug_flag;

    // Initialize the Mini-XML writer
    database->root = mxmlNewXML("1.0");
    assert(database->root != NULL);

    // The XML version tuple is added automatically with UTF-8 encoding

    // Create the root metadata element
    mxml_node_t *metadata = mxmlNewElement(database->root, "metadata");
    mxmlElementSetAttr(metadata, "xmlns", "https://www.vc5codec.org/xml/metadata");
    database->metadata = metadata;

    // Define nodes for the current chunk elemenet and metadata tuples
    //mxml_node_t *chunk = NULL;
    //mxml_node_t *class = NULL;

    // Initialize the current and next levels in the metadata hierarchy
    database->current_level = 0;
    database->next_level = 0;

    return error;
}


/*
	@brief Create the metadata database for the hierarchy of metadata chunks and tuples read from the bitstream
*/
CODEC_ERROR CreateMetadataDatabase(DATABASE **database_out, bool verbose_flag, bool debug_flag, bool duplicates_flag)
{
	CODEC_ERROR error = CODEC_ERROR_OKAY;

	// bool verbose_flag = (args != NULL) ? args->verbose_flag : false;
	// bool debug_flag = (args != NULL) ? args->debug_flag : false;

	// Suppress compiler warnings about unused variables
	// (void)verbose_flag;
	// (void)debug_flag;

	// Allocate and initialize a new metadata database
	assert(database_out != NULL && *database_out == NULL);
	if (! (database_out != NULL && *database_out == NULL)) {
		return CODEC_ERROR_BAD_INPUT;
	}

	DATABASE *database = malloc(sizeof(DATABASE));
	if (database == NULL) {
		return CODEC_ERROR_ALLOC_FAILED;
	}

	memset(database, 0, sizeof(DATABASE));

	// if (args != NULL)
	// {
		database->verbose_flag = verbose_flag;
		database->debug_flag = debug_flag;
		database->duplicates_flag = duplicates_flag;
	// }

	error = InitializeMetadataProcessing(database);
	if (error != CODEC_ERROR_OKAY) {
		return error;
	}

	*database_out = database;

	return error;
}


/*
	@brief Delete the metadata database
*/
CODEC_ERROR DestroyMetadataDatabase(DATABASE *database)
{
	CODEC_ERROR error = CODEC_ERROR_OKAY;

	if (database->root != NULL)
	{
		// Delete the XML tree and remove all XML nodes from the database
		mxmlDelete(database->root);
		ClearDatabaseEntries(database);
	}

	free(database);

	return error;
}


#if _DECODER

/**** This section of code implements the proxy database used by the decoder ****/

/*
	@brief Insert a new metadata chunk element into the database
*/
CODEC_ERROR InsertDatabaseChunk(DATABASE *database, uint16_t chunk_tag, uint32_t chunk_size)
{
	CODEC_ERROR error = CODEC_ERROR_OKAY;

	int current_level = database->current_level;

	//printf("Chunk tag: 0x%04X, size: %d\n", chunk_tag, chunk_size);

	if (database->debug_flag) {
		fprintf(stderr, "Chunk tag: 0x%0X, size: %d\n", chunk_tag, chunk_size);
	}
	else if (database->verbose_flag)
	{
		if (chunk_tag == METADATA_CHUNK_LARGE) {
			printf("%sChunk tag: 0x%02X, value: 0x%06X (%d)\n", Indentation(current_level), chunk_tag, chunk_size, chunk_size);
		} else {
			printf("%sChunk tag: 0x%04X, value: 0x%04X (%d)\n", Indentation(current_level), chunk_tag, chunk_size, chunk_size);
		}
	}

	// Process the chunk payload at the top level in the tuple hierarchy
	database->current_level = 0;

	// Create the node for the chunk
	mxml_node_t *chunk = mxmlNewElement(database->metadata, "chunk");

	// Set the chunk tag and size attributes
	uint16_t output_tag = (chunk_tag == 0x6100) ? (chunk_tag >> 8) : chunk_tag;
	mxmlElementSetAttrf(chunk, "tag", (output_tag == 0x61) ? "0x%2X" : "0x%04X", output_tag);
	mxmlElementSetAttrf(chunk, "size", "%d", chunk_size);

	// The chunk node should be the first node on the XML node stack
	ResetStack();

	// Push the node for the chunk element onto the node stack
	PushNode(chunk);

	// Process the chunk payload at the next level in the tuple hierarchy
	database->current_level++;

	// Remember the current chunk element
	database->chunk = chunk;

	return error;
}


/*
	@brief Insert a new metadata class instance into the current chunk element
*/
CODEC_ERROR InsertDatabaseClass(DATABASE *database, TUPLE_HEADER *tuple_header)
{
	CODEC_ERROR error = CODEC_ERROR_OKAY;

	// if (database->verbose_flag)
	// {
	// 	printf("Class tag: %c%c%c%c, type: %d, size: %d, padding: %d\n",
	// 		FOURCC_CHARS(tuple_header->tag), tuple_header->type, tuple_header->size, tuple_header->padding);
	// }

	mxml_node_t *class = mxmlNewElement(database->chunk, "tuple");
	mxmlElementSetAttrf(class, "tag", "%c%c%c%c", FOURCC_CHARS(tuple_header->tag));
	mxmlElementSetAttrf(class, "type", "%c", tuple_header->type);
	mxmlElementSetAttrf(class, "size", "%u", tuple_header->size);

	// The repeat count in a class instance is always zero
	assert(tuple_header->count == 0);
	//mxmlElementSetAttrf(class, "count", "%u", tuple_header->count);	

	// Always output the padding even if it is zero
	mxmlElementSetAttrf(class, "padding", "%u", tuple_header->padding);

	PushNode(class);

	// Remember the current class instance
	database->class = class;

	return error;
}


/*
	@brief Insert a new metadata tuple into the current metadata class
*/
CODEC_ERROR InsertDatabaseTuple(DATABASE *database, TUPLE *metadata_tuple)
{
	CODEC_ERROR error = CODEC_ERROR_OKAY;

	assert(metadata_tuple != NULL);

	TUPLE_HEADER *tuple_header = &metadata_tuple->header;
	//void *tuple_payload = metadata_tuple->payload;

	assert(HasRepeatCount(tuple_header->type) || tuple_header->count == 0);

	// if (database->verbose_flag)
	// {
	// 	printf("Tuple tag: %c%c%c%c, type: %c, size: %d, count: %d, padding: %d\n",
	// 		FOURCC_CHARS(tuple_header->tag), tuple_header->type, tuple_header->size, tuple_header->count, tuple_header->padding);
	// }

	// Create the XML element for the metadata tuple
	//mxml_node_t *tuple = mxmlNewElement(class, "tuple");
	mxml_node_t *tuple = NewTupleNode(tuple_header->tag);
	assert(tuple != NULL);
	if (! (tuple != NULL)) {
		return CODEC_ERROR_BAD_TUPLE;
	}

	// The tuple tag attributes should have been set when the XML node was created
	assert(mxmlElementGetAttr(tuple, "tag") != NULL);

	// Add the tag attribute if it is not already set
	if (mxmlElementGetAttr(tuple, "tag") == NULL) {
		mxmlElementSetAttrf(tuple, "tag", "%c%c%c%c", FOURCC_CHARS(tuple_header->tag));
	}

	// Always output the tuple type and size
	mxmlElementSetAttrf(tuple, "type", "%c", (tuple_header->type == 0) ? '0' : tuple_header->type);
	mxmlElementSetAttrf(tuple, "size", "%u", tuple_header->size);

#if 1
	// Output the count if the tuple has a repeat count
	if (HasRepeatCount(tuple_header->type)) {
		mxmlElementSetAttrf(tuple, "count", "%u", tuple_header->count);
	}
#else
	// Always output the count (tuples without a repeat count have count set to zero)
	mxmlElementSetAttrf(tuple, "count", "%u", tuple_header->count);
#endif

#if _DEBUG
	size_t total_size = ((tuple_header->count > 0) ? tuple_header->count : 1) * tuple_header->size;
	//fprintf(stderr, "%zd\n", total_size);

	// Tuple must have a payload unless it is a nested tuple or an encoding curve
	assert(tuple_header->type == 0 || tuple_header->type == 'P' || total_size > 0);
#endif

	// size_t segment_count = (total_size + 3) / 4;
	// size_t payload_size = 4 * segment_count;
	// size_t padding = payload_size - total_size;

	// Output the tuple value unless this is a nested tuple
	if (! IsNestedTuple(tuple_header->type))
	{

		//DumpTupleValue(payload, sizeof(payload), tuple_tag, type, size, count, current_level, tuple, args);
		//TUPLE_HEADER tuple_header = {tuple_tag, type, size, count, padding};
		DumpTupleValue(metadata_tuple->payload, metadata_tuple->payload_size, tuple_header, database->current_level, tuple, NULL);
	}

	// Always output the padding even if it is zero
	mxmlElementSetAttrf(tuple, "padding", "%u", tuple_header->padding);

	// if (database->debug_flag) {
	// 	fprintf(stderr, "Tuple: %c%c%c%c, type: %c, size: %d, count: %d, padding: %d, payload_size: %zd\n",
	// 		FOURCC_CHARS(tuple_header->tag), tuple_header->type, tuple_header->size, tuple_header->count, tuple_header->padding, metadata_tuple->payload_size);
	// }
	//DumpNodeInfo(tuple, "New node");

	// Remember the current metadata tuple
	database->tuple = tuple;

	return error;
}


/*
	@brief Write the metadata database to the output file in XML format
*/
CODEC_ERROR OutputMetadataDatabase(DATABASE *database, FILE *output_file)
{
	//if (database->debug_flag) printf("OutputMetadataDatabase database: %p, output_file: %p\n", database, output_file);
	
	// Not an error if the database was not created
	if (database == NULL) {
		return CODEC_ERROR_UNEXPECTED;
	}

	// if (database->debug_flag) {
	// 	printf("Database root: %p, metadata: %p\n", database->root, database->metadata);
	// }

	// Write the XML tree to the output file without text wrapping
	mxmlSetWrapMargin(0);
	int mxml_result = mxmlSaveFile(database->root, output_file, whitespace_cb);

	//printf("mxml_result: %d\n", mxml_result);

	if (database->debug_flag) {
		fprintf(stderr, "Saved XML file result: %d\n", mxml_result);
	}

	return (mxml_result == 0) ? CODEC_ERROR_OKAY : CODEC_ERROR_FILE_WRITE;
}


/*!
	@brief Clear all pointers in the database to XML nodes

	This routine assumes that the XML tree has already been deleted.
*/
CODEC_ERROR ClearDatabaseEntries(DATABASE *database)
{
	if (database != NULL)
	{
		database->root = NULL;
		database->metadata = NULL;
		database->chunk = NULL;
		database->class = NULL;
		database->tuple = NULL;
	}

	return CODEC_ERROR_OKAY;
}


/*!
	@brief Update the level in the metadata hierarchy
*/
CODEC_ERROR UpdateDatabaseLevel(DATABASE *database, TUPLE_TAG tuple_tag, TUPLE_TYPE tuple_type)
{
	return UpdateNestingLevel(tuple_tag, tuple_type, &database->current_level, &database->next_level);
}


/*!
	@brief Set the current level in the hierarchy to the next level
*/
CODEC_ERROR SetDatabaseNextLevel(DATABASE *database)
{
	database->current_level = database->next_level;
	return CODEC_ERROR_OKAY;
}


/*!
	@brief Return the indentation for the current level in the metadata hierarchy
*/
const char *CurrentLevelIndentation(DATABASE *database)
{
	return Indentation(database->current_level);
}


/*!
	@brief Remove duplicate tuples from the database
*/
CODEC_ERROR PruneDatabaseDuplicateTuples(DATABASE *database)
{
	assert(database != NULL && database->root != NULL);

	if (database->duplicates_flag) {
		return PruneDuplicateTuples(database->root, NULL);
	}

	return CODEC_ERROR_OKAY;
}

/*!
	@brief Set the flags that control output to the terminal
*/
CODEC_ERROR SetDatabaseFlags(DATABASE *database, bool verbose_flag, bool debug_flag)
{
	if (database != NULL)
	{
		database->verbose_flag = verbose_flag;
		database->debug_flag = debug_flag;
		return CODEC_ERROR_OKAY;
	}

	return CODEC_ERROR_UNEXPECTED;
}


#if _TESTING

/*!
	@brief Scaffolding for testing the code used by the reference decoder

	The reference decoder calls the routines in this file after finding a metadata chunk element,
	starting with the routine @ref InsertDatabaseChunk and continuing with class instances in the
	chunk and the hierarchy of tuples in each class instance.

	This code builds an XML tree since that is the result needed to verify comformance with ST 2073-7,
	but the naming conventions used in this code contemplate replacing the XML tree with an actual database.
	See the implementation of @ref ReadBinaryMetadata for an example of a database for intrinsic metadata.
*/
CODEC_ERROR DecodeBinaryMetadata(FILE *input_file, FILE *output_file, DATABASE *database, ARGUMENTS *args)
{
	CODEC_ERROR error = CODEC_ERROR_OKAY;

	assert(input_file != NULL);
	assert(output_file != NULL);
	assert(database != NULL);

	bool verbose_flag = (args != NULL) ? args->verbose_flag : true;
	bool debug_flag = (args != NULL) ? args->debug_flag : true;

	SetDatabaseFlags(database, verbose_flag, debug_flag);

	//int mxml_result = 0;
	int file_result = 1;

#if 0
	// Initialize the Mini-XML writer
	database->root = mxmlNewXML("1.0");

	// The XML version tuple is added automatically with UTF-8 encoding

	// Create the root metadata element
	mxml_node_t *metadata = mxmlNewElement(database->root, "metadata");
	mxmlElementSetAttr(metadata, "xmlns", "https://www.vc5codec.org/xml/metadata");
	database->metadata = metadata;

	// Define nodes for the current chunk elemenet and metadata tuples
	//mxml_node_t *chunk = NULL;
	//mxml_node_t *class = NULL;

	// Initialize the current and next levels in the metadata hierarchy
	database->current_level = 0;
	database->next_level = 0;
#else
	// error = InitMetadataProcessing(database);
	// if (error != CODEC_ERROR_OKAY) {
	// 	return error;
	// }
#endif

	// Read the chunk header
	SEGMENT chunk_header;
	file_result = fread(&chunk_header, sizeof(chunk_header), 1, input_file);
	if (file_result != 1) {
		return CODEC_ERROR_FILE_READ;
	}

	uint16_t chunk_tag = 0;
	uint32_t chunk_size = 0;

	if (! ParseChunkHeader(chunk_header, &chunk_tag, &chunk_size, false)) {
		return CODEC_ERROR_BAD_CHUNK;
	}

	error = InsertDatabaseChunk(database, chunk_tag, chunk_size);
	if (error != CODEC_ERROR_OKAY) {
		return error;
	}

	// Push the node for the chunk element onto the node stack
	//PushNode(database->chunk);

	// Process the chunk payload at the next level in the tuple hierarchy
	//current_level++;

	// Read metadata tuples from the input file until end of file
	while (file_result == 1)
	{
		FOURCC tuple_tag;
		file_result = fread(&tuple_tag, sizeof(tuple_tag), 1, input_file);
		if (file_result != 1) break;

		// Is this tuple another metadata chunk element?
		SEGMENT chunk_header = (SEGMENT)tuple_tag;

		if (ParseChunkHeader(chunk_header, &chunk_tag, &chunk_size, false))
		{
			// Process the chunk payload at the top level in the tuple hierarchy
			//database->current_level = 0;

			error = InsertDatabaseChunk(database, chunk_tag, chunk_size);
			if (error != CODEC_ERROR_OKAY) {
				return error;
			}

			// The chunk node should be the first node on the XML node stack
			// ResetStack();

			// Push the node for the chunk element onto the node stack
			// PushNode(database->chunk);

			// Process the chunk payload at the next level in the tuple hierarchy
			// database->current_level++;

			continue;
		}

		// Read the next segment containing the data type and the tuple size and repeat count
		SEGMENT type_size_count;
		file_result = fread(&type_size_count, sizeof(type_size_count), 1, input_file);
		if (file_result != 1) break;

		// Swap the segment into network (big endian) order
		type_size_count = Swap32(type_size_count);

		// The first byte is the tuple data type
		char type = (type_size_count >> 24);
		size_t count;
		size_t size;

		if (HasRepeatCount(type))
		{
			// One byte size and two byte count and two byte repeast count
			size = (type_size_count >> 16) & 0xFF;
			count = (type_size_count & 0xFFFF);
		}
		else
		{
			// Three byte size with no repeat count
			size = type_size_count & 0xFFFFFF;
			count = 0;
		}

		// Update the nesting level based on the new tuple tag and the current nested tag and level
		UpdateNestingLevel(tuple_tag, type, &database->current_level, &database->next_level);

		if (verbose_flag)
		{
			printf("%sTuple tag: %c%c%c%c, type: %c, size: %zu, count: %zu\n",
				Indentation(database->current_level), FOURCC_CHARS(tuple_tag), type, size, count);
		}

		//if (tuple_tag == TupleTag("CFHD"))
		if (IsClassInstance(tuple_tag, type))
		{
			size_t padding = TuplePadding(size, 0);

			TUPLE_HEADER tuple_header = {tuple_tag, type, size, 0, padding};
			error = InsertDatabaseClass(database, &tuple_header);
			if (error != CODEC_ERROR_OKAY) {
				return error;
			}

			//TODO: Check that the current node at the top of the stack is a metadata chunk element

			// Push the node for the metadata class instance onto the node stack
			// PushNode(class);
		}
		else
		{
			// if (debug_flag && (tuple_tag == FOURCC_VALUE("GYRO") || tuple_tag == FOURCC_VALUE("LAYD")))
			// {
			// 	fprintf(stderr, "Tuple: %c%c%c%c, size: %zd, count: %zd, total_size: %zd, padding: %zd, payload_size: %zd\n",
			// 		FOURCC_CHARS(tuple_tag), size, count, total_size, padding, payload_size);
			// }

			// Round up the size to a segment boundary
			// size_t segment_count = (size + 3)/4;
			// size_t padding = 4 * segment_count - size;

			size_t total_size = ((count > 0) ? count : 1) * size;
			//fprintf(stderr, "%zd\n", total_size);

			size_t segment_count = (total_size + 3) / 4;
			size_t payload_size = 4 * segment_count;
			size_t padding = payload_size - total_size;

			// Define the tuple header (set the payload after the payload is read from the file)
			TUPLE tuple = {
				{tuple_tag, type, size, count, padding},
				NULL,
				0
			};

			// Tuple must have a payload unless it is a nested tuple or an encoding curve
			assert(tuple.header.type == 0 || tuple.header.type == 'P' || total_size > 0);

			// Output the tuple value unless this is a nested tuple
			if (! IsNestedTuple(tuple.header.type))
			{
				// Read the payload (metadata value and padding)
				//size_t padding = sizeof(SEGMENT) - (total_size % sizeof(SEGMENT));
				//if (padding == sizeof(SEGMENT)) padding = 0;
				//size_t payload_size = total_size + padding;
				ALLOC_BUFFER(payload, payload_size);
				int file_result = fread(payload, sizeof(payload), 1, input_file);
				if (file_result != 1) {
					return CODEC_ERROR_FILE_READ;
				}

				tuple.payload = payload;
				tuple.payload_size = payload_size;

				error = InsertDatabaseTuple(database, &tuple);
				if (error != CODEC_ERROR_OKAY) {
					return error;
				}
			}
			else
			{
				error = InsertDatabaseTuple(database, &tuple);
				if (error != CODEC_ERROR_OKAY) {
					return error;
				}
			}

			if (debug_flag) {
				fprintf(stderr, "Tuple: %c%c%c%c, size: %zd, count: %zd, total_size: %zd, padding: %zd, payload_size: %zd\n",
					FOURCC_CHARS(tuple_tag), size, count, total_size, padding, payload_size);
			}
			//DumpNodeInfo(tuple, "New node");

			//printf("Tuple: %c%c%c%c, type: %c (%X)\n", FOURCC_CHARS(tuple_tag), ((type == 0) ? '0' : type), type);
		}

		// Update the nesting level for the next tuple in the bitstream
		database->current_level = database->next_level;
	}

	if (verbose_flag) fprintf(stderr, "\n");

	if (file_result != 1 && !feof(input_file)) {
		if (debug_flag) fprintf(stderr, "File read error: %d\n", file_result);
		return CODEC_ERROR_FILE_READ;
	}

	if (args->duplicates_flag) {
		// Prune duplicate entries from the database
		error = PruneDuplicateTuples(database->root, args);
		assert(error == CODEC_ERROR_OKAY);
		if (! (error == CODEC_ERROR_OKAY)) {
			return error;
		}
	}

	// Output the metadata in XML format
	error = OutputMetadataDatabase(database, output_file);

	// Delete the XML tree and remove all XML nodes from the database
	// mxmlDelete(database->root);
	// ClearDatabaseEntries(database);

	return error;	
}

#endif

#else

/**** This section of code creates a database from the binary representation of metadata ****/

/*
	@brief Insert a new metadata chunk element into the database
*/
CODEC_ERROR InsertDatabaseChunk(DATABASE *database, uint16_t chunk_tag, uint32_t chunk_size)
{
	CODEC_ERROR error = CODEC_ERROR_OKAY;

	printf("Database chunk tag: 0x%04X, size: %d\n", chunk_tag, chunk_size);


	return error;
}


/*
	@brief Insert a new metadata class instance into the current chunk element
*/
CODEC_ERROR InsertDatabaseClass(DATABASE *database, TUPLE_HEADER *tuple_header)
{
	CODEC_ERROR error = CODEC_ERROR_OKAY;

	printf("Database class tag: %c%c%c%c, type: %d, size: %d, padding: %d\n",
		FOURCC_CHARS(tuple_header->tag), tuple_header->type, tuple_header->size, tuple_header->padding);


	return error;
}


/*
	@brief Insert a new metadata tuple into the current metadata class
*/
CODEC_ERROR InsertDatabaseTuple(DATABASE *database, TUPLE *tuple)
{
	CODEC_ERROR error = CODEC_ERROR_OKAY;

	assert(HasaRepeatCount(tuple_header->type) || tuple_header->count == 0);
	printf("Database tuple tag: %c%c%c%c, type: %c, size: %d, count: %d, padding: %d\n",
		FOURCC_CHARS(tuple_header->tag), tuple_header->type, tuple_header->size, tuple_header->count, tuple_header->padding);


	return error;
}


/*
	@brief Write the metadata database to the output file in XML format
*/
CODEC_ERROR OutputMetadataDatabase(DATABASE *database, FILE *output_file)
{
	CODEC_ERROR error = CODEC_ERROR_OKAY;

	fprintf(stderr, "Output database to file\n");


	return error;
}


//! Nesting level in the metadata tuple hierarchy
static int current_level = 0;


/*!
	@brief Read metadata from a binary file and store the metadata in a database

	This is experimental code, not finished or tested, for creating a metadata database from the
	binary representation of metadata read from a file instead of dumping the metadata in XML format.
	Unlike other routines in the XML dumper, this routine does not create an XML tree as the
	intermediate ot final representation.

	The intent is that an application can start with this skeleton code to design and implement
	a true database for metadata tuples, providing fast access to metadata as needed by the
	application.

	Other routines that create an XML tree take care to output the metadata in the same XML format
	as the test cases input to the XML parser or sample encoder, making it easy to compare the
	output of this program with the each metadata test case. Extra code would have to be written
	to perform a similar comparison between a textual representation of the metadata database created
	by this program with the correponding metadata test case.
*/
CODEC_ERROR ReadBinaryMetadata(FILE *input_file, DATABASE *database, ARGUMENTS *args)
{
	CODEC_ERROR error = CODEC_ERROR_OKAY;

	// Return code from the operations
	int file_result = 1;

	// Initialize the current nesting level in the metadata tuple hierarchy
	current_level = 0;

	// Nesting level for metadata tuples that follow the current tuple
	int next_level = 0;

	// Read the chunk header
	SEGMENT chunk_header;
	file_result = fread(&chunk_header, sizeof(chunk_header), 1, input_file);
	if (file_result != 1) {
		return CODEC_ERROR_FILE_READ;
	}

	uint16_t chunk_tag = 0;
	uint32_t chunk_size = 0;

	if (! ParseChunkHeader(chunk_header, &chunk_tag, &chunk_size, false)) {
		return CODEC_ERROR_BAD_CHUNK;
	}

	if (args->debug_flag) {
		fprintf(stderr, "Chunk tag: 0x%0X, size: %d\n", chunk_tag, chunk_size);
	}
	else if (args->verbose_flag)
	{
		if (chunk_tag == METADATA_CHUNK_LARGE) {
			printf("%sChunk tag: 0x%02X, value: 0x%06X (%d)\n", Indentation(current_level), chunk_tag, chunk_size, chunk_size);
		} else {
			printf("%sChunk tag: 0x%04X, value: 0x%04X (%d)\n", Indentation(current_level), chunk_tag, chunk_size, chunk_size);
		}
	}

	error = InsertDatabaseChunk(database, chunk_tag, chunk_size);
	assert(error == CODEC_ERROR_OKAY);
	if (! (error == CODEC_ERROR_OKAY)) {
		return error;
	}

	// Push the node for the chunk element onto the node stack
	//PushNode(chunk);

	// Process the chunk payload at the next level in the tuple hierarchy
	//current_level++;

	// Read metadata tuples from the input file until end of file
	while (file_result == 1)
	{
		FOURCC tuple_tag;
		file_result = fread(&tuple_tag, sizeof(tuple_tag), 1, input_file);
		if (file_result != 1) break;

		// Is this tuple another metadata chunk element?
		SEGMENT chunk_header = (SEGMENT)tuple_tag;

		if (ParseChunkHeader(chunk_header, &chunk_tag, &chunk_size, false))
		{
			// Process the chunk payload at the top level in the tuple hierarchy
			current_level = 0;

			if (args->debug_flag) {
				fprintf(stderr, "Chunk tag: 0x%0X, size: %d\n", chunk_tag, chunk_size);
			}
			else if (args->verbose_flag)
			{
				if (chunk_tag == METADATA_CHUNK_LARGE) {
					printf("%sChunk tag: 0x%02X, value: 0x%06X (%d)\n", Indentation(current_level), chunk_tag, chunk_size, chunk_size);
				} else {
					printf("%sChunk tag: 0x%04X, value: 0x%04X (%d)\n", Indentation(current_level), chunk_tag, chunk_size, chunk_size);
				}
			}

			error = InsertDatabaseChunk(database, chunk_tag, chunk_size);
			assert(error == CODEC_ERROR_OKAY);
			if (! (error == CODEC_ERROR_OKAY)) {
				return error;
			}

			// The chunk node should be the first node on the XML node stack
			//ResetStack();

			// Push the node for the chunk element onto the node stack
			//PushNode(chunk);

			// Process the chunk payload at the next level in the tuple hierarchy
			current_level++;

			continue;
		}

		// Read the next segment containing the data type and the tuple size and repeat count
		SEGMENT type_size_count;
		file_result = fread(&type_size_count, sizeof(type_size_count), 1, input_file);
		if (file_result != 1) break;

		// Swap the segment into network (big endian) order
		type_size_count = Swap32(type_size_count);

		// The first byte is the tuple data type
		char type = (type_size_count >> 24);
		size_t count;
		size_t size;

		if (HasRepeatCount(type))
		{
			// One byte size and two byte count and two byte repeast count
			size = (type_size_count >> 16) & 0xFF;
			count = (type_size_count & 0xFFFF);
		}
		else
		{
			// Three byte size with no repeat count
			size = type_size_count & 0xFFFFFF;
			count = 0;
		}

		// Update the nesting level based on the new tuple tag and the current nested tag and level
		UpdateNestingLevel(tuple_tag, type, &current_level, &next_level);

		if (args->verbose_flag)
		{
			printf("%sTuple tag: %c%c%c%c, ", Indentation(current_level), FOURCC_CHARS(tuple_tag));

			// if (type == 0) {
			// 	printf("type: (none), size: %zu, count: %zu\n", size, count);
			// }
			// else {
				printf("type: %c, size: %zu, count: %zu\n", type, size, count);
			// }
		}

		size_t total_size = ((count > 0) ? count : 1) * size;
		//fprintf(stderr, "%zd\n", total_size);

		//if (tuple_tag == TupleTag("CFHD"))
		if (IsClassInstance(tuple_tag, type))
		{
			// Round up the size to a segment boundary
			size_t segment_count = (size + 3)/4;
			size_t padding = 4 * segment_count - size;

			TUPLE_HEADER tuple_header = {tuple_tag, type, size, count, padding};

			error = InsertDatabaseClass(database, &tuple_header);
			assert(error == CODEC_ERROR_OKAY);
			if (! (error == CODEC_ERROR_OKAY)) {
				return error;
			}

			//TODO: Check that the current node at the top of the stack is a metadata chunk element

			// Push the node for the metadata class instance onto the node stack
			//PushNode(class);
		}
		else
		{

			// Tuple must have a payload unless it is a nested tuple or an encoding curve
			assert(type == 0 || type == 'P' || total_size > 0);

			// Round up the size to a segment boundary
			size_t segment_count = (size + 3)/4;
			size_t payload_size = 4 * segment_count;
			size_t padding = payload_size - total_size;

			TUPLE tuple = {
				{tuple_tag, type, size, count, padding}, NULL
			};

			if (! IsNestedTuple(type))
			{
				// Allocate space for the tuple payload
				ALLOC_BUFFER(payload, payload_size);

				// Read the payload (metadata value and padding)
				int file_result = fread(payload, sizeof(payload), 1, input_file);
				if (file_result != 1) {
					return CODEC_ERROR_FILE_READ;
				}

				tuple.payload = payload;

				// Insert the tuple and payload into the database
				error = InsertDatabaseTuple(database, &tuple);
				assert(error == CODEC_ERROR_OKAY);
				if (! (error == CODEC_ERROR_OKAY)) {
					return error;
				}
			}
			else
			{
				// Insert a nested tuple into the database
				error = InsertDatabaseTuple(database, &tuple);
				assert(error == CODEC_ERROR_OKAY);
				if (! (error == CODEC_ERROR_OKAY)) {
					return error;
				}
			}

			//printf("Tuple: %c%c%c%c, type: %c (%X)\n", FOURCC_CHARS(tuple_tag), ((type == 0) ? '0' : type), type);

			if (args->debug_flag) {
				fprintf(stderr, "Tuple: %c%c%c%c, size: %zd, count: %zd, total_size: %zd, padding: %zd, payload_size: %zd\n",
					FOURCC_CHARS(tuple_tag), size, count, total_size, padding, payload_size);
			}
			//DumpNodeInfo(tuple, "New node");

		}

		// Update the nesting level for next tuple in the bitstream
		current_level = next_level;
	}

	if (args->verbose_flag) fprintf(stderr, "\n");

	if (file_result != 1 && !feof(input_file)) {
		if (args->debug_flag) fprintf(stderr, "File read error: %d\n", file_result);
		return CODEC_ERROR_FILE_READ;
	}

	return error;
}

#endif

#endif
