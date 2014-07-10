SLRE: Super Light Regular Expression library
============================================

SLRE is an ISO C library that implements a subset of Perl regular
expression syntax. Main features of SLRE are:

   * Written in strict ANSI C'89
   * Small size (compiled x86 code is about 5kB)
   * Uses little stack and does no dynamic memory allocation
   * Provides simple intuitive API
   * Implements most useful subset of Perl regex syntax (see below)
   * Easily extensible. E.g. if one wants to introduce a new
metacharacter `\i`, meaning "IPv4 address", it is easy to do so with SLRE.

SLRE is perfect for tasks like parsing network requests, configuration
files, user input, etc, when libraries like [PCRE](http://pcre.org) are too
heavyweight for the given task. Developers of embedded systems would benefit
most.

## Supported Syntax

    (?i)    Must be at the beginning of the regex. Makes match case-insensitive
    ^       Match beginning of a buffer
    $       Match end of a buffer
    ()      Grouping and substring capturing
    \s      Match whitespace
    \S      Match non-whitespace
    \d      Match decimal digit
    +       Match one or more times (greedy)
    +?      Match one or more times (non-greedy)
    *       Match zero or more times (greedy)
    *?      Match zero or more times (non-greedy)
    ?       Match zero or once (non-greedy)
    x|y     Match x or y (alternation operator)
    \meta   Match one of the meta character: ^$().[]*+?|\
    \xHH    Match byte with hex value 0xHH, e.g. \x4a
    [...]   Match any character from set. Ranges like [a-z] are supported
    [^...]  Match any character but ones from set

Under development: Unicode support.

./regex list questions
