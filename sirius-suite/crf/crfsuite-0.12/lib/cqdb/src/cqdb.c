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

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include <cqdb.h>

#define CHUNKID             "CQDB"
#define BYTEORDER_CHECK     (0x62445371)
#define NUM_TABLES          (256)
#define OFFSET_REFS         (0 + sizeof(header_t))
#define OFFSET_DATA         (OFFSET_REFS + sizeof(tableref_t) * NUM_TABLES)

/**
 * An element of a hash table.
 */
typedef struct {
    uint32_t    hash;       /**< Hash value of the record. */
    uint32_t    offset;     /**< Offset address to the actual record. */
} bucket_t;

/**
 * A hash table.
 */
typedef struct {
    uint32_t    num;        /**< Number of elements in the table. */
    uint32_t    size;       /**< Maximum number of elements. */
    bucket_t*   bucket;     /**< Bucket (array of bucket_t). */
} table_t;

/**
 * CQDB chunk header.
 */
typedef struct {
    int8_t      chunkid[4]; /**< Chunk identifier, "CQDB". */
    uint32_t    size;       /**< Chunk size including this header. */
    uint32_t    flag;       /**< Global flags. */
    uint32_t    byteorder;  /**< Byte-order indicator. */
    uint32_t    bwd_size;   /**< Number of elements in the backward array. */
    uint32_t    bwd_offset; /**< Offset to the backward array. */
} header_t;

/**
 * Reference to a hash table.
 */
typedef struct {
    uint32_t    offset;     /**< Offset to a hash table. */
    uint32_t    num;        /**< Number of elements in the hash table. */
} tableref_t;

/**
 * Writer for a constant quark database.
 */
struct tag_cqdb_writer {
    uint32_t    flag;           /**< Operation flag. */
    FILE*       fp;             /**< File pointer. */
    uint32_t    begin;          /**< Offset address to the head of this database. */
    uint32_t    cur;            /**< Offset address to a new key/data pair. */
    table_t     ht[NUM_TABLES]; /**< Hash tables (string -> id). */

    uint32_t*   bwd;            /**< Backlink array. */
    uint32_t    bwd_num;        /**< */
    uint32_t    bwd_size;       /**< Number of elements in the backlink array. */
};

/**
 * Constant quark database (CQDB).
 */
struct tag_cqdb {
    uint8_t*    buffer;         /**< Pointer to the memory block. */
    size_t      size;           /**< Size of the memory block. */

    header_t    header;         /**< Chunk header. */
    table_t     ht[NUM_TABLES]; /**< Hash tables (string -> id). */

    uint32_t*   bwd;            /**< Array for backward look-up (id -> string). */

    int         num;            /**< Number of key/data pairs. */
};


uint32_t hashlittle(const void *key, size_t length, uint32_t initval);




static size_t write_uint32(cqdb_writer_t* wt, uint32_t value)
{
    uint8_t buffer[4];
    buffer[0] = (uint8_t)(value & 0xFF);
    buffer[1] = (uint8_t)(value >> 8);
    buffer[2] = (uint8_t)(value >> 16);
    buffer[3] = (uint8_t)(value >> 24);
    return fwrite(buffer, sizeof(uint8_t), 4, wt->fp) / sizeof(value);
}

static size_t write_data(cqdb_writer_t* wt, const void *data, size_t size)
{
    return fwrite(data, size, 1, wt->fp);
}

cqdb_writer_t* cqdb_writer(FILE *fp, int flag)
{
    int i;
    cqdb_writer_t* dbw = (cqdb_writer_t*)calloc(1, sizeof(cqdb_writer_t));

    if (dbw != NULL) {
        /* Initialize cqdb_writer_t members. */
        memset(dbw, 0, sizeof(*dbw));
        dbw->flag = flag;
        dbw->fp = fp;
        dbw->begin = ftell(dbw->fp);
        dbw->cur = OFFSET_DATA;

        /* Initialize the hash tables.*/
        for (i = 0;i < NUM_TABLES;++i) {
            dbw->ht[i].bucket = NULL;
        }

        dbw->bwd = NULL;
        dbw->bwd_num = 0;
        dbw->bwd_size = 0;

        /* Move the file pointer to the offset to the first key/data pair. */
        if (fseek(dbw->fp, dbw->begin + dbw->cur, SEEK_SET) != 0) {
            goto error_exit;    /* Seek error. */
        }
    }

    return dbw;

error_exit:
    free(dbw);
    return NULL;
}

static int cqdb_writer_delete(cqdb_writer_t* dbw)
{
    int i;

    /* Free allocated memory blocks. */
    for (i = 0;i < NUM_TABLES;++i) {
        free(dbw->ht[i].bucket);
    }
    free(dbw->bwd);
    free(dbw);
    return 0;
}

int cqdb_writer_put(cqdb_writer_t* dbw, const char *str, int id)
{
    int ret = 0;
    const void *key = str;
    uint32_t ksize = (uint32_t)(strlen(str) + 1);

    /* Compute the hash value and choose a hash table. */
    uint32_t hv = hashlittle(key, ksize, 0);
    table_t* ht = &dbw->ht[hv % 256];

    /* Check for non-negative identifier. */
    if (id < 0) {
        ret = CQDB_ERROR_INVALIDID;
        goto error_exit;
    }

    /* Write out the current data. */
    write_uint32(dbw, (uint32_t)id);
    write_uint32(dbw, (uint32_t)ksize);
    write_data(dbw, key, ksize);
    if (ferror(dbw->fp)) {
        ret = CQDB_ERROR_FILEWRITE;
        goto error_exit;
    }

    /* Expand the bucket if necessary. */
    if (ht->size <= ht->num) {
        ht->size = (ht->size+1) * 2;
        ht->bucket = (bucket_t*)realloc(ht->bucket, sizeof(bucket_t) * ht->size);
        if (ht->bucket == NULL) {
            ret = CQDB_ERROR_OUTOFMEMORY;
            goto error_exit;
        }
    }

    /* Set the hash value and current offset position. */
    ht->bucket[ht->num].hash = hv;
    ht->bucket[ht->num].offset = dbw->cur;
    ++ht->num;

    /* Store the backlink if specified. */
    if (!(dbw->flag & CQDB_ONEWAY)) {
        /* Expand the backlink array if necessary. */
        if (dbw->bwd_size <= (uint32_t)id) {
            uint32_t size = dbw->bwd_size;

            while (size <= (uint32_t)id) size = (size + 1) * 2;
            dbw->bwd = (uint32_t*)realloc(dbw->bwd, sizeof(uint32_t) * size);
            if (dbw->bwd == NULL) {
                ret = CQDB_ERROR_OUTOFMEMORY;
                goto error_exit;
            }
            while (dbw->bwd_size < size) {
                dbw->bwd[dbw->bwd_size++] = 0;
            }
        }

        if (dbw->bwd_num <= (uint32_t)id) {
            dbw->bwd_num = (uint32_t)id+1;
        }

        dbw->bwd[id] = dbw->cur;
    }

    /* Increment the current position. */
    dbw->cur += sizeof(uint32_t) + sizeof(uint32_t) + ksize;
    return 0;

error_exit:
    dbw->flag |= CQDB_ERROR_OCCURRED;
    return ret;
}

int cqdb_writer_close(cqdb_writer_t* dbw)
{
    uint32_t i, j;
    int k, ret = 0;
    long offset = 0;
    header_t header;

    /* If an error have occurred, just free the memory blocks. */
    if (dbw->flag & CQDB_ERROR_OCCURRED) {
        cqdb_writer_delete(dbw);
        return 0;
    }

    /* Initialize the file header. */
    strncpy((char*)header.chunkid, CHUNKID, 4);
    header.byteorder = BYTEORDER_CHECK;
    header.bwd_offset = 0;
    header.bwd_size = dbw->bwd_num;

    /*
        Store the hash tables. At this moment, the file pointer refers to
        the offset succeeding the last key/data pair.
     */
    for (i = 0;i < NUM_TABLES;++i) {
        table_t* ht = &dbw->ht[i];

        /* Do not write empty hash tables. */
        if (ht->bucket != NULL) {
            /*
                Actual bucket will have the double size; half elements
                in the bucket are kept empty.
             */
            int n = ht->num * 2;

            /* Allocate the bucket. */
            bucket_t* dst = (bucket_t*)calloc(n, sizeof(bucket_t));
            if (dst == NULL) {
                ret = CQDB_ERROR_OUTOFMEMORY;
                goto error_exit;
            }

            /*
                Put hash elements to the bucket with the open-address method.
             */
            for (j = 0;j < ht->num;++j) {
                const bucket_t* src = &ht->bucket[j];
                int k = (src->hash >> 8) % n;

                /* Find a vacant element. */
                while (dst[k].offset != 0) {
                    k = (k+1) % n;
                }

                /* Store the hash element. */
                dst[k].hash = src->hash;
                dst[k].offset = src->offset;
            }

            /* Write the bucket. */
            for (k = 0;k < n;++k) {
                write_uint32(dbw, dst[k].hash);
                write_uint32(dbw, dst[k].offset);
            }

            /* Free the bucket. */
            free(dst);
        }
    }

    /* Write the backlink array if specified. */
    if (!(dbw->flag & CQDB_ONEWAY) && 0 < dbw->bwd_size) {
        /* Store the offset to the head of this array. */
        header.bwd_offset = ftell(dbw->fp) - dbw->begin;
        /* Store the contents of the backlink array. */
        for (i = 0;i < dbw->bwd_num;++i) {
            write_uint32(dbw, dbw->bwd[i]);
        }
    }

    /* Check for an occurrence of a file-related error. */
    if (ferror(dbw->fp)) {
        ret = CQDB_ERROR_FILEWRITE;
        goto error_exit;
    }

    /* Store the current position. */
    offset = ftell(dbw->fp);
    if (offset == -1) {
        ret = CQDB_ERROR_FILETELL;
        goto error_exit;
    }
    header.size = (uint32_t)offset - dbw->begin;

    /* Rewind the current position to the beginning. */
    if (fseek(dbw->fp, dbw->begin, SEEK_SET) != 0) {
        ret = CQDB_ERROR_FILESEEK;
        goto error_exit;
    }

    /* Write the file header. */
    write_data(dbw, header.chunkid, 4);
    write_uint32(dbw, header.size);
    write_uint32(dbw, header.flag);
    write_uint32(dbw, header.byteorder);
    write_uint32(dbw, header.bwd_size);
    write_uint32(dbw, header.bwd_offset);

    /*
        Write references to hash tables. At this moment, dbw->cur points
        to the offset succeeding the last key/data pair. 
     */
    for (i = 0;i < NUM_TABLES;++i) {
        /* Offset to the hash table (or zero for non-existent tables). */
        write_uint32(dbw, dbw->ht[i].num ? dbw->cur : 0);
        /* Bucket size is double to the number of elements. */
        write_uint32(dbw, dbw->ht[i].num * 2);
        /* Advance the offset counter. */
        dbw->cur += (dbw->ht[i].num * 2) * sizeof(bucket_t);
    }

    /* Check an occurrence of a file-related error. */
    if (ferror(dbw->fp)) {
        ret = CQDB_ERROR_FILEWRITE;
        goto error_exit;
    }

    /* Seek to the last position. */
    if (fseek(dbw->fp, offset, SEEK_SET) != 0) {
        ret = CQDB_ERROR_FILESEEK;
        goto error_exit;
    }

    cqdb_writer_delete(dbw);
    return ret;

error_exit:
    /* Seek to the first position. */
    fseek(dbw->fp, dbw->begin, SEEK_SET);
    cqdb_writer_delete(dbw);
    return ret;
}



static uint32_t read_uint32(uint8_t* p)
{
    uint32_t value;
    value  = ((uint32_t)p[0]);
    value |= ((uint32_t)p[1] << 8);
    value |= ((uint32_t)p[2] << 16);
    value |= ((uint32_t)p[3] << 24);
    return value;
}

static uint8_t *read_tableref(tableref_t* ref, uint8_t *p)
{
    ref->offset = read_uint32(p);
    p += sizeof(uint32_t);
    ref->num = read_uint32(p);
    p += sizeof(uint32_t);
    return p;
}

static bucket_t* read_bucket(uint8_t* p, uint32_t num)
{
    uint32_t i;
    bucket_t *bucket = (bucket_t*)calloc(num, sizeof(bucket_t));
    for (i = 0;i < num;++i) {
        bucket[i].hash = read_uint32(p);
        p += sizeof(uint32_t);
        bucket[i].offset = read_uint32(p);
        p += sizeof(uint32_t);
    }
    return bucket;
}

static uint32_t* read_backward_links(uint8_t* p, uint32_t num)
{
    uint32_t i;
    uint32_t *bwd = (uint32_t*)calloc(num, sizeof(uint32_t));
    for (i = 0;i < num;++i) {
        bwd[i] = read_uint32(p);
        p += sizeof(uint32_t);
    }
    return bwd;
}

cqdb_t* cqdb_reader(void *buffer, size_t size)
{
    int i;
    cqdb_t* db = NULL;

    /* The minimum size of a valid CQDB is OFFSET_DATA. */
    if (size < OFFSET_DATA) {
        return NULL;
    }

    /* Check the file chunkid. */
    if (memcmp(buffer, CHUNKID, 4) != 0) {
        return NULL;
    }
    
    db = (cqdb_t*)calloc(1, sizeof(cqdb_t));
    if (db != NULL) {
        uint8_t* p = NULL;

        /* Set memory block and size. */
        db->buffer = buffer;
        db->size = size;

        /* Read the database header. */
        p = db->buffer;
        strncpy((char*)db->header.chunkid, (const char*)p, 4);
        p += sizeof(uint32_t);
        db->header.size = read_uint32(p);
        p += sizeof(uint32_t);
        db->header.flag = read_uint32(p);
        p += sizeof(uint32_t);
        db->header.byteorder = read_uint32(p);
        p += sizeof(uint32_t);
        db->header.bwd_size = read_uint32(p);
        p += sizeof(uint32_t);
        db->header.bwd_offset = read_uint32(p);
        p += sizeof(uint32_t);

        /* Check the consistency of byte order. */
        if (db->header.byteorder != BYTEORDER_CHECK) {
            free(db);
            return NULL;
        }

        /* Check the chunk size. */
        if (size < db->header.size) {
            free(db);
            return NULL;
        }

        /* Set pointers to the hash tables. */
        db->num = 0;    /* Number of records. */
        p = (db->buffer + OFFSET_REFS);
        for (i = 0;i < NUM_TABLES;++i) {
            tableref_t ref;
            p = read_tableref(&ref, p);
            if (ref.offset) {
                /* Set buckets. */
                db->ht[i].bucket = read_bucket(db->buffer + ref.offset, ref.num);
                db->ht[i].num = ref.num;
            } else {
                /* An empty hash table. */
                db->ht[i].bucket = NULL;
                db->ht[i].num = 0;
            }

            /* The number of records is the half of the table size.*/
            db->num += ref.num / 2;
        }

        /* Set the pointer to the backlink array if any. */
        if (db->header.bwd_offset) {
            db->bwd = read_backward_links(db->buffer + db->header.bwd_offset, db->num);
        } else {
            db->bwd = NULL;
        }
    }

    return db;
}

void cqdb_delete(cqdb_t* db)
{
    int i;

    if (db != NULL) {
        for (i = 0;i < NUM_TABLES;++i) {
            free(db->ht[i].bucket);
        }
        free(db->bwd);
        free(db);
    }
}

int cqdb_to_id(cqdb_t* db, const char *str)
{
    uint32_t hv = hashlittle(str, strlen(str)+1, 0);
    int t = hv % 256;
    table_t* ht = &db->ht[t];

    if (ht->num && ht->bucket != NULL) {
        int n = ht->num;
        int k = (hv >> 8) % n;
        bucket_t* p = NULL;

        while (p = &ht->bucket[k], p->offset) {
            if (p->hash == hv) {
                int value;
                uint32_t ksize;
                uint8_t *q = db->buffer + p->offset;
                value = (int)read_uint32(q);
                q += sizeof(uint32_t);
                ksize = read_uint32(q);
                q += sizeof(uint32_t);
                if (strcmp(str, (const char *)q) == 0) {
                    return value;
                }
            }
            k = (k+1) % n;
        }
    }

    return CQDB_ERROR_NOTFOUND;
}

const char* cqdb_to_string(cqdb_t* db, int id)
{
    /* Check if the current database supports the backward look-up. */
    if (db->bwd != NULL && (uint32_t)id < db->header.bwd_size) {
        uint32_t offset = db->bwd[id];
        if (offset) {
            uint8_t *p = db->buffer + offset;
            p += sizeof(uint32_t);  /* Skip key data. */
            p += sizeof(uint32_t);  /* Skip value size. */
            return (const char *)p;
        }
    }

    return NULL;
}

int cqdb_num(cqdb_t* db)
{
    return db->num;
}
