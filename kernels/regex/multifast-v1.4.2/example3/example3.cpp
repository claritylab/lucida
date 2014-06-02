/*
 * example3.cpp: This program illustrates how to use ahocorasick library 
 * with a C++ wrapper
 * 
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

#include <iostream>
using std::cout;
using std::endl;
#include <string>
using std::string;

#include "AhoCorasickPlus.h"

std::string sample_patterns[] = {
    "city",
    "clutter",
    "ever",
    "experience",
    "neo",
    "one",
    "simplicity",
    "utter",
    "whatever",
};
#define PATTERN_COUNT (sizeof(sample_patterns)/sizeof(std::string))

std::string input_text1 = "experience the ease and simplicity of multifast";
std::string input_text2 = "whatever you are be a good one";
std::string input_text3 = "out of clutter, find simplicity";


int main (int argc, char ** argv)
{
    AhoCorasickPlus atm;

    for (unsigned int i=0; i<PATTERN_COUNT; i++)
    {
        AhoCorasickPlus::EnumReturnStatus status;
        AhoCorasickPlus::PatternId patId = i;
        status = atm.addPattern(sample_patterns[i], patId);
        if (status!=AhoCorasickPlus::RETURNSTATUS_SUCCESS)
        {
            cout << "Failed to add: " << sample_patterns[i] << endl;
        }
    }
    atm.finalize();
    
    AhoCorasickPlus::Match aMatch;

    cout << "Searching '" << input_text1 << "'" << endl;
    atm.search(input_text1, false);
    while (atm.findNext(aMatch))
    {
        cout << "@" << aMatch.position << "\t#" << aMatch.id << "\t" << sample_patterns[aMatch.id] << endl;
    }
    
    cout << "Searching '" << input_text2 << "'" << endl;
    atm.search(input_text2, false);
    while (atm.findNext(aMatch))
    {
        cout << "@" << aMatch.position << "\t#" << aMatch.id << "\t" << sample_patterns[aMatch.id] << endl;
    }
    
    cout << "Searching '" << input_text3 << "'" << endl;
    atm.search(input_text3, true); // try it with keep flag disabled
    while (atm.findNext(aMatch))
    {
        cout << "@" << aMatch.position << "\t#" << aMatch.id << "\t" << sample_patterns[aMatch.id] << endl;
    }

    return 0;
}
