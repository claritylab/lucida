/*
 MeCab -- Yet Another Part-of-Speech and Morphological Analyzer

 $Id: mmap.h,v 1.3 2002/10/14 09:40:05 taku-ku Exp $;

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
#ifndef _MECAB_MMAP_H
#define _MECAB_MMAP_H

#include <errno.h>
#include <stdexcept>
#include <string>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

extern "C" {

#ifdef HAVE_SYS_TYPES_H   
#include <sys/types.h>
#endif

#ifdef HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif

#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif

#ifdef HAVE_STRING_H
#include <string.h>
#endif

#if defined (_WIN32) && !defined (__CYGWIN__)
#ifdef HAVE_WINDOWS_H
#include <windows.h>
#endif
#else
   
#ifdef HAVE_SYS_MMAN_H
#include <sys/mman.h>
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#endif
}

#ifndef O_BINARY
#define O_BINARY 0
#endif

#if !defined (_WIN32) || defined (__CYGWIN__)
static inline int open__ (const char* name, int flag) { return open (name, flag); }
static inline int close__ (int fd) { return close (fd); }
#endif

namespace YamCha
{
  template <class T>
  class Mmap 
  {
  private:
    T             *text;
    unsigned int  length;
    std::string        fileName;
    std::string        _what; 

#if defined (_WIN32) && !defined (__CYGWIN__)
    HANDLE hFile;
    HANDLE hMap;
#else
    int    fd;
    int    flag;
#endif

  public:
    T&       operator [] (unsigned int n)       { return *(text + n); }
    const T& operator [] (unsigned int n) const { return *(text + n); }
    T*       begin ()           { return text; }
    const T* begin ()    const  { return text; }
    T*       end   ()           { return text + size(); }
    const T* end   ()    const  { return text + size(); }
    unsigned int size ()        { return length/sizeof(T); }
    const char *what ()         { return _what.c_str(); }
    const char *getFileName ()  { return fileName.c_str(); }
    unsigned int getFileSize () { return length; }
    
    /*
     *  This code is imported from sufary, develoved by
     *  TATUO Yamashita <yto@nais.to> Thanks!
     */ 
#if defined (_WIN32) && !defined (__CYGWIN__)
    bool open (const char *filename, const char *mode = "r")
    {
      try {
	this->close ();
	unsigned long mode1, mode2, mode3;
	fileName = std::string (filename);

	if (strcmp (mode, "r") == 0) {
	  mode1 = GENERIC_READ;
	  mode2 = PAGE_READONLY;
	  mode3 = FILE_MAP_READ;
	} else if (strcmp (mode, "r+") == 0) {
	  mode1 = GENERIC_READ | GENERIC_WRITE;
	  mode2 = PAGE_READWRITE;
	  mode3 = FILE_MAP_ALL_ACCESS;
	} else {
	  throw std::runtime_error ("unknown open mode");
	}
 
	hFile = CreateFile (filename, mode1, FILE_SHARE_READ, 0,
			    OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
	if (hFile == INVALID_HANDLE_VALUE) 
	  throw std::runtime_error ("CreateFile() failed");

	length = GetFileSize (hFile, 0);

	hMap = CreateFileMapping (hFile, 0, mode2, 0, 0, 0);
	if (! hMap) throw std::runtime_error ("CreateFileMapping() failed");

	text = (T *)MapViewOfFile (hMap, mode3, 0, 0, 0);
	if (! text) throw std::runtime_error ("MapViewOfFile() failed");

	return true;
      }

      catch (std::exception &e) {
	 this->close();
	_what  = "Mmap::open(): " + std::string (filename) + " : " + e.what ();
	return false;
      }
    }

    bool close()
    {
      if (text) { UnmapViewOfFile (text); text = 0; }
      if (hFile != INVALID_HANDLE_VALUE) { CloseHandle (hFile); hFile = INVALID_HANDLE_VALUE; }
      if (hMap) { CloseHandle (hMap); hMap = 0; }

      return true;
    }

    Mmap (): text(0), hFile (INVALID_HANDLE_VALUE), hMap (0) {}
     
    Mmap (const char *filename, const char *mode = "r"):
     text(0), hFile (INVALID_HANDLE_VALUE), hMap (0)
    {
      if (! this->open (filename, mode))
	throw std::runtime_error (_what);
    }     

#else

    bool open (const char *filename, const char *mode = "r")
    {
      try {
	this->close ();
	struct stat st;
	fileName = std::string(filename);
	 
	if      (strcmp (mode, "r") == 0)  flag = O_RDONLY; 
	else if (strcmp (mode, "r+") == 0) flag = O_RDWR;
	else throw std::runtime_error ("unknown open mode");
	 
	if ((fd = open__ (filename, flag | O_BINARY)) < 0)
	  throw std::runtime_error ("open() failed");

	if (fstat (fd, &st) < 0)
	  throw std::runtime_error ("failed to get file size");
	
	length = (unsigned int)st.st_size;

#ifdef HAVE_MMAP
	int prot = PROT_READ;
	if (flag == O_RDWR) prot |= PROT_WRITE;
	char *p;
	if ((p = (char *)mmap (0, length, prot, MAP_SHARED, fd, 0)) == MAP_FAILED) 
	  throw std::runtime_error ("mmap() failed");
	text = reinterpret_cast<T *>(p);

#else
	text = new T [length];
	if (read (fd, text, length) < 0) 
	  throw std::runtime_error ("read() failed");
#endif
	close__ (fd);
	fd = -1;

	return true;
      }

      catch (std::exception &e) {
	 this->close();
	_what  = "Mmap::open(): " + std::string (filename) + " : " + e.what ();
	return false;
      }
    }

    bool close ()
    {
      if (fd >= 0) { close__ (fd); fd = -1; };
       
      if (text) {
	 
#ifdef HAVE_MMAP
	munmap ((char *)text, length);
#else
	if (flag == O_RDWR) { 
	  int fd2; 
	  if ((fd2 = open__ (fileName.c_str(), O_RDWR)) >= 0) {
	     write (fd2, text, length);
	     close__ (fd2);
	  }
	}
	delete [] text;
	text = 0;
#endif    
      }

      return true;
    }

    Mmap (): text(0), fd (-1) {}

     Mmap (const char *filename, const char *mode = "r"):
       text(0), fd (-1)
    {
      if (! this->open (filename, mode))
	throw std::runtime_error (_what);
    }
#endif      

    ~Mmap () { this->close (); }
  };
}
#endif
