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

./regex list questions
