#pragma once

#include <map>
#include <string>
#include <fstream>
#include <algorithm>
using namespace std;

class Properties {

public:

    Properties ()  {}

    bool Read (const string& strFile) {
        ifstream is(strFile.c_str());
        if (!is.is_open()) return false;
        while (!is.eof()) {
            string strLine;
            getline(is,strLine);
            strLine.erase(remove_if(strLine.begin(), 
                                    strLine.end(), 
                                    [](char x){return isspace(x);}),
                          strLine.end());
            uint nPos = strLine.find('=');
            if (strLine.length() == 0 || strLine[0] == '#' || 
                strLine[0] == '!' || string::npos == nPos) continue;
            string strKey = strLine.substr(0,nPos);
            string strVal = strLine.substr(nPos + 1, strLine.length() - nPos + 1);
            m_map.insert(map<string,string>::value_type(strKey,strVal));
        }
        return true;
    }

    bool GetValue(const string& strKey, string& strValue) const {
        map<string,string>::const_iterator i;
        i = m_map.find(strKey);
        if (i != m_map.end()) {
            strValue = i->second;
            return true;
        }
        return false;
    }
    
protected:

    map<string,string> m_map;    
};