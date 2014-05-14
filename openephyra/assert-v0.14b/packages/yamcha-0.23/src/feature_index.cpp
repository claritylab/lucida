/*
  YamCha -- Yet Another Multipurpose CHunk Annotator

 $Id: feature_index.cpp,v 1.5 2002/10/21 12:03:07 taku-ku Exp $;

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
#include <functional>
#include <algorithm>
#include <iostream>
#include <stdexcept>
#include <iterator>
#include "feature_index.h"
#include "common.h"

namespace YamCha 
{
  bool FeatureIndex::parse_start_end (const std::string &src, 
					  int &start, int &end, int end_default)
  {
    char *ptr = (char *)src.c_str();

    start = str2int (ptr);

    unsigned int pos = src.find ("..");
    if (pos == std::string::npos) { // not found ..
      end = start;
      return true;
    } else if (pos == (src.size()-2)) { // foud but .. style
      end = end_default;
      if (end < start) return false;
      return true;
    } else {
      ptr += (pos + 2);
      end = str2int (ptr);
      if (end < start) return false; // error, start > end  }
    }

    return true;
  }

  unsigned int FeatureIndex::getColumnSize () 
  {
    unsigned int c = 0;
    for (unsigned int i = 0; i < features.size(); i++) 
      c = _max (features[i].second + 1, static_cast<int>(c));
    return c;
  }

  bool FeatureIndex::setFeature(const std::string &feature, const std::string &tag)
  {
    try {
      features.clear();
      tags.clear();

      std::vector <std::string> tmp;

      split_string(feature, "\t ", tmp);
      for (unsigned int i = 0; i < tmp.size(); i++) {
	std::vector <std::string> tmp2;
	if (split_string(tmp[i],":",tmp2) != 2) throw std::runtime_error (feature);
	features.push_back(std::make_pair<int,int>(str2int (tmp2[0].c_str()), str2int (tmp2[1].c_str())));
      }

      split_string(tag, "\t ", tmp);
      for (unsigned int i = 0; i < tmp.size(); i++) {
	int row = str2int (tmp[i].c_str());
	tags.push_back(row);
      }

      return true;
    }

    catch (std::exception &e) {
      throw std::runtime_error (std::string ("FeatureIndex::setFeature() format error: ") + e.what ());
      return false;
    }
  }

  bool FeatureIndex::setFeature(const std::string& str, int max_col)
  {
    try {

      std::vector <std::string> list, rclist, item;
      std::vector <int> row, col, tag;
      int start,end;

      features.clear();
      tags.clear();

      if (! split_string (str,"\t ",list)) throw std::runtime_error (str);

      for (unsigned int i = 0; i < list.size(); i++) {
	int size = split_string(list[i],":",rclist);

	if (size == 3 && (rclist[0] == "F" || rclist[0] == "f")) {

	  if (! split_string (rclist[1],",",item)) 
	    throw std::runtime_error (list[i]);

	  for (unsigned int j = 0; j < item.size(); j++) {
	    if (! parse_start_end (item[j],start,end, max_col-1)) 
	      throw std::runtime_error (list[i]);
	    for (int j = start; j <= end; j++) row.push_back(j);
	  }

	  if (! split_string(rclist[2],",",item)) 
	    throw std::runtime_error (list[i]);

	  for (unsigned int j = 0; j < item.size(); j++) {
	    if (! parse_start_end (item[j],start,end,max_col - 1) || start < 0) 
	      throw std::runtime_error (list[i]);
	    if (max_col-1 < end) 
	      throw std::runtime_error (list[i]);
	    for (int j = start; j <= end; j++) col.push_back(j);
	  }

	} else if (size == 2 && (rclist[0] == "T" || rclist[0] == "t")) {
	  if (! split_string (rclist[1],",",item)) 
	    throw std::runtime_error (list[i]);
	  for (unsigned int j = 0; j < item.size(); j++) {
	    if (! parse_start_end (item[j],start,end,0) || start * end <= 0 ) 
	      throw std::runtime_error (list[i]);
	    for (int j = start; j <= end; j++) tag.push_back(j);
	  }
	} else if (list[i] == " " || list[i] == "") {
	  // do nothing
	} else {
	  throw std::runtime_error (list[i]);
	}
      }

      std::sort (row.begin(), row.end()); 
      std::vector <int>::iterator rlast = std::unique (row.begin(),row.end());
      std::sort (col.begin(),col.end()); 
      std::vector <int>::iterator clast = std::unique (col.begin(),col.end());
    
      for (std::vector<int>::iterator i = row.begin(); i < rlast ; i++) 
	for (std::vector<int>::iterator j = col.begin(); j < clast ; j++) 
	  features.push_back (std::make_pair <int, int>(*i, *j));

      std::sort(tag.begin(),tag.end()); 
      std::vector <int>::iterator tlast = std::unique (tag.begin(),tag.end());
      for (std::vector <int>::iterator i = tag.begin(); i < tlast; i++) {
	tags.push_back(*i);
      }
    }

    catch (std::exception &e) {
      throw std::runtime_error (std::string ("FeatureIndex::setFeature() format error: ") + e.what ());
      return false;
    }

    return true;
  }
}
