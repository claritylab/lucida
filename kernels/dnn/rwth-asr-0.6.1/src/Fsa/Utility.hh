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
#ifndef _FSA_UTILITY_HH
#define _FSA_UTILITY_HH

#include <cstdlib>
#include <Core/Types.hh>
#include "Alphabet.hh"
#include "Hash.hh"
#include "Types.hh"
#include <Core/Vector.hh>

namespace Fsa {

	u32 estimateBytes(u32 x);
	void setBytes(Core::Vector<u8>::iterator i, u32 x, int nBytes);
	void appendBytes(Core::Vector<u8> &v, u32 x, int nBytes);
	u32 getBytesAndIncrement(Core::Vector<u8>::const_iterator &a, int nBytes);
	u32 getBytes(Core::Vector<u8>::const_iterator a, int nBytes);

	class LabelIdStrings {
	public:
		typedef u32 Id;
		typedef std::vector<Id> IdList;
		static const Id Empty;
		static const Id Invalid;
		typedef Core::Vector<LabelId> LabelIdList;
		typedef LabelIdList::const_iterator const_iterator;

	private:
		struct StringHashKey_ {
			LabelIdStrings &s_;
			StringHashKey_(LabelIdStrings &s) :
				s_(s) {
			}
			u32 operator()(Id s) {
				u32 value = 0;
				for (const_iterator i = s_.begin(s); *i != InvalidLabelId; ++i)
					value = 337 * value + abs(*i);
				return value;
			}
		};
		struct StringHashEqual_ {
			LabelIdStrings &s_;
			StringHashEqual_(LabelIdStrings &s) :
				s_(s) {
			}
			bool operator()(Id s1, Id s2) const {
				const_iterator i1 = s_.begin(s1), i2 = s_.begin(s2);
				for (; (*i1 != InvalidLabelId) && (*i1 == *i2); ++i1, ++i2);
				if (*i1 == *i2) return true; //check for different length
				// return (*i1 == InvalidLabelId) && (*i2 == InvalidLabelId);
				return false;
			}
		};
		typedef Hash<Id, StringHashKey_, StringHashEqual_> HashedStrings;
		HashedStrings hashedStrings_;
		LabelIdList strings_;

	public:
		LabelIdStrings() :
			hashedStrings_(StringHashKey_(*this), StringHashEqual_(*this)) {
			// strings_.push_back(InvalidLabelId);
		}

		Id start() const {
			return strings_.size();
		}

		Id insert(Id start) {
			std::pair<HashedStrings::Cursor, bool> found = hashedStrings_.insertExisting(start);
			return hashedStrings_[found.first];
		}

		Id stop(Id start) {
			strings_.push_back(InvalidLabelId);
			std::pair<HashedStrings::Cursor, bool> found =
					hashedStrings_.insertExisting(start);
			if (found.second)
				discard(start);
			return hashedStrings_[found.first];
		}

		void append(LabelId l) {
			strings_.push_back(l);
		}

		Id append(Core::Vector<LabelId>::const_iterator i);

		void discard(Id start) {
			strings_.resize(start);
		}

		const_iterator begin(Id s) const {
			return strings_.begin() + s;
		}

		u32 length(Id s) const;

		void dump(std::ostream &o, Id id,
				ConstAlphabetRef alphabet = ConstAlphabetRef()) const;

		size_t getMemoryUsed() const;
	};

	typedef std::pair<std::string, std::string> QualifiedFilename;
	QualifiedFilename splitQualifiedFilename(const std::string &file,
			const std::string &defaultFormat = "");
} // namespace Fsa

#endif // _FSA_UTILITY_HH
