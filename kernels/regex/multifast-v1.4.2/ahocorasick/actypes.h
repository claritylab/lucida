/*
 * actypes.h: Includes basic data types of ahocorasick library
 * This file is part of multifast.
 *
    Copyright 2010-2013 Kamiar Kanani <kamiar.kanani@gmail.com>

    multifast is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    multifast is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with multifast.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef _AC_TYPES_H_
#define _AC_TYPES_H_

#ifdef __cplusplus
extern "C" {
#endif

/* AC_ALPHABET_t:
 * defines the alphabet type.
 * Actually defining AC_ALPHABET_t as a char work as well, but sometimes we deal
 * with streams of other basic types e.g. integers or enumerators.
 * Although they consists of string of bytes (chars), but using their specific
 * types as AC_ALPHABET_t will lead to a better performance. so instead of
 * working with strings of chars, we assume that we are working with strings of
 * AC_ALPHABET_t and leave it optional for other users to define their
 * own alphabets.
**/
typedef char AC_ALPHABET_t;

/* AC_REP_t:
 * Provides a more readable representative for a pattern.
 * because patterns themselves are not always suitable for displaying
 * (e.g. hex patterns), we offer this type to improve intelligibility
 * of output. Sometimes it can be also useful, when you are
 * retrieving patterns from a database, to maintain their identifiers in the
 * automata for further reference. we provisioned two possible types as a
 * union. you can add your desired type in it.
**/
typedef union AC_REP
{
    const char * stringy; /* null-terminated string */
    unsigned long number;
} AC_REP_t;

/* AC_PATTERN_t:
 * This is the pattern type that must be fed into AC automata.
 * the 'astring' field is not null-terminated, because it can contain zero
 * value bytes. the 'length' field determines the number of AC_ALPHABET_t it
 * carries. the 'rep' field is described in AC_REP_t. despite
 * 'astring', 'rep' can have duplicate values for different given
 * AC_PATTERN_t. it is an optional field and you can just fill it with 0.
 * CAUTION:
 * Not always the 'astring' points to the correct position in memory.
 * it is the responsibility of your program to maintain a permanent allocation
 * for astring field.
**/
typedef struct AC_PATTERN
{
    const AC_ALPHABET_t * astring; /* String of alphabets */
    unsigned int length; /* Length of pattern */
    AC_REP_t rep; /* Representative string (optional) */
} AC_PATTERN_t;

/* AC_TEXT_t:
 * The input text type that is fed to ac_automata_search() to be searched.
 * it is similar to AC_PATTERN_t. actually we could use AC_PATTERN_t as input
 * text, but for the purpose of being more readable, we defined this new type.
**/
typedef struct AC_TEXT
{
    const AC_ALPHABET_t * astring; /* String of alphabets */
    unsigned int length; /* Length of string */
} AC_TEXT_t;

/* AC_MATCH_t:
 * Provides the structure for reporting a match in the text.
 * a match occurs when the automata reaches a final node. any final
 * node can match one or more pattern at a position in a text. the
 * 'patterns' field holds these matched patterns. obviously these
 * matched patterns have same end-position in the text. there is a relationship
 * between matched patterns: the shorter one is a factor (tail) of the longer
 * one. the 'position' maintains the end position of matched patterns. the
 * start position of patterns could be found by knowing their 'length' in
 * AC_PATTERN_t. e.g. suppose "recent" and "cent" are matched at
 * position 40 in the text, then the start position of them are 34 and 36
 * respectively. finally the field 'match_num' maintains the number of
 * matched patterns.
**/
typedef struct AC_MATCH
{
    AC_PATTERN_t * patterns; /* Array of matched pattern */
    long position; /* The end position of matching pattern(s) in the text */
    unsigned int match_num; /* Number of matched patterns */
} AC_MATCH_t;

/* AC_STATUS_t:
 * Return status of an AC function
**/
typedef enum AC_STATUS
{
    ACERR_SUCCESS = 0,          /* No error occurred */
    ACERR_DUPLICATE_PATTERN,    /* Duplicate patterns */
    ACERR_LONG_PATTERN,         /* Pattern length is longer than AC_PATTRN_MAX_LENGTH */
    ACERR_ZERO_PATTERN,         /* Empty pattern (zero length) */
    ACERR_AUTOMATA_CLOSED,      /* Automata is closed. after calling
                                 * ac_automata_finalize() you can not add new 
                                 * patterns to the automata. */
} AC_STATUS_t;

/* AC_MATCH_CALBACK_t:
 * This is the call-back function to report match back to the caller.
 * when a match is find, the automata will reach you using this function and sends
 * you a pointer to AC_MATCH_t. using that pointer you can handle
 * matches. you can send parameters to the call-back function when you call
 * ac_automata_search(). at call-back, the automata will sent you those
 * parameters as the second parameter (void *) of AC_MATCH_CALBACK_t. inside
 * the call-back function you can cast it to whatever you want.
 * If you return 0 from AC_MATCH_CALBACK_t function to the automata, it will
 * continue searching, otherwise it will return from ac_automata_search()
 * to your calling function.
**/
typedef int (*AC_MATCH_CALBACK_f)(AC_MATCH_t *, void *);

/* AC_PATTRN_MAX_LENGTH:
 * Maximum acceptable pattern length in AC_PATTERN_t.length
**/
#define AC_PATTRN_MAX_LENGTH 1024

#ifdef __cplusplus
}
#endif

#endif
