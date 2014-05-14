/*
 Darts -- Double-ARray Trie System

 $Id: darts.h,v 1.6 2003/01/01 10:14:37 taku-ku Exp $;

 Copyright (C) 2001  Taku Kudoh <taku-ku@is.aist-nara.ac.jp>
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <vector>
#include <cstring>
#include <cstdio>

#ifndef _DARTS_H
#define _DARTS_H

#ifdef HAVE_ZLIB_H
namespace zlib {
#include <zlib.h>
}
#define SH(p) ((unsigned short)(unsigned char)((p)[0]) | ((unsigned short)(unsigned char)((p)[1]) << 8))
#define LG(p) ((unsigned long)(SH(p)) | ((unsigned long)(SH((p)+2)) << 16))
#endif

namespace Darts {

#if 4 == 2
typedef short DartsInt;
typedef unsigned short DartsUInt;
#define DARTS_ARRAY_SIZE_IS_DEFINED 1  
#endif

#if 4 == 4 && ! defined (DARTS_ARRAY_SIZE_IS_DEFINED)
typedef int DartsInt;
typedef unsigned int DartsUInt;
#define DARTS_ARRAY_SIZE_IS_DEFINED 1
#endif

#if 4 == 4 && ! defined (DARTS_ARRAY_SIZE_IS_DEFINED)
typedef long DartsInt;
typedef unsigned long DartsUInt;
#define DARTS_ARRAY_SIZE_IS_DEFINED 1
#endif

#if 4 == 8 && ! defined (DARTS_ARRAY_SIZE_IS_DEFINED)
typedef long long DartsInt;
typedef unsigned long long DartsUInt;
#endif

#if 1 == 4 
typedef int DartsChar;
typedef unsigned int DartsUChar;
#define DARTS_NODE_SIZE_IS_DEFINED 1
#endif

#if 1 == 2 && ! defined (DARTS_NODE_SIZE_IS_DEFINED)
typedef short DartsChar;
typedef unsigned short DartsUChar;
#define DARTS_NODE_SIZE_IS_DEFINED 1
#endif
#define DARTS_LENGHT_FUNC Length<DartsChar>

#if 1 == 1 && ! defined (DARTS_NODE_SIZE_IS_DEFINED)
typedef char DartsChar;
typedef unsigned char DartsUChar;
#undef  DARTS_LENGTH_FUNC
#define DARTS_LENGTH_FUNC charLength
#endif

  template <class T> inline T _max (T x, T y) { return (x > y) ? x : y; }
  template <class T> inline T* _resize (T* ptr, size_t n, size_t l, T v)
  {
    T *tmp = new T [l]; // realloc
    for (size_t i = 0; i < n; i++) tmp[i] = ptr[i]; // copy
    for (size_t i = n; i < l; i++) tmp[i] = v;
    delete [] ptr;
    return tmp;
  }

  template <class T>
  class Length
  {
  public:
    size_t operator() (const T *str) const { size_t i; for (i = 0; str[i] != (T)0; i++) {}; return i; }
  };
  
  class charLength
  {
  public:
    size_t operator() (const char *str) const  { return std::strlen (str); };
  };

  template  <class NodeType,  class NodeUType,
             class ArrayType, class ArrayUType, class LengthFunc = Length<NodeType> >
  class DoubleArrayImpl
  {
  private:
    struct Node
    {
      ArrayUType code;
      size_t     depth;
      size_t     left;
      size_t     right;
    };

    struct Unit 
    {
      ArrayType    base;
      ArrayUType   check;
    };

    Unit           *array;
    size_t         *used;
    size_t         size;
    size_t         alloc_size;
    NodeType       **str;
    size_t         str_size;
    size_t         *len;
    ArrayType      *val;

    unsigned int   progress;
    size_t         next_check_pos;
    int            no_delete;
    int            (*progress_func) (size_t, size_t);

    size_t resize (const size_t new_size)
    {
      Unit tmp;
      tmp.base = 0;
      tmp.check = 0;
      
      array = _resize (array, alloc_size, new_size, tmp);
      used  = _resize (used,  alloc_size, new_size, (size_t)0);
      alloc_size = new_size;

      return new_size;
    }

    size_t fetch (const Node &parent, std::vector <Node> &children)
    {
      ArrayUType prev = 0;

      for (size_t i = parent.left; i < parent.right; i++) {
	if ((len ? len[i] : LengthFunc()(str[i])) < parent.depth) continue;

	const NodeUType *tmp = (NodeUType *)(str[i]);

	ArrayUType cur = 0;
	if ((len ? len[i] : LengthFunc()(str[i])) != parent.depth) 
	  cur = (ArrayUType)tmp[parent.depth] + 1;
	  
	if (prev > cur) throw -3;

	if (cur != prev || children.size() == 0) {
	  Node tmp_node;
	  tmp_node.depth = parent.depth + 1;
	  tmp_node.code  = cur;
	  tmp_node.left  = i;
	  if (children.size()) children[children.size()-1].right = i;

	  children.push_back(tmp_node);
	}

	prev = cur;
      }

      if (children.size())
	children[children.size()-1].right = parent.right;

      return children.size();
    }
     
    size_t insert (const std::vector <Node> &children)
    {
      size_t begin       = 0;
      size_t pos         = _max ((size_t)children[0].code + 1, next_check_pos) - 1;
      size_t nonzero_num = 0;
      int   first        = 0;

      while (1) {
      next:
	pos++;

	if (array[pos].check) {
	  nonzero_num++;
	  continue;
	} else if (! first) {
	  next_check_pos = pos;
	  first = 1;
	}

	begin = pos - children[0].code;
	if (alloc_size < (begin + children[children.size()-1].code))
	  resize((size_t)(alloc_size * _max(1.05, 1.0 * str_size / progress)));

	if (used[begin]) continue;

	for (size_t i = 1; i < children.size(); i++)
	  if (array[begin + children[i].code].check != 0) goto next;

	break;
      }

      // -- Simple heuristics --
      // if the percentage of non-empty contents in check between the index
      // 'next_check_pos' and 'check' is grater than some constant value (e.g. 0.9),
      // new 'next_check_pos' index is written by 'check'.
      if (1.0 * nonzero_num/(pos - next_check_pos + 1) >= 0.95) next_check_pos = pos;

      used[begin] = 1;
      size = _max (size, (size_t)begin + children[children.size()-1].code + 1);

      for (size_t i = 0; i < children.size(); i++)
	array[begin + children[i].code].check = begin;

      for (size_t i = 0; i < children.size(); i++) {
	std::vector <Node> new_children;

	if (! fetch(children[i], new_children)) {
	  array[begin + children[i].code].base = 
	    val ? (ArrayType)(-val[children[i].left]-1) : (ArrayType)(-children[i].left-1);

	  if (val && (ArrayType)(-val[children[i].left]-1) >= 0) throw -2;

	  ++progress;
	  if (progress_func) (*progress_func) (progress, str_size);

	} else {
	  size_t h = insert(new_children);
	  array[begin + children[i].code].base = h;
	}
      }

      return begin;
    }

  public:

    DoubleArrayImpl (): array(0), used(0), size(0), alloc_size(0), no_delete(0) {};

    ~DoubleArrayImpl ()
    {
      if (! no_delete) delete [] array;
      delete [] used;
    }

    int setArray (void *ptr)
    {
      clear();
      array = (Unit *)ptr;
      no_delete = 1;
      return 1;
    }

    void *getArray()
    {
      return (void *)array;
    }

    void clear ()
    {
      if (! no_delete) delete [] array;
      delete [] used;
      array      = 0;
      used       = 0;
      alloc_size = 0;
      size       = 0;
      no_delete  = 0;
    }
    
    size_t getUnitSize() { return sizeof(Unit); };
    
    size_t getSize() { return size; };

    size_t getNonzeroSize ()
    {
      size_t result = 0;
      for (size_t i = 0; i < size; i++)
	if (array[i].check) result++;
      return result;
    }

    int build (const size_t  _str_size,
	       NodeType      **_str,
	       size_t        *_len = 0,
	       ArrayType     *_val = 0,
	       int (*_progress_func)(size_t, size_t) = 0)
    {
      try {
	if (!_str_size || ! _str) return 0;

	progress_func  = _progress_func;
	str            = _str;
	len            = _len;
	str_size       = _str_size;
	val            = _val;
	progress       = 0;

	resize (1024 * 10);

	array[0].base = 1;
	next_check_pos = 0;

	Node root_node;
	root_node.left  = 0;
	root_node.right = _str_size;
	root_node.depth = 0;

	std::vector <Node> children;
	fetch (root_node, children);
	insert (children);
	delete [] used;
	used  = 0;
	return 0;
      }

      catch (int &e) {
	delete [] used;
	used  = 0;
	clear ();
	return e;
      }
      
      catch (...) {
	delete [] used;
	used  = 0;
	clear ();
	return -1;
      }
    }

    int open (const char *file,
	      const char *mode = "rb",
	      size_t offset = 0,
	      size_t _size = 0)
    {
      std::FILE *fp = std::fopen(file, mode);
      if (! fp) return -1;
      if (std::fseek (fp, offset, SEEK_SET) != 0) return -1;

      if (! _size) { 
	if (std::fseek (fp, 0L,     SEEK_END) != 0) return -1;
	_size = std::ftell (fp);
	if (std::fseek (fp, offset, SEEK_SET) != 0) return -1;
      }

      int r = open (fp, _size);
      std::fclose (fp);
      return r;
    }

    int open (std::FILE *fp, size_t _size = 0)
    {
      if (! fp || ! _size) return -1;
      clear();

      size = _size;
      size /= sizeof(Unit);
      array  = new Unit [size];
      if (size != std::fread((Unit *)array,  sizeof(Unit), size, fp)) return -1;

      return 0;
    }

    int save (const char *file,
	      const char *mode = "wb",
	      size_t offset = 0)
    {
      std::FILE *fp = std::fopen(file, mode);
      int r = save (fp);
      std::fclose (fp);
      return r;
    }

    int save (std::FILE *fp)
    {
      if (! size || !fp ||
	  size != std::fwrite((Unit *)array,  sizeof(Unit), size, fp))
	return -1;

      return 0;
    }
    
    ArrayType exactMatchSearch (const NodeType *key,
                size_t len = 0, 
		size_t pos = 0)
    {
      if (! len) len = LengthFunc() (key);

      register ArrayType  b = array[pos].base;
      register ArrayUType p;

      for (register size_t i = 0; i < len; i++) {
	p = b + (NodeUType)(key[i]) + 1;
	if ((ArrayUType)b == array[p].check) b = array[p].base;
	else return -1;
      }

      p = b;
      ArrayType n = array[p].base;
      if ((ArrayUType)b == array[p].check && n < 0) return -n-1;

      return -1;
    }

    size_t commonPrefixSearch (const NodeType *key,
                              ArrayType *result,
			      size_t len = 0,
			      size_t pos = 0)
    {
      if (! len) len = LengthFunc() (key);

      register ArrayType  b   = array[pos].base;
      register size_t     num = 0;
      register ArrayType  n;
      register ArrayUType p;

      for (register size_t i = 0; i < len; i++) {
	p = b; // + 0;
	n = array[p].base;
	if ((ArrayUType) b == array[p].check && n < 0) result[num++] = -n-1;

	p = b + (NodeUType)(key[i]) + 1;
	if ((ArrayUType) b == array[p].check) b = array[p].base;
	else return num;
      }

      p = b;
      n = array[p].base;
      if ((ArrayUType)b == array[p].check && n < 0) result[num++] = -n-1;

      return num;
    }

#ifdef HAVE_ZLIB_H
    int gzopen (const char *file, size_t _size = 0)
    {
      std::FILE *fp  = std::fopen (file, "rb");
      if (! fp) return -1;
      clear();
      
      size = _size;
      if (! size) {
        if (-1L != (long)std::fseek (fp, (-8), SEEK_END)) {
	  char buf [8];
	  if (std::fread ((char*)buf, 1, 8, fp) != sizeof(buf)) {
	    std::fclose(fp);
	    return -1;
	  }
	  size = LG (buf+4);
	  size /= sizeof (Unit);
        }
      }
      std::xfclose(fp);

      if (! size) return -1;

      zlib::gzFile gzfp = zlib::gzopen(file, "rb");
      if (gzfp) {
	array = new Unit [size];
	std::gzread (gzfp, (Unit *)array,  sizeof(Unit) * size);
	std::gzclose (gzfp);
	return 0;
      }

      return -1;
    }

    int gzsave (const char *file)
    {
      zlib::gzFile gzfp = zlib::gzopen (file, "wb");
      if (gzfp) {
      	zlib::gzwrite (gzfp, (Unit *)array,  sizeof(Unit) * size);
	zlib::gzclose (gzfp);
	return 0;
      }

      return -1;
    }
#endif    

  };
   
  typedef Darts::DoubleArrayImpl<DartsChar, DartsUChar, DartsInt, DartsUInt, DARTS_LENGTH_FUNC> DoubleArray;
}
#endif
