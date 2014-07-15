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
#include "Tokenizer.hh"
#include "StringUtilities.hh"

using namespace Core;

const std::string StringTokenizer::whiteSpace_ = " \t\n\r\f\v";

StringTokenizer::Iterator::Iterator()
    : parent_(), begin_(0), end_(0)
{}

StringTokenizer::Iterator::Iterator(const StringTokenizer *parent)
    : parent_(parent), begin_(0)
{
    end_ = findNext(begin_);
}

StringTokenizer::Iterator::Iterator(const StringTokenizer *parent,
			      size_type begin,
			      size_type end)
    : parent_(parent), begin_(begin), end_(end)
{}

inline StringTokenizer::Iterator::size_type StringTokenizer::Iterator::findStart(size_type begin) const
{
    return parent_->str_.find_first_not_of(parent_->delim_, begin);
}

inline StringTokenizer::Iterator::size_type StringTokenizer::Iterator::findNext(size_type begin) const
{
    return parent_->str_.find_first_of(parent_->delim_, begin);
}

StringTokenizer::Iterator& StringTokenizer::Iterator::operator++()
{
    begin_ = end_;
    if (begin_ != std::string::npos) {
	begin_ = findStart(begin_);
	end_ = findNext(begin_);
    }
    return *this;
}

StringTokenizer::Iterator StringTokenizer::Iterator::operator++(int)
{
    Iterator i(*this);
    i.begin_ = end_;
    if (end_ == std::string::npos) {
	i.begin_ = findStart(i.begin_);
	i.end_ = findNext(i.begin_);
    }
    return i;
}

std::string StringTokenizer::Iterator::operator*() const
{
    std::string result(parent_->str_, begin_, std::min(parent_->str_.size() - begin_, (end_ - begin_ )));
    if (parent_->trim_)
	stripWhitespace(result);
    return result;
}


StringTokenizer::StringTokenizer(const std::string &text, const std::string &delimiter, bool trim)
    : str_(text), delim_(delimiter), trim_(trim),
      endIterator_(this, std::string::npos, std::string::npos)
{}


StringTokenizer::StringTokenizer(const std::string &text)
    : str_(text), delim_(whiteSpace_), trim_(true),
      endIterator_(this, std::string::npos, std::string::npos)
{}

StringTokenizer::Iterator StringTokenizer::begin() const
{
    return Iterator(this);
}

const StringTokenizer::Iterator& StringTokenizer::end() const
{
    return endIterator_;
}

std::vector<std::string> StringTokenizer::operator() () const
{
    std::vector<std::string> tokens;
    for (Iterator itTok = begin(), endTok = end(); itTok != endTok; ++itTok)
	tokens.push_back(*itTok);
    return tokens;
}
