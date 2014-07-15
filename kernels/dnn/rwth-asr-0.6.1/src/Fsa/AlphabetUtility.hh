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
#ifndef _FSA_ALPHABET_UTILITY_HH
#define _FSA_ALPHABET_UTILITY_HH

#include "Alphabet.hh"
#include "Types.hh"
#include <Core/Vector.hh>

namespace Fsa
{
/**
 * An object the contains all information that is gathered by the count
 * function for alphabets.
 **/
class AlphabetCounts
{
public:
	LabelId maxLabelId_;
	LabelId nLabels_;
	LabelId nDisambiguators_;
public:
	AlphabetCounts() :
		maxLabelId_(0), nLabels_(0), nDisambiguators_(0) {}
};

/**
 * Calculates some interesting statistics of the size of an alphabet.
 * Complexity: O(I)
 * @param a the alphabet
 * @param progress set to true if function should show a progress indicator
 *    on current tty
 * @return returns a data structure containing the counts
 **/
AlphabetCounts count(ConstAlphabetRef a, bool progress = false);

/**
 * Calculates the number of disambiguators of an alphabet.
 * Complexity: O(I)
 * @param a the alphabet
 * @return the number of disambiguators
 **/
LabelId countDisambiguators(ConstAlphabetRef a);

/**
 * Represents the result of a call to one of the mapAlphabet
 * functions. This is a separate class and not simply a vector,
 * because:
 *
 * 1. we need additional information of the type of
 *    mapping (injective, one-to-one, etc.)
 * 2. seemlessly support all special labels
 **/
class AlphabetMapping : public Core::Vector<LabelId>
{
public:
	typedef u8 Type;
	static const Type typeUnmapped = 0x01; /**< all symbols have the same index in FROM and TO */
	static const Type typeComplete = 0x02; /**< all symbols in FROM are also in TO */
	static const Type typeIdentity = 0x04; /**< FROM and TO are identical (not just equal) */

private:
	friend void mapAlphabet(ConstAlphabetRef, ConstAlphabetRef,
			AlphabetMapping&, LabelId, u32);
	ConstAlphabetRef from_, to_;
	Type type_;
	LabelId unkId_;

public:
	AlphabetMapping();
	void clear();
	AlphabetMapping &operator=(const AlphabetMapping&);

	/**
	 * @return the type of the mapping
	 **/
	Type type() const
	{
		return type_;
	}

	/**
	 * @return true if this mapping requires an automaton to be
	 * modified.  False if this is a trivial mapping where all
	 * label ids remain unchanged.
	 **/
	bool isModifyingType() const
	{
		return (type_ & (typeUnmapped|typeComplete)) != (typeUnmapped
				|typeComplete);
	}

	/**
	 * @return a reference to the alphabet from which to map
	 **/
	ConstAlphabetRef from() const
	{
		return from_;
	}

	/**
	 * @return a reference to the alphabet to which we map
	 **/
	ConstAlphabetRef to() const
	{
		return to_;
	}

	/**
	 * Add a mapping for a single label id.
	 **/
	void map(LabelId from, LabelId to);

	/**
	 * Add a mapping to delete a single label id.
	 **/
	void del(LabelId);

	/**
	 * Request the target label id for a source label id. This
	 * method is aware of special label ids and does not map them.
	 * @return the target label id
	 **/
	const LabelId operator[](LabelId i) const
	{
		if (!isModifyingType())
			return i;
		if ((i < FirstLabelId) || (LastLabelId < i))
			return i;
		if (size_t(i) >= size())
			return unkId_;
		return Core::Vector<LabelId>::operator[](i);
	}
};

/**
 * Generally calculates the mapping between two alphabets. Since
 * alphabets match on symbol level, but algorithms work on integer
 * labels only, mapping label ids between alphabets is a necessary
 * step if two automata with different alphabets need to interact.
 * Complexity: the complexity depends on the implementation of the
 *    underlying alphabets. In case of two static alphabets the
 *    worst-case complexity is O(|from| log |to|)
 * @param from a reference to the alphabet to map from
 * @param to a reference to the alphabet to map to
 * @param mapping the resulting mapping (passed by reference to save
 *    overhead due to temporary object creation)
 * @param reportUnknowns maximum number of unknown symbols that should be reported
 **/
void mapAlphabet(ConstAlphabetRef from, ConstAlphabetRef to, AlphabetMapping &mapping,
		LabelId unknownId = InvalidLabelId, u32 reportUnknowns = 0);
} // namespace Fsa
#endif // _FSA_ALPHABET_UTILITY_HH
