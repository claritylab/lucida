/*
 *      Constant Quark Database (CQDB).
 *
 * Copyright (c) 2007, Naoaki Okazaki
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the Northwestern University, University of Tokyo,
 *       nor the names of its contributors may be used to endorse or promote
 *       products derived from this software without specific prior written
 *       permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER
 * OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/* $Id$ */

#ifndef    __CQDB_H__
#define    __CQDB_H__

/** @file */



/** 
 * \addtogroup cqdb_const CQDB Constants
 * @{
 *
 *    The CQDB Constants.
 */

/**
 * CQDB flags.
 */
enum {
    CQDB_NONE = 0,                        /**< No flag. */
    CQDB_ONEWAY = 0x00000001,            /**< A reverse lookup array is omitted. */
    CQDB_ERROR_OCCURRED = 0x00010000,    /**< An error has occurred. */
};

/**
 * CQDB status codes.
 */
enum {
    CQDB_SUCCESS = 0,                    /**< Success. */
    CQDB_ERROR = -1024,                    /**< Unspecified error. */
    CQDB_ERROR_NOTFOUND,                /**< String not found. */
    CQDB_ERROR_OUTOFMEMORY,                /**< Insufficient memory. */
    CQDB_ERROR_FILEWRITE,                /**< Error in fwrite() operations. */
    CQDB_ERROR_FILETELL,                /**< Error in ftell() operations. */
    CQDB_ERROR_FILESEEK,                /**< Error in fseek() operations. */
    CQDB_ERROR_INVALIDID,                /**< Invalid parameters. */
};

/** @} */



/** 
 * \addtogroup cqdb_writer CQDB Writer API
 * @{
 *
 *    The CQDB Writer API constructs a CQDB chunk on a seekable stream. The
 *    seekable stream must be created by the fopen() function with writable and
 *    binary flags ("wb"). The CQDB Writer API can build a CQDB chunk at any
 *    position on the stream; one can thus write some data, append a CQDB chunk,
 *    and continue writing other data on the stream.
 *
 *    By default, the function cqdb_writer() constructs a database with forward
 *    (string to integer identifier) and backward (integer identifier to string)
 *    lookups. The data for reverse lookup is omitted with ::CQDB_ONEWAY flag
 *    specified.
 *
 *    It is recommended to keep the maximum number of identifiers as smallest as
 *    possible because reverse lookup is maintained by a array with the size of
 *    sizeof(int) * (maximum number of identifiers + 1). For example, putting a
 *    set of integer identifers (0, 1, 1000) creates a reverse lookup array with
 *    1001 elements only to waste the disk space for 998 (= 1001-3) elements in
 *    the array.
 */

struct tag_cqdb_writer;
typedef struct tag_cqdb_writer cqdb_writer_t;    /**< Typedef of a CQDB writer. */

/**
 * Create a new CQDB writer on a seekable stream.
 *
 *    This function initializes a database on the seekable stream and returns
 *    the pointer to a ::cqdb_writer_t instance to write the database.
 *    The stream must have the writable and binary flags. The database creation
 *    flag must be zero except when the reverse lookup array is unnecessary;
 *    specifying ::CQDB_ONEWAY flag will save the storage space for the reverse
 *    lookup array. Once calling this function, one should avoid accessing the
 *    seekable stream directly until calling cqdb_writer_close().
 *
 *    @param    fp                The pointer to the writable and seekable stream.
 *    @param    flag            Database creation flag.
 *    @retval    cqdb_writer_t*    The pointer to the new ::cqdb_writer_t instance if
 *                            successful; otherwise \c NULL.
 */
cqdb_writer_t* cqdb_writer(FILE *fp, int flag);

/**
 * Put a string/identifier association to the database.
 *
 *    This function append a string/identifier association into the database.
 *    Make sure that the string and/or identifier have never been inserted to
 *    the database and that the identifier is a non-negative value.
 *
 *    @param    dbw            The pointer to the ::cqdb_writer_t instance.
 *    @param    str            The pointer to the string.
 *    @param    id            The identifier.
 *    @retval    int            Zero if successful, or a status code otherwise.
 */
int cqdb_writer_put(cqdb_writer_t* dbw, const char *str, int id);

/**
 * Close a CQDB writer.
 *
 *    This function finalizes the database on the stream. If successful, the
 *    data remaining on the memory is flushed to the stream; the stream position
 *    is moved to the end of the chunk. If an unexpected error occurs, this
 *    function tries to rewind the stream position to the original position when
 *    the function cqdb_writer() was called.
 *
 *    @param    dbw            The pointer to the ::cqdb_writer_t instance.
 *    @retval    int            Zero if successful, or a status code otherwise.
 */
int cqdb_writer_close(cqdb_writer_t* dbw);

/** @} */



/** 
 * \addtogroup cqdb_reader CQDB Reader API
 * @{
 *
 *    The CQDB reader API provides a read access to the database whose memory
 *    image is loaded on a memory block. The memory-passing interface has
 *    several advantages. Firstly, one can choose an efficient way for their
 *    application to load a database image to a memory block, e.g., to read
 *    the whole image from a file, to use the Memory Mapped File (mmap) API,
 *    etc.
 *    Secondaly, one can design the file format freely only if the memory
 *    block for a database is extracted from the file.
 *    
 *    The most fundamental operation on the CQDB reader API is forward lookup
 *    through the use of cqdb_to_id() function, which retrieves integer
 *    identifiers from strings. Reverse lookup (retrieving strings from integer
 *    identifiers) with cqdb_to_string() function is not supported if the
 *    database has been created with ::CQDB_ONEWAY flag.
 */

struct tag_cqdb;
typedef struct tag_cqdb cqdb_t;        /**< Typedef of a CQDB reader. */

/**
 * Open a new CQDB reader on a memory block.
 *
 *    This function initializes a database on a memory block and returns the
 *    pointer to a ::cqdb_t instance to access the database.
 *
 *    @param    buffer        The pointer to the memory block.
 *    @param    size        The size of the memory block.
 *    @retval    cqdb_t*        The pointer to the ::cqdb_t instance.
 */
cqdb_t* cqdb_reader(void *buffer, size_t size);

/**
 * Delete the CQDB reader.
 *
 *    This function frees the work area allocated by cqdb_reader() function.
 *
 *    @param    db            The pointer to the ::cqdb_t instance.
 */
void cqdb_delete(cqdb_t* db);

/**
 * Retrieve the identifier associated with a string.
 *
 *    This function returns the identifier associated with a string.
 *
 *    @param    db            The pointer to the ::cqdb_t instance.
 *    @param    str            The pointer to a string.
 *    @retval    int            The non-negative identifier if successful, negative
 *                        status code otherwise.
 */
int cqdb_to_id(cqdb_t* db, const char *str);

/**
 * Retrieve the string associated with an identifier.
 *
 *    This function returns the string associated with an identifier.
 *
 *    @param    db            The pointer to the cqdb_t instance.
 *    @param    id            The id.
 *    @retval    const char*    The pointer to the string associated with the
 *                        identifier if successful; otherwise \c NULL.
 */
const char* cqdb_to_string(cqdb_t* db, int id);

/**
 * Get the number of associations in the database.
 *
 *    This function returns the number of associations in the database.
 *
 *    @param    db            The pointer to the ::cqdb_t instance.
 *    @retval    int            The number of string/identifier associations.
 */
int cqdb_num(cqdb_t* db);

/** @} */



/**
@mainpage Constant Quark Database (CQDB)

@section intro Introduction

It is a common technique for speed and memory optimizations that an
application converts all string values into integer identifiers, does some
processing with integer values, and then restores the original string values
(if necessary). The data structure for two-way associations between strings
and integer identifiers is known as Quark:
- GQuark in GLib: http://www.gtk.org/
- quark (C++): http://www.chokkan.org/software/sample/quark.h

Constant Quark Database (CQDB) is a database library specialized for
serialization and retrieval of <i>static</i> associations between strings and
integer identifiers. The database provides several features:
- <b>Fast look-ups.</b> Retrieving an integer identifier for a string is
  usually done by accessing three memory blocks. Retrieving a string for an
  integer identifier is always done by accessing two memory blocks.
  See the @ref performance "performance" evaluation.
- <b>Low overhead.</b> A CQDB database consists of a chunk header (24 bytes),
  hash tables (2048 bytes and 16 bytes per record), a reverse lookup array
  (4 bytes per integer identifier), and records (8 bytes + string size per
  record).
  See the @ref performance "performance" evaluation.
- <b>Sophisticated hash function.</b> CQDB incorporates the fast and
  collision-resistant hash function for strings
  (<a href="http://www.burtleburtle.net/bob/c/lookup3.c">lookup3.c</a>)
  implemented by Bob Jenkins.
- <b>Chunk format.</b> The structure of CQDB is designed to store the data in
  a chunk of a file; CQDB can be embedded into a file with other arbitrary
  data.
- <b>Omissible reverse look-up array.</b> The reverse look-up array can be
  omitted if it is not necessary to retrieve strings from integer identifiers.
- <b>Cross platform.</b> The source code can be compiled on Microsoft Visual
  Studio 2005, GNU C Compiler (gcc), etc.
- <b>Simple API.</b> The CQDB API exposes only a few functions.

CQDB is suitable for implementing dictionaries in which fast look-ups of
strings and identifiers are essential while a dictionary update rarely occurs.
The data structure is a specialization (and extension) of the
<a href="http://cr.yp.to/cdb.html">Constant Database</a> proposed by
Daniel J. Bernstein.

CQDB does not support assigning a unique integer identifier for a given string,
modify associations, nor check collisions in strings and identifiers; thus,
it may be necessary to use an existing Quark implementation to manage proper
associations between strings and identifiers on memory.

This library is used by the 
<a href="http://www.chokkan.org/software/crfsuite/">CRFsuite</a> project.

@section download Download

- <a href="http://www.chokkan.org/software/dist/cqdb-1.1.tar.gz">Source code</a>

CQDB is distributed under the term of the
<a href="http://www.opensource.org/licenses/bsd-license.php">modified BSD license</a>.

@section changelog History
- Version 1.1 (2007-12-01):
    - Fixed a bug when a CQDB chunk is embedded to a file.

- Version 1.0 (2007-09-20):
    - Initial release.

@section api Documentation

- @ref cqdb_const "CQDB Constants"
- @ref cqdb_reader "CQDB Reader API"
- @ref cqdb_writer "CQDB Writer API"

@section sample Sample programs
@subsection sample_writer A writer sample

This sample code constructs a database "test.cqdb" with 1,000,000
string/identifier associations,
"00000000"/0, "00000001"/1, ..., "01000000"/1000000.

@code

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cqdb.h"

#define    DBNAME        "test.cqdb"
#define    NUMELEMS    1000000

int main(int argc, char *argv[])
{
    int i, ret;
    char str[10];
    FILE *fp = NULL;
    cqdb_writer_t* dbw = NULL;

    // Open a file for writing.
    fp = fopen(DBNAME, "wb");
    if (fp == NULL) {
        fprintf(stderr, "ERROR: failed to open the file.\n");
        return 1;
    }

    // Create a CQDB on the file stream.
    dbw = cqdb_writer(fp, 0);
    if (dbw == NULL) {
        fprintf(stderr, "ERROR: failed to create a CQDB on the file.\n");
        goto error_exit;
    }

    // Put string/integer associations, "00000001"/1, ..., "01000000"/1000000.
    for (i = 0;i < NUMELEMS;++i) {
        sprintf(str, "%08d", i);
        if (ret = cqdb_writer_put(dbw, str, i)) {
            fprintf(stderr, "ERROR: failed to put a pair '%s'/%d.\n", str, i);
            goto error_exit;    
        }
    }

    // Close the CQDB.
    if (ret = cqdb_writer_close(dbw)) {
        fprintf(stderr, "ERROR: failed to close the CQDB.\n");        
        goto error_exit;
    }

    // Close the file.
    fclose(fp);
    return 0;

error_exit:
    if (dbw != NULL) cqdb_writer_close(dbw);
    if (fp != NULL) fclose(fp);
    return 1;
}

@endcode

@subsection sample_reader A reader sample

This sample code issues string queries "00000000", ..., "01000000" to retrive
integer identifiers (forward lookups) and integer queries 0, ..., 1000000 to
retrieve the strings  "00000000", ..., "01000000". 

@code

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cqdb.h"

#define    DBNAME        "test.cqdb"
#define    NUMELEMS    1000000

int main(int argc, char *argv[])
{
    int i, j, ret;
    long size = 0;
    char str[10], *value = NULL, *buffer = NULL;
    FILE *fp = NULL;
    cqdb_t* db = NULL;

    // Open the database.
    fp = fopen(DBNAME, "rb");
    if (fp == NULL) {
        fprintf(stderr, "ERROR: failed to open the file\n");
        return 1;
    }

    // Obtain the file size.
    fseek(fp, 0, SEEK_END);
    size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    // Read the content of the file at a time.
    buffer = (char *)malloc(size);
    if (buffer == NULL) {
        fprintf(stderr, "ERROR: out of memory.\n");
        goto error_exit;
    }
    fread(buffer, 1, size, fp);
    fclose(fp);
    fp = NULL;

    // Open the database on the memory.
    db = cqdb_reader(buffer, size);
    if (db == NULL) {
        fprintf(stderr, "ERROR: failed to open a CQDB on the file.\n");
        goto error_exit;
    }

    // Forward lookups: strings to integer identifiers.
    for (i = 0;i < NUMELEMS;++i) {
        sprintf(str, "%08d", i);
        j = cqdb_to_id(db, str);
        // Validity check.
        if (j < 0 || i != j) {
            fprintf(stderr, "ERROR: inconsistency error '%s'/%d.\n", str, i);
            goto error_exit;    
        }
    }

    // Reverse lookups: integer identifiers to strings.
    for (i = 0;i < NUMELEMS;++i) {
        sprintf(str, "%08d", i);
        value = cqdb_to_string(db, i);
        // Validity check.
        if (value == NULL || strcmp(str, value) != 0) {
            fprintf(stderr, "ERROR: inconsistency error '%s'/%d.\n", str, i);
            goto error_exit;    
        }
    }

    // Delete the instance of the CQDB.
    cqdb_delete(db);
    free(buffer);

    return 0;

error_exit:
    if (fp != NULL) fclose(fp);
    if (buffer != NULL) free(buffer);
    return 1;
}

@endcode

@section performance Performance

An experiment for performance comparision with
<a href="http://www.oracle.com/database/berkeley-db/">Berkeley DB (BDB) 4.5.20</a>
and <a href="http://qdbm.sourceforge.net/">Quick Database Manager (QDBM) 1.8.75</a>
was conducted.
Constructing a database with 1,000,000 string/identifier associations,
"00000000"/0, "00000001"/1, ..., "01000000"/1000000, this experiment issued
string queries "00000000", ..., "01000000" (forward lookups) and integer
queries 0, ..., 1000000 (reverse lookups) to the database. Since BDB and
QDBM do not support reverse lookups, reverse items (key: identifier,
value: string) were inserted to the database in addition to the forward items
(key: string, value: integer). Microsoft Windows Vista Business was running on
the test environment (Intel Core2Duo 6600 (2.40GHz) processor, Intel G965
chipset, 2GB main memory, and Seagate ST3320620 HDD).
The test codes were compiled with Microsoft Visual Studio 2005.

This table shows the elapsed time for constructing the database (write time),
the elapsed time for processing with the queries (read time), and the size of
the database generated by each database library.
The read/write access was extremely faster than those of other database
libraries. The database was smaller than half the size of those generated by
other libraries.
This results suggest that the CQDB has the substantial advantage over the
existing database libraries for implementing a static quark database.

<table>
<tr>
<th>Database</th><th>Parameters</th><th>Write time [sec]</th><th>Read time [sec]</th><th>Database size [MB]</th>
</tr>
<tr align="right">
<td align="left">Constant Quark Database (CQDB) 1.0</td>
<td align="left">Default (none)</td>
<td><b>1.48</b></td><td><b>0.65</b></td><td><b>35.2</b></td>
</tr>
<tr align="right">
<td align="left">Berkeley DB (BDB) 4.5.20</td>
<td align="left">Default</td>
<td>91.8</td><td>37.5</td><td>79.7</td>
</tr>
<tr align="right">
<td align="left">Berkeley DB (BDB) 4.5.20</td>
<td align="left">table_size=4000000; cache_size=200MB</td>
<td>57.8</td><td>37.5</td><td>79.7</td>
</tr>
<tr align="right">
<td align="left">Quick Database Manager (QDBM) 1.8.75</td>
<td align="left">Default</td>
<td>95.4</td><td>80.6</td><td>76.3</td>
</tr>
<tr align="right">
<td align="left">Quick Database Manager (QDBM) 1.8.75</td>
<td align="left">table_size=4000000</td>
<td>15.7</td><td>12.0</td><td>92.2</td>
</tr>
</table>

@section reference Reference
- <a href="http://cr.yp.to/cdb.html">cdb</a> by Daniel J. Bernstein.
- <a href="http://www.corpit.ru/mjt/tinycdb.html">TinyCDB</a> by Michael Tokarev.
- <a href="http://www.unixuser.org/~euske/doc/cdbinternals/index.html">Constant Database (cdb) Internals</a> by Yusuke Shinyama.
- <a href="http://www.burtleburtle.net/bob/hash/index.html">Hash Functions and Block Ciphers</a> by Bob Jenkins.

*/

#endif/*__CQDB_H__*/
