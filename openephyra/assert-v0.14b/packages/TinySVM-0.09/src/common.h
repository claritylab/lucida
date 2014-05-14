/*
 TinySVM -- Yet Another Tiny SVM Package

 $Id: common.h,v 1.36 2002/08/20 06:31:16 taku-ku Exp $;

 Copyright (C) 2001-2002  Taku Kudo <taku-ku@is.aist-nara.ac.jp>
 All rights reserved.

 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Library General Public
 License as published by the Free Software Foundation; either
 version 2 of the License, or (at your option) any later verjsion.

 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Library General Public License for more details.

 You should have received a copy of the GNU Library General Public
 License along with this library; if not, write to the
 Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 Boston, MA 02111-1307, USA.
*/


#ifndef _COMMON_H
#define _COMMON_H

#define COPYRIGHT  "TinySVM - tiny SVM package\nCopyright (C) 2000-2002 Taku Kudo, All rights reserved.\n"

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef STDC_HEADERS
#include <stdio.h>
#endif

#ifdef HAVE_STDIO_H
#include <stdio.h>
#endif

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

#ifdef HAVE_CTYPE_H
#include <ctype.h>
#endif

#ifdef HAVE_STRING_H
#include <string.h>
#endif

#ifdef HAVE_MATH_H
#include <math.h>
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#ifdef HAVE_SYS_TIMES_H
#include <sys/times.h>
#endif

#ifdef HAVE_WINDOWS_H
#include <windows.h>
#endif

#ifdef HAVE_GETOPT_H
#include <getopt.h>
#endif

#if defined HAVE_GETOPT_H && defined HAVE_GETOPT_LONG
#include <getopt.h>
#else
#include "getopt.h"
#endif

#ifndef HAVE_MEMSET
#ifdef HAVE_BZERO
#define memset(a,b,c) bzero((a),(c))
#else
template <class T> inline void memset(T* &a, int b, int c)
{
   for (int i = 0; i < c; i++) a[i] = b;
}
#endif
#endif

#ifndef HAVE_MEMCPY
#ifdef HAVE_BCOPY
#define memcpy(a,b,c) bcopy((b),(a),(c))
#else
template <class T> inline void memcpy(T* &a, T &b, int c)
{
   for (int i = 0; i < c; i++) a[i] = b[i];
}
#endif
#endif

#define EPS_A 1e-12
#ifndef HUGE_VAL
#define HUGE_VAL 1e+37
#endif

#define INF HUGE_VAL
#define MAXLEN 1024

#ifndef EXIT_FAILURE
#define EXIT_FAILURE 1
#endif

#ifndef EXIT_SUCCESS
#define EXIT_SUCCESS 0
#endif

// Factory Functions
namespace TinySVM {
   
template <class T> inline T _min (T x, T y) { return (x < y) ? x : y; }
template <class T> inline T _max (T x, T y) { return (x > y) ? x : y; }
template <class T> inline void _swap (T &x, T &y) { T z = x; x = y; y = z; }

template <class S, class T> inline void _clone (T*& dst, S* src, int n)
{
  dst = new T [n];
  memcpy ((void *)dst, (void *)src, sizeof(T)*n);
}

// resize ptr from n to l
template <class T> inline T* _resize (T* ptr, int n, int l, T v)
{
  T *dst = new T [l];

  memcpy ((void *)dst, (void *)ptr, sizeof(T)*n);
  for (int i = n; i < l; i++) dst[i] = v;
  delete [] ptr;
  return dst;
}

// increse size of array automatically, simple implimentation
// you have to incriment N by yourself
template <class T> inline T* _append (T* ptr, int n, T v1, T v2)
{
  if (n % MAXLEN == 0) ptr = _resize (ptr, n, n + MAXLEN, v2);
  ptr[n] = v1;

  return ptr;
}
}

#endif
