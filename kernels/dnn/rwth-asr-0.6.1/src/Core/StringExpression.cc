// Copyright 2011 RWTH Aachen University. All rights reserved.
//
// Licensed under the RWTH ASR License (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.hltpr.rwth-aachen.de/rwth-asr/rwth-asr-license.html
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
#include "StringExpression.hh"

using namespace Core;

/* StringExpression */

/*********************************************************************************************/
void StringExpression::setValue(const TokenReferences &references, const std::string &value)
/*********************************************************************************************/
{

    TokenReferences::const_iterator r;
    for(r = references.begin(); r != references.end(); ++ r)
	tokens_[*r] = value;
}

/*********************************************************************************************/
void StringExpression::clear(const TokenReferences &references)
/*********************************************************************************************/
{

    TokenReferences::const_iterator r;
    for(r = references.begin(); r != references.end(); ++ r)
	tokens_[*r].clear();
}

/*********************************************************************************************/
void StringExpression::pushBackToken(const std::string &token)
/*********************************************************************************************/
{
    ensure(!token.empty());
    tokens_.push_back(token);
}

/*********************************************************************************************/
void StringExpression::pushBackVariableToken(const std::string &token)
/*********************************************************************************************/
{
    ensure(!token.empty());
    tokens_.push_back(Token());
    variables_[token].push_back(tokens_.size() - 1);
}


/*********************************************************************************************/
bool StringExpression::setVariable(const std::string& name, const std::string& value)
    /*********************************************************************************************/
{
    Variables::iterator v = variables_.find(name);
    if (v != variables_.end()) {
	setValue(v->second, value);
	return true;
    }
    return false;
}

/*********************************************************************************************/
bool StringExpression::setVariables(const Core::Configuration &c)
/*********************************************************************************************/
{
    bool success = true;
    for(Variables::iterator v = variables_.begin(); v != variables_.end(); ++ v) {
	std::string value;
	if (c.get(v->first, value))
	    setValue(v->second, value);
	else
	    success = false;
    }
    return success;
}

/*********************************************************************************************/
bool StringExpression::clear(const std::string& name)
/*********************************************************************************************/
{
    Variables::iterator v = variables_.find(name);
    if (v != variables_.end()) {
	clear(v->second);
	return true;
    }
    return false;
}

/*********************************************************************************************/
void StringExpression::clear()
/*********************************************************************************************/
{
    for(Variables::iterator v = variables_.begin(); v != variables_.end(); ++ v)
	clear(v->second);
}

/*********************************************************************************************/
bool StringExpression::value(std::string &v) const
/*********************************************************************************************/
{
    std::string result;
    for(Tokens::const_iterator t = tokens_.begin(); t != tokens_.end(); ++ t) {
	if (t->set())
	    result += t->value();
	else
	    return false;
    }
    v = result;
    return true;
}


/* StringExpressionParser */

/*********************************************************************************************/
bool StringExpressionParser::accept(const std::string &expression)
/*********************************************************************************************/
{
    clear();

    std::string::size_type pos = 0, begin, end;
    while(pos < expression.size()) {
	begin = expression.find(openTag_, pos);
	if (begin == std::string::npos) {
	    toBuild_.pushBackToken(expression.substr(pos));
	    pos = begin;
	} else {
	    if (begin != pos)
		toBuild_.pushBackToken(expression.substr(pos, begin - pos));
	    pos = begin + openTag_.size();
	    end = expression.find(closeTag_, pos);
	    if (end == std::string::npos || end == pos)
		return false;
	    toBuild_.pushBackVariableToken(expression.substr(pos, end - pos));
	    pos = end + closeTag_.size();
	}
    }
    return true;
}


/*********************************************************************************************/
void StringExpressionParser::clear()
/*********************************************************************************************/
{
    toBuild_.tokens_.clear();
    toBuild_.variables_.clear();
}


/*********************************************************************************************/
StringExpression Core::makeStringExpression(const std::string &expression,
					    const std::string &openTag,
					    const std::string &closeTag)
/*********************************************************************************************/
{
    StringExpression result;
    StringExpressionParser parser(result, openTag, closeTag);

    parser.accept(expression);
    return result;
}
