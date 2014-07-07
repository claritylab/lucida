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
// $Id: Alignment.hh 9621 2014-05-13 17:35:55Z golik $

#ifndef _SPEECH_ALIGNMENT_HH
#define _SPEECH_ALIGNMENT_HH


#include "Types.hh"
#include <Core/BinaryStream.hh>
#include <Core/XmlStream.hh>
#include <Mm/Types.hh>
#include <Mc/Types.hh>
#include <Fsa/Semiring.hh>
#include <Am/ClassicStateModel.hh>


namespace Speech {

    struct AlignmentItem {
	TimeframeIndex time;
	Fsa::LabelId emission;
	Mm::Weight weight;
	AlignmentItem() : time(0), emission(0), weight(0) {}
	AlignmentItem(TimeframeIndex t, Am::AllophoneStateIndex e, Mm::Weight w=1.0) :
	    time(t), emission(e), weight(w) {}
	bool operator==(const AlignmentItem &other) const {
	    return (time == other.time && emission == other.emission && weight == other.weight);
	}
    };


    // ================================================================================

    class Alignment;
    Core::BinaryInputStream  &operator>>(Core::BinaryInputStream&,  Alignment&);
    Core::BinaryOutputStream &operator<<(Core::BinaryOutputStream&, const Alignment&);

    class Alignment : public std::vector<AlignmentItem> {
    public:
	typedef std::pair<Alignment::iterator, Alignment::iterator> Frame;
    private:
	static const char *magic;
	static const char *magic_alphabet;
	static const size_t magicSize;
	Score score_;
	Core::Ref<const Fsa::Alphabet> alphabet_;
	// If the archive was read, then this contains the read alphabet information (cleared when the mapping is applied)
	std::map<u32, std::string> archiveAlphabet_;
	void mapAlphabet(bool skipMismatch);
    public:
	Alignment();

	void setScore(Score score) { score_ = score; }

	/** Returns the score of the alignment, if available. */
	Score score() const { return score_; }

	/** Set a mapping alphabet. When this is set, the alphabet is used to robustly map
	 *  allophone indices between different alphabets.
	 *  If skipMismatch is true, then alignment items which could not be mapped into the new
	 *  alphabet are simply removed. Otherwise an error is raised for such items. */
	bool setAlphabet(Core::Ref<const Fsa::Alphabet> alphabet, bool skipMismatch = false);

	/** Returns true, if at least one alignment item has a weight different from one. */
	bool hasWeights() const;

	/** Sorts the alignment items, such that timeframes have ascending order and
	 *  secondly weight have descending order. */
	void sortItems(bool byDecreasingWeight = true);
	void sortStableItems();

	/** Combines all alignment items which differ only in their weight. */
	void combineItems(Fsa::ConstSemiringRef sr = Fsa::TropicalSemiring);

	/** Builds weights from negative logarithm of item weights. */
	void expm();

	/** Adds @param weight to each item weight. */
	void addWeight(Mm::Weight weight);

	/** Filter alignment items by their weights. */
	void filterWeights(Mm::Weight minWeight, Mm::Weight maxWeight=1.0);

	/** Normalize weights, such that for each timeframe the sum of weights is one. */
	void normalizeWeights();

	/** Clip all weights into the interval [a..b] */
	void clipWeights(Mm::Weight a=0.0, Mm::Weight b=1.0);

	/** */
	void resetWeightsSmallerThan(Mm::Weight a=0.0, Mm::Weight b=0.0);

	/** */
	void resetWeightsLargerThan(Mm::Weight a=1.0, Mm::Weight b=1.0);

	/** Multiply all weights with the specified value */
	void multiplyWeights(Mm::Weight c);

	/** Raise all weights to the given power gamma. Note that the weights are not normalized afterwards.
	 *  This function is intended to approximate a re-alignment with a different acoustic-model scale. */
	void gammaCorrection(Mc::Scale gamma);

	/** For each time frame, get a pair of begin and end iterator */
	void getFrames(std::vector<Frame> &rows) ;

	/** Component-wise weight multiplication */
	Alignment& operator*=(const Alignment&);

	friend Core::BinaryInputStream	&operator>>(Core::BinaryInputStream&,	     Alignment&);
	friend Core::BinaryOutputStream &operator<<(Core::BinaryOutputStream&, const Alignment&);
	void write(std::ostream&) const;
	void writeXml(Core::XmlWriter&) const;
	friend Core::XmlWriter &operator<<(Core::XmlWriter&, const Alignment&);
	void addTimeOffset(TimeframeIndex);
    };

} // namespace Speech

namespace Core {

    template <>
    class NameHelper<Speech::Alignment> : public std::string {
    public:
	NameHelper() : std::string("flow-alignment") {}
    };

} // namespace Core


#endif // _SPEECH_ALIGNMENT_HH
