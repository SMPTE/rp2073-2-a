/*!	@file database.h

	Definitions for the database of metadata tuples read from the bitstream
*/

#ifndef _DATABASE_H
#define _DATABASE_H

// #if 0	//_DECODER && !_TESTING

// /*!
//     @brief fake arguments data structure used to connect the XML dumper to the reference decoder
// */
// typedef struct _arguments
// {
//     bool duplicates_flag;       //!< Remove duplicate tuples
//     bool verbose_flag;          //!< Enable verbose output
//     bool debug_flag;            //!< Enable debugging output

// } ARGUMENTS;

// #else

// //! Forward reference to the command-line arguments
// typedef struct _arguments ARGUMENTS;

// #endif

#if _DATABASE

//! Forward reference to the database structure
typedef struct _database DATABASE;


//! Create the metadata database for the hierarchy of metadata chunks and tuples read from the bitstream
CODEC_ERROR CreateMetadataDatabase(DATABASE **database_out, bool verbose_flag, bool debug_flag, bool duplicates_flag);

//! Initialize the database with the root and metadata nodes
//CODEC_ERROR InitMetadataProcessing(DATABASE *database);

//! Write the metadata database to the output file in XML format
CODEC_ERROR OutputMetadataDatabase(DATABASE *database, FILE *output_file);

//! Insert a new metadata chunk element into the database
CODEC_ERROR InsertDatabaseChunk(DATABASE *database, uint16_t chunk_tag, uint32_t chunk_size);

//! Insert a new metadata class instance into the current chunk element
CODEC_ERROR InsertDatabaseClass(DATABASE *database, TUPLE_HEADER *tuple_header);

//! Insert a new metadata tuple into the current metadata class
CODEC_ERROR InsertDatabaseTuple(DATABASE *database, TUPLE *header);

//! Clear all pointers in the database to XML nodes
CODEC_ERROR ClearDatabaseEntries(DATABASE *database);

//! Update the level in the metadata hierarchy
CODEC_ERROR UpdateDatabaseLevel(DATABASE *database, TUPLE_TAG tuple_tag, TUPLE_TYPE tuple_type);

//! Set the current level in the hierarchy to the next level
CODEC_ERROR SetDatabaseNextLevel(DATABASE *database);

//! Return the indentation for the current level in the metadata hierarchy
const char *CurrentLevelIndentation(DATABASE *database);

//! Remove duplicate tuples from the database
CODEC_ERROR PruneDatabaseDuplicateTuples(DATABASE *database);

//! Set the flags that control output to the terminal
CODEC_ERROR SetDatabaseFlags(DATABASE *database, bool verbose_flag, bool debug_flag);

//! Delete the metadata database
CODEC_ERROR DestroyMetadataDatabase(DATABASE *database);


#if _TESTING

//! Scaffolding for testing the code used by the reference decoder
CODEC_ERROR DecodeBinaryMetadata(FILE *input_file, FILE *output_file, DATABASE *database, ARGUMENTS *args);

#endif

#endif

#endif
