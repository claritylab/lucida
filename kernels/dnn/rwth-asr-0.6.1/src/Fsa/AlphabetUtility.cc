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
#include <Core/Application.hh>
#include "AlphabetUtility.hh"

namespace Fsa
{

AlphabetCounts count(ConstAlphabetRef a, bool progress)
{
	//Core::ProgressIndicator *p = 0;
	//if (progress) p = new Core::ProgressIndicator("counting", "symbols");
	AlphabetCounts c;
	for (Alphabet::const_iterator i = a->begin(); i != a->end(); ++i) {
		if (i > c.maxLabelId_)
			c.maxLabelId_ = i;
		++c.nLabels_;
		if (a->isDisambiguator(LabelId(i))) ++c.nDisambiguators_;
	}
	//if (p) delete p;
	return c;
}

LabelId countDisambiguators(ConstAlphabetRef a)
{
	LabelId n = 0;
	for (Alphabet::const_iterator i = a->begin(); i != a->end(); ++i)
		if (a->isDisambiguator(LabelId(i)))
			++n;
	return n;
}

AlphabetMapping::AlphabetMapping()
{
	type_ = typeUnmapped | typeComplete | typeIdentity;
	unkId_ = InvalidLabelId;
}

void AlphabetMapping::clear()
{
	Core::Vector<LabelId>::clear();
	type_ = AlphabetMapping::typeUnmapped | AlphabetMapping::typeComplete;
}

AlphabetMapping &AlphabetMapping::operator=(const AlphabetMapping &am) {
	Core::Vector<LabelId>::operator=(am);
	from_ = am.from_;
	to_ = am.to_;
	type_ = am.type_;
	return *this;
}

void AlphabetMapping::map(LabelId from, LabelId to) {
	verify_(to != InvalidLabelId);
	grow(from, unkId_);
	Core::Vector<LabelId>::operator[](from) = to;
	if (from != to)
	type_ &= ~typeUnmapped;
	type_ &= ~typeIdentity;
}

void AlphabetMapping::del(LabelId from) {
	grow(from, unkId_);
	Core::Vector<LabelId>::operator[](from) = unkId_;
	type_ &= ~(typeComplete | typeIdentity);
}

void mapAlphabet(ConstAlphabetRef from, ConstAlphabetRef to, AlphabetMapping &mapping,
		LabelId unknownId, u32 maximumUnknownsReported)
{
	u32 nUnknowns = 0;

	mapping.clear();
	mapping.from_ = from;
	mapping.to_ = to;
	mapping.unkId_ = unknownId;

	if (!from || !to || from == to) {
		mapping.type_ |= AlphabetMapping::typeIdentity;
	} else {
		for (Alphabet::const_iterator i = from->begin(); i != from->end(); ++i) {
			LabelId id = to->index(*i);
			if (id != InvalidLabelId) {
				mapping.map(LabelId(i), id);
			} else {
				mapping.del(LabelId(i));
				if (nUnknowns < maximumUnknownsReported && maximumUnknownsReported> 0)
				Core::Application::us()->warning(
						"Fsa::mapAlphabet: \"%s\" is not in second alphabet",
						(*i).c_str());
				else if (nUnknowns == maximumUnknownsReported && maximumUnknownsReported> 0)
				Core::Application::us()->warning(
						"Fsa::mapAlphabet: further unknown symbols not reported");
				++nUnknowns;
			}
		}
		if (!mapping.isModifyingType())
		mapping.clear();
	}
	if (nUnknowns> 0 && maximumUnknownsReported> 0)
	Core::Application::us()->warning(
			"Fsa::mapAlphabet: %d symbols were mapped to \"%s\"",
			nUnknowns, ((unknownId == InvalidLabelId) ?
					"*INVALID*" : to->symbol(unknownId).c_str()));
}
} // namespace Fsa
