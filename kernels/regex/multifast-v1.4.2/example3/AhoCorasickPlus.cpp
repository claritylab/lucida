/*
 * AhoCorasickPlus.cpp: This is the implementation file for a sample 
 * C++ wrapper for Aho-Corasick C library 
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

#include "ahocorasick.h"
#include "AhoCorasickPlus.h"

AhoCorasickPlus::AhoCorasickPlus ()
{
    m_automata = ac_automata_init ();
    m_acText = new AC_TEXT_t;
}

AhoCorasickPlus::~AhoCorasickPlus ()
{
    ac_automata_release (m_automata);
    delete m_acText;
}

AhoCorasickPlus::EnumReturnStatus AhoCorasickPlus::addPattern (const std::string &pattern, PatternId id)
{
    EnumReturnStatus rv = RETURNSTATUS_FAILED;

    AC_PATTERN_t tmp_patt;
    tmp_patt.astring = (AC_ALPHABET_t*) pattern.c_str();
    tmp_patt.length = pattern.size();
    tmp_patt.rep.number = id;

    AC_STATUS_t status = ac_automata_add (m_automata, &tmp_patt);
    
    switch (status)
    {
        case ACERR_SUCCESS:             rv = RETURNSTATUS_SUCCESS; break;
        case ACERR_DUPLICATE_PATTERN:   rv = RETURNSTATUS_DUPLICATE_PATTERN; break;
        case ACERR_LONG_PATTERN:        rv = RETURNSTATUS_LONG_PATTERN; break;
        case ACERR_ZERO_PATTERN:        rv = RETURNSTATUS_ZERO_PATTERN; break;
        case ACERR_AUTOMATA_CLOSED:     rv = RETURNSTATUS_AUTOMATA_CLOSED; break;
    }
    return rv;
}

AhoCorasickPlus::EnumReturnStatus AhoCorasickPlus::addPattern (const char pattern[], PatternId id)
{
    std::string tmpString = pattern;
    return addPattern (tmpString, id);
}

void AhoCorasickPlus::finalize ()
{
    ac_automata_finalize (m_automata);
}

void AhoCorasickPlus::search (std::string& text, bool keep)
{
    m_acText->astring = text.c_str();
    m_acText->length = text.size();
    ac_automata_settext (m_automata, m_acText, (int)keep);
}

bool AhoCorasickPlus::findNext (Match& match)
{
    if (m_matchQueue.size()>0)
    {
        match = m_matchQueue.front();
        m_matchQueue.pop();
        return true;
    }
    
    AC_MATCH_t * matchp;
    
    if ((matchp = ac_automata_findnext (m_automata)))
    {
        Match singleMatch;
        singleMatch.position = matchp->position;
        
        for (unsigned int j=0; j < matchp->match_num; j++)
        {
            singleMatch.id = matchp->patterns[j].rep.number;
            // we ignore tmp_patt.astring it may have been invalidated
            m_matchQueue.push(singleMatch);
        }
    }
    
    if (m_matchQueue.size()>0)
    {
        match = m_matchQueue.front();
        m_matchQueue.pop();
        return true;
    }

    return false;
}
