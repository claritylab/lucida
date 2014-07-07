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
#ifndef _CORE_TOKENIZER_HH
#define _CORE_TOKENIZER_HH

#include <string>
#include <vector>

namespace Core
{

    /**
     * Splits a string in tokens
     *
     * The interface is somehow inspired by the StringTokenizer of
     * the Common C++ Library
     */
    class StringTokenizer
    {
    public:
	class Iterator
	{
	public:
	    Iterator();

	     bool operator==(const Iterator &rhs) const;
	     bool operator!=(const Iterator &rhs) const;

	    Iterator& operator++();
	    Iterator  operator++(int);

	    std::string operator*() const;

	private:
	    typedef std::string::size_type size_type;

	    const StringTokenizer *parent_;
	    size_type begin_;
	    size_type end_;

	    size_type findStart(size_type begin) const;
	    size_type findNext(size_type begin) const;

	    explicit Iterator(const StringTokenizer *parent);
	    Iterator(const StringTokenizer *parent, size_type begin, size_type end);

	    friend class StringTokenizer;
	};

    public:
	StringTokenizer(const std::string &text, const std::string &delimiter, bool trimTokens = false);
	StringTokenizer(const std::string &text);

	Iterator begin() const;
	const Iterator& end() const;

	std::vector<std::string> operator() () const;

    private:
	const std::string &str_;
	/* do no use a reference, because
	 * delim is often a temporay std::string
	 * constructed from a const char *
	 */
	const std::string delim_;
	bool trim_;
	const Iterator endIterator_;

	static const std::string whiteSpace_;

	friend class Iterator;
    };


    // ================================================================


    inline bool StringTokenizer::Iterator::operator==(const Iterator &rhs) const
    {
	return (end_ == rhs.end_ && begin_ == rhs.begin_ && parent_ == rhs.parent_);
    }

    inline bool StringTokenizer::Iterator::operator!=(const Iterator &rhs) const
    {
	return !(*this == rhs);
    }

}

#endif // _CORE_TOKENIZER_HH
