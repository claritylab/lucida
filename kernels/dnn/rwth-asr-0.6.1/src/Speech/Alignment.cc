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
#include "Alignment.hh"
#include <Core/Application.hh>

// #include <Fsa/Arithmetic.hh>
// #include <Fsa/Best.hh>
// #include <Fsa/Basic.hh>
// #include <Fsa/Compose.hh>
// #include <Fsa/Output.hh>
// #include <Fsa/Rational.hh>
// #include <Fsa/RemoveEpsilons.hh>
// #include <Lattice/Utilities.hh>
// #include <Lattice/Basic.hh>
// #include <Am/Application.hh>
// #include <Am/ClassicAcousticModel.hh>
// #include <Core/CompressedStream.hh>
// #include <Core/Archive.hh>


using namespace Speech;

/**
 * Alignment of time frame indices between acoustic features and HMM states
 * features:	start		   t-1	    t	   t+1		end-1
 *   INITIAL O---------O-->   --O-------O-------O----->	   --O---------O FINAL
 * states: start    start+1    t-1	t      t+1	   end-1      end
 */


//=================================================================================


/* Helper functions to realize a UTF-8-like encoding scheme for u32 values.
 * The best compression rate is achieved if there are many small values
 * and only some huge values.
 */
void writePackedU32(Core::BinaryOutputStream &out, u32 value) {
    do {
	if (value < 128) {
	    out << (u8)value;
	} else {
	    out << (u8)((value & 127) | 128);
	}
	value >>= 7;
    } while(value);
}

u32 readPackedU32(Core::BinaryInputStream &in) {
    u32 result = 0; u8 b; u8 s = 0;
    do {
	in >> b;
	if (b & 128) {
	    result |= ((b & 127) << s);
	} else {
	    result |= (b << s);
	}
	s += 7;
    } while (b & 128);
    return result;
}


//=================================================================================


const char *Alignment::magic = "ALIGNRLE";
const char *Alignment::magic_alphabet = "AALPHRLE";
const size_t Alignment::magicSize = 8;

/*
 * Alignments use a simple run-length compression scheme.
 * (Run-length encoding is a sensible choice because of the HMM
 * loop-transitions, esp. in silence portions.)
 * A one byte code n indicated one of the following cases:
 * n > 0: the next n shorts are mixtures indices
 * n < 0: the next short should be repeated |n| times
 * In case time indices are not contiguous, the 0 code is used:
 * n = 0: the next long is the new time frame index
 *
 * Weighted alignments use a different compression scheme:
 * u32 values like sizes and allophone state indices are compressed
 * by an utf-8-like encoding. Weights are expected to be inside
 * the interval [0..1] and quantized by a u16 value.
 */

Core::BinaryOutputStream &Speech::operator<<(Core::BinaryOutputStream &os, const Alignment &a)
{
    os.write(a.alphabet_ ? Alignment::magic_alphabet : Alignment::magic, Alignment::magicSize);

    std::map<Fsa::LabelId, std::string> alphabet;

    TimeframeIndex time = 0;
    if (!a.hasWeights()) {
	// viterbi compression scheme with run-length encoding
	os << u32(a.size());
	for (s32 i = 0; os && i < s32(a.size());) {
	    if (time != a[i].time) {
		os << s8(0) << u32(time = a[i].time);
	    }
	    s32 n;

	    if(a.alphabet_ && !alphabet.count(a[i].emission))
	    {
		std::string symbol = a.alphabet_->symbol(a[i].emission);
		verify(symbol.length());
		alphabet.insert(std::make_pair(a[i].emission, symbol));
	    }

	    if (i+1 < s32(a.size()) && a[i].emission == a[i+1].emission) {
		for (n = 1; i+n < s32(a.size()); ++n) {
		    if (a[i+n].emission != a[i].emission) break;
		    if (a[i+n].time != a[i].time + n) break;
		    if (-n <= Core::Type<s8>::min) break;
		}
		hope(a[i].emission <= Fsa::LastLabelId);
		os << s8(-n) << u32(a[i].emission);
		i += n;
	    } else {
		for (n = 1; i+n < s32(a.size()); ++n) {
		    if (a[i+n].emission == a[i+n-1].emission) break;
		    if (a[i+n].time != a[i].time + n) break;
		    if (n >= Core::Type<s8>::max) break;
		}
		os << s8(n);
		while (n--) {
		    if(a.alphabet_ && !alphabet.count(a[i].emission))
		    {
			std::string symbol = a.alphabet_->symbol(a[i].emission);
			verify(symbol.length());
			alphabet.insert(std::make_pair(a[i].emission, symbol));
		    }
		    hope(a[i].emission <= Fsa::LastLabelId);
		    os << u32(a[i++].emission);
		}
	    }
	    time += n;
	}
    } else {
	// compression scheme for weighted alignments
#if 1
	const u32 version = 2;
#endif
	const u32 highestBitU32 = 1 << 31;
	os << u32(version | highestBitU32); // use highest bit to distinguish this scheme from the RLE scheme
	writePackedU32(os, a.size());
	Alignment::const_iterator i, i_end, j;
	i = j = a.begin();
	i_end = a.end();
	TimeframeIndex lastTimePlusOne = 0;
	while (i != i_end) {
	    TimeframeIndex time = j->time;
	    while (i != i_end && i->time == time) ++i;
	    u32 nItemsCurrentFrame = u32(i - j);
	    if (time == lastTimePlusOne) {
		writePackedU32(os, nItemsCurrentFrame*2);
	    } else {
		writePackedU32(os, nItemsCurrentFrame*2+1);
		writePackedU32(os, time);
	    }
	    for (; j != i; ++j) {
		if(a.alphabet_ && !alphabet.count(j->emission))
		{
		    std::string symbol = a.alphabet_->symbol(j->emission);
		    verify(symbol.length());
		    alphabet.insert(std::make_pair(j->emission, symbol));
		}

		writePackedU32(os, (u32)(j->emission));
#if 1
		f32 tmp = j->weight;
		os << tmp;
#endif
	    }
	    lastTimePlusOne = time+1;
	}
    }
    if(a.alphabet_)
    {
	writePackedU32(os, alphabet.size());
	for(std::map<Fsa::LabelId, std::string>::const_iterator it = alphabet.begin(); it != alphabet.end(); ++it)
	{
	    writePackedU32(os, it->first);
	    os << it->second;
	}
    }
    return os;
}

Core::BinaryInputStream &Speech::operator>>(Core::BinaryInputStream &is, Alignment &a)
{
    a.clear();

    char header[Alignment::magicSize];
    is.read(header, Alignment::magicSize);
    bool matchedHeader = strncmp(header, Alignment::magic, Alignment::magicSize) == 0;
    bool matchedAlphabetHeader = strncmp(header, Alignment::magic_alphabet, Alignment::magicSize) == 0;
    if (!matchedHeader && !matchedAlphabetHeader) {
	is.addState(std::ios::badbit);
	return is;
    }

    u32 size = 0;
    is >> size;
    if (size < ((u32)1 << 31)) {
	// RLE scheme
	u32 time = 0;
	u32 mixture;
	s8 n;
	for (; is && a.size() < size;) {
	    is >> n;
	    if (n > 0) {
		// read N different mixtures
		while (n--) {
		    is >> mixture;
		    a.push_back(AlignmentItem(time++, mixture));
		}
	    } else if (n < 0) {
		// repeat mixture N times
		is >> mixture;
		while (n++) {
		    a.push_back(AlignmentItem(time++, mixture));
		}
	    } else /* if (n == 0) */ {
		// change time index
		is >> time;
	    }
	}
    } else {
	// compression scheme for weighted alignments
	u32 version = size; version &= ((1 << 31) - 1);
	require(version <= 2);
	a.resize(readPackedU32(is));
	TimeframeIndex time = 0;
	Alignment::iterator i, i_end;
	i = a.begin(); i_end = a.end();
	while (i != i_end) {
	    u32 nItemsCurrentFrame = readPackedU32(is);
	    if (nItemsCurrentFrame & 1) {
		time = readPackedU32(is);
	    }
	    nItemsCurrentFrame /= 2;
	    for (; nItemsCurrentFrame; --nItemsCurrentFrame, ++i) {
		require(i != i_end);
		i->time = time;
		i->emission = readPackedU32(is);
		if (version == 1) {
		    u16 quantizedWeight; is >> quantizedWeight;
		    i->weight = (Mm::Weight)(quantizedWeight) / 65535;
		} else if (version == 2) {
		    f32 tmp; is >> tmp;
		    i->weight = tmp;
		}
	    }
	    ++time;
	}
    }

    if(matchedAlphabetHeader)
    {
	u32 count = readPackedU32(is);
	for(u32 i = 0; i < count; ++i)
	{
	    u32 emission = readPackedU32(is);
	    std::string target;
	    is >> target;
	    a.archiveAlphabet_.insert(std::make_pair(emission, target));
	}
	for(Alignment::iterator it = a.begin(); it != a.end(); ++it)
	    verify(a.archiveAlphabet_.count(it->emission));
	if(a.alphabet_)
	    a.mapAlphabet(false);
    }

    return is;
}

void Alignment::mapAlphabet(bool skipMismatch)
{
    verify(alphabet_);
    const Am::AllophoneStateAlphabet* alphabet = dynamic_cast<const Am::AllophoneStateAlphabet*>(alphabet_.get());
    verify(alphabet);
    std::set<std::string> missing;
    u32 missingCount = 0;
    // replaced for-loop using STL iterator by openMP parallelized loop
    // looking up the indices in the string-hash map is quite expensive for sequence training
    // TODO there must be a better way to do this more efficiently
    // for(Alignment::iterator it = begin(); it != end(); ++it)
    #pragma omp parallel for
    for(size_t t = 0; t < this->size(); ++t) {
	Alignment::iterator it = begin() + t;
	std::map<u32, std::string>::const_iterator mapIt = archiveAlphabet_.find(it->emission);
	verify(mapIt != archiveAlphabet_.end());
	verify(!mapIt->second.empty());
	it->emission = alphabet->index(mapIt->second, false);
	if(it->emission == Fsa::InvalidLabelId)
	{
	    missing.insert(mapIt->second);
	    ++missingCount;
	    continue;
	}
	verify(it->emission != Fsa::InvalidLabelId);
	// TODO assertion is not multithreading safe because of reference counting
//	verify(alphabet_->symbol(it->emission) == mapIt->second);
    }
    archiveAlphabet_.clear();
    if(missingCount)
    {
	{
	    Core::Component::Message msg = Core::Application::us()->warning();
	    msg << "failed mapping " << missingCount << " out of " << size() << " alignment-items\n";
	    msg << "mismatched allophones:";
	    for(std::set<std::string>::const_iterator it = missing.begin(); it != missing.end(); ++it)
		msg << " " << *it;
	}
	if(!skipMismatch)
	{
	    verify(0 /* mapping of some allophones failed, and skipping is disabled */);
	}
	Alignment::iterator itOut = begin();
	for(Alignment::iterator it = begin(); it != end(); ++it)
	{
	    if(it->emission != Fsa::InvalidLabelId)
	    {
		*itOut = *it;
		++itOut;
	    }
	}
	erase(itOut, end());
    }
}

bool Alignment::setAlphabet(Core::Ref<const Fsa::Alphabet> alphabet, bool skipMismatch)
{
    if(alphabet_ == alphabet)
	return false;

    if(alphabet_)
    {
	// Backup the relevant part of the old alphabet, to perform the mapping aftwards
	verify(archiveAlphabet_.empty());
	for(Alignment::iterator it = begin(); it != end(); ++it)
	{
	    std::string symbol = alphabet_->symbol(it->emission);
	    verify(!symbol.empty());
	    archiveAlphabet_.insert(std::make_pair(it->emission, symbol));
	}
    }

    alphabet_ = alphabet;
    if(archiveAlphabet_.size())
    {
	mapAlphabet(skipMismatch);
	return true;
    }else{
	return false;
    }
}

Alignment::Alignment() : score_(Core::Type<Score>::max) {}


void Alignment::write(std::ostream &os) const
{
    for (const_iterator i = begin(); os && i != end(); ++i) {
	os << "time = "	    << i->time << '\t'
	   << "emission = ";
	if(alphabet_)
	{
	    os << alphabet_->symbol(i->emission);
	}else{
	    os << i->emission;

	} os << ((i->weight != 1.0) ? Core::form("\tweight = %f", i->weight) : "")
	   << std::endl;
    }
}

void Alignment::writeXml(Core::XmlWriter &os) const
{
    os << Core::XmlOpen("alignment");
    for (const_iterator i = begin(); os && i != end(); ++i) {
	os << Core::XmlOpen("align")
	   << Core::XmlFull("time", i->time);

	if(alphabet_)
	{
	    os << Core::XmlFull("emission", alphabet_->symbol(i->emission));
	}else{
	    os << Core::XmlFull("emission", i->emission);
	}

	if (i->weight != 1.0)
	    os << Core::XmlFull("weight", i->weight);
	os << Core::XmlClose("align");
    }
    os << Core::XmlClose("alignment");
}

Core::XmlWriter &Speech::operator<<(Core::XmlWriter &o, const Alignment &a)
{
    a.writeXml(o); return o;
}


struct SortByTimeAndDecreasingWeight {
    bool operator()(const AlignmentItem &a, const AlignmentItem &b) {
	if (a.time < b.time or (a.time == b.time and a.weight > b.weight))
	    return true;
	return false;
    }
};

struct SortByTimeAndIncreasingWeight {
    bool operator()(const AlignmentItem &a, const AlignmentItem &b) {
	if (a.time < b.time or (a.time == b.time and a.weight < b.weight))
	    return true;
	return false;
    }
};

struct SortByTimeAndEmission {
    bool operator()(const AlignmentItem &a, const AlignmentItem &b) {
	if (a.time < b.time or (a.time == b.time and a.emission < b.emission))
	    return true;
	return false;
    }
};

void Alignment::sortItems(bool byDecreasingWeight)
{
    if (byDecreasingWeight) {
	std::sort(begin(), end(), SortByTimeAndDecreasingWeight());
    } else {
	std::sort(begin(), end(), SortByTimeAndIncreasingWeight());
    }
}

void Alignment::sortStableItems()
{
    std::stable_sort(begin(), end(), SortByTimeAndEmission());
}

void Alignment::combineItems(Fsa::ConstSemiringRef sr)
{
    if (empty()) return;
    SortByTimeAndEmission comp;
    std::sort(begin(), end(), comp);
    Alignment::iterator in, last, in_end, out;
    out = last = begin();
    in = last + 1;
    in_end = end();
    for (; in != in_end; ++in, ++last) {
	if (in->emission == last->emission and in->time == last->time) {
	    out->weight = f32(sr->collect(Fsa::Weight(out->weight), Fsa::Weight(in->weight)));
	} else {
	    *(++out) = *in;
	}
    }
    erase(out+1, in_end);
}

void Alignment::expm()
{
    for (Alignment::iterator in = begin(); in != end(); ++in) {
	in->weight = std::isinf(in->weight) ? 0 : exp(-in->weight);
    }
}

void Alignment::addWeight(Mm::Weight weight)
{
    for (Alignment::iterator in = begin(); in != end(); ++in) {
	in->weight += weight;
    }
}

void Alignment::filterWeights(Mm::Weight minWeight, Mm::Weight maxWeight) {
    Alignment::iterator in, in_end, out;
    in = out = begin();
    in_end = end();
    for (; in != in_end; ++in) {
	if (in->weight >= minWeight and in->weight <= maxWeight)
	    *(out++) = *in;
    }
    erase(out, in_end);
}

void Alignment::gammaCorrection(Mc::Scale gamma) {
    Alignment::iterator in, in_end;
    in = begin();
    in_end = end();
    for (; in != in_end; ++in) {
	if (in->weight != 0.0)
	    in->weight = std::exp(gamma * std::log(in->weight));
    }
}

void Alignment::multiplyWeights(Mm::Weight c) {
    Alignment::iterator in, in_end;
    in = begin();
    in_end = end();
    for (; in != in_end; ++in) {
	in->weight *= c;
    }
}

void Alignment::normalizeWeights() {
    if (empty()) return;
    Alignment::iterator in, in_end, out;
    in = out = begin();
    in_end = end();
    Mm::Weight weightCurrentFrame;
    while (in != in_end) {
	weightCurrentFrame = 0.0;
	do {
	    weightCurrentFrame += in->weight;
	    ++in;
	} while (in != in_end and in->time == (in-1)->time);
	Mm::Weight inverseWeightCurrentFrame = 1 / weightCurrentFrame;
	for (; out != in; ++out) {
	    out->weight *= inverseWeightCurrentFrame;
	}
    }
}

void Alignment::clipWeights(Mm::Weight a, Mm::Weight b) {
    Alignment::iterator in, in_end;
    in = begin(); in_end = end();
    for (; in != in_end; ++in) {
	if (in->weight < a)
	    in->weight = a;
	else if (in->weight > b)
	    in->weight = b;
    }
}

void Alignment::resetWeightsSmallerThan(Mm::Weight a, Mm::Weight b) {
	Alignment::iterator in, in_end;
	in = begin(); in_end = end();
	for (; in != in_end; ++in) {
		if (in->weight < a)
			in->weight = b;
	}
}

void Alignment::resetWeightsLargerThan(Mm::Weight a, Mm::Weight b) {
	Alignment::iterator in, in_end;
	in = begin(); in_end = end();
	for (; in != in_end; ++in) {
		if (in->weight > a)
			in->weight = b;
	}
}

bool Alignment::hasWeights() const {
    const_iterator in, in_end;
    in = begin(); in_end = end();
    for (; in != in_end; ++in)
	if (in->weight != 1.0) return true;
    return false;
}

void Alignment::getFrames(std::vector<std::pair<Alignment::iterator, Alignment::iterator> > &rows)  {
    rows.clear();
    Alignment::iterator i = begin();
    Alignment::iterator i_end = end();
    Alignment::iterator i_firstCurrentFrame = begin();
    while (i != i_end) {
	while (i != i_end && i->time == i_firstCurrentFrame->time) ++i;
	rows.push_back(std::pair<Alignment::iterator, Alignment::iterator>(i_firstCurrentFrame, i));
	i_firstCurrentFrame = i;
    }
}

Alignment& Alignment::operator*=(const Alignment &a2) {
    require(size() == a2.size());
    Alignment::iterator in = begin();
    Alignment::iterator in_end = end();
    Alignment::const_iterator in2 = a2.begin();
    for (; in != in_end; ++in, ++in2) {
	in->weight *= in2->weight;
    }
    return *this;
}

void Alignment::addTimeOffset(TimeframeIndex offset) {
    for (iterator i = begin(); i != end(); ++i) {
	i->time += offset;
    }
}


// ================================================================================
