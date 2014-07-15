# Project:      SPRINT
# File:		Options.make - configurable part of build environment
# Revision:     $Id: Options.make 8270 2011-06-17 09:49:50Z rybach $

# -----------------------------------------------------------------------------
# configurable setting: compile mode
#
# standard	- most debug error checks (i.e. assertion) and all release
#                 optimizations enabled. very fast, catches most programming
#                 errors, but inaccurate when being debugged using gdb.
#		  (high speed, moderate risk)
#                 Should be the default compile mode.
#
# release	- all assertion checking (debug checks) disabled, but
#                 only run-time checks (for user errors) enabled, fully
#                 optimized. very fast, but won't catch any programming
#		  errors.  (maximum speed, maximum risk)
#		  Should be used in production systems with proven set-ups
#
# debug		- all error checks enabled, no optimizations at all.
#		  very slow (about a factor of 4 compared to standard and
#		  release), but very safe against programming and user errors.
#		  very accurate when being debugged. (no risk, no fun;-)
# 	          Should be used when trying to find a bug (especially when
#		  using a debugger)
#
# Note that you can change the compile mode on the command line using, e.g.
# make COMPILE=debug

COMPILE			= standard
#COMPILE		= release
#COMPILE		= debug
#COMPILE		= debug_light

# -----------------------------------------------------------------------------
# configurable setting: debug level
#
# define a DBG_LEVEL value equal or greater than 0 to control the verboseness
# of your DBG(level) or DBGX(level) macro commands (see src/Core/Debug.hh).
# Only those debug commands whose level is lower or equal than the specified
# DBG_LEVEL will be compiled and executed, all other commands will be removed
# by the compiler.
#
# In release mode all debug commands are turned of by default.
#
DBG_LEVEL     = -1

# -----------------------------------------------------------------------------
# configurable setting: profiling mode
#
# none		- no support for run-time profiling
# gprof		- support for gprof
#		  (-pg, statically linked, profiling libc)
# valgrind	- support for valgrind/cachegrind
#		  (debugging symbols, no MMX code)
#
# Note that you can change the profiling mode on the command line using, e.g.
# make PROFILE=gprof
#

PROFILE			= none
#PROFILE		= gprof
#PROFILE		= valgrind

# -----------------------------------------------------------------------------
# configurable setting: nameing convention for binary files
#
# normal        - use plain names
# extended	- use suffixes for architecture, compile mode
#		  and profiling mode
#

#BINARYFILENAMES	= normal
BINARYFILENAMES		= extended

# -----------------------------------------------------------------------------
# configurable setting: used compiler suite
#
# gcc           - GNU Compiler Collection
# icc		- Intel(R) C / C++ Compiler
COMPILER                = gcc
# COMPILER              = icc

# -----------------------------------------------------------------------------
# configurable setting: install target path

INSTALL_TARGET		= $(TOPDIR)/arch/$(OBJEXT)
#INSTALL_TARGET		= /u/sprint

# Additional CC Flags
ADDCCFLAGS              =

# Additional Suffix for build-directories and executable extension
SUFFIX                  =

# Object build directory
# Empty string to use build directories in the source tree.
# .build/$(OBJDIR) are links to directories in $(OBJBUILDDIR)
OBJBUILDDIR             =
#OBJBUILDDIR           = $(TMPDIR)/build$(shell pwd)

