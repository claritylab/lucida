/*
 YamCha -- Yet Another Multipurpose CHunk Annotator

 $Id: feature_index.h,v 1.6 2002/10/14 12:12:15 taku-ku Exp $;

 Copyright (C) 2001  Taku Kudoh <taku-ku.aist-nara.ac.jp>
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
#ifndef _YAMCHA_FEATUREINDEX_H
#define _YAMCHA_FEATUREINDEX_H

#include <vector>
#include <string>

namespace YamCha 
{
  class FeatureIndex
  {
  private:
    bool parse_start_end (const std::string &, int &, int &, int);
  public:
    bool setFeature (const std::string&, const std::string&);
    bool setFeature (const std::string&, int);
    unsigned int getColumnSize ();
    std::vector < std::pair <int, int> > features;
    std::vector <int> tags;
  };
}

#endif


