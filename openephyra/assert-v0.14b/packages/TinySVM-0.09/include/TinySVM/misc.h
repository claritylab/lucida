/*
 TinySVM -- Yet Another Tiny SVM Package

 $Id: misc.h,v 1.12 2002/08/20 06:31:17 taku-ku Exp $;

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


#ifndef _MISC_H
#define _MISC_H

namespace TinySVM {
   
enum { LINEAR, POLY, NEURAL, RBF, ANOVA };
enum { DOUBLE_FEATURE, BINARY_FEATURE };
enum { SVM, SVR, ONE_CLASS };

struct feature_node
{
  int index;
  double value;
};
};
#endif

