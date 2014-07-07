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
// $Id: WordlistInterface.cc 5439 2005-11-09 11:05:06Z bisani $

#include "WordlistInterface.hh"

#include <Core/StringUtilities.hh>
#include "InternalWordlist.hh"

using namespace Core;
using namespace Lm;


const Core::ParameterInt WordlistInterfaceLm::paramHistoryLimit(
    "history-limit",
    "maximum length of history considered (m-grammity - 1)",
    historyLengthLimit, 0, historyLengthLimit);
const Core::ParameterBool WordlistInterfaceLm::paramUseBackingOff(
    "use-backing-off",
    "use backing off to reduce length of histories",
    false);

const char *WordlistInterfaceLm::internalClassName(InternalClassIndex c) const {
    require(wl);
    return reinterpret_cast<const char*>(wl->word(c));
}

WordlistInterfaceLm::WordlistInterfaceLm(
    const Core::Configuration &c,
    Bliss::LexiconRef l):
    Core::Component(c),
    IndexMappedLm(c, l),
    wl(0),
    backingOffCounterBase_(0),
    backingOffCounter_(0),
    backingOffStatistics_(c, "backing-off-statistics", Core::Channel::disabled)
{
    historyManager_ = this;
    setMaxHistoryLength(paramHistoryLimit(config));
    useBackingOff_ = paramUseBackingOff(config);

    typedef u32* u32ptr;

    if (useBackingOff_) {
	log("using backing off");
	backingOffCounterBase_ =
	    new u32[(maxHistoryLength() + 1) * (maxHistoryLength() + 1)];
	backingOffCounter_ = new u32ptr [maxHistoryLength() + 1];
	for (size_t i = 0; i < maxHistoryLength() + 1; ++i) {
	    backingOffCounter_[i] = backingOffCounterBase_ +
		i * maxHistoryLength();
	    for (size_t j = 0; j < maxHistoryLength() + 1; ++j)
		backingOffCounter_[i][j] = 0;
	}
    } else log("not using backing off");
}

WordlistInterfaceLm::~WordlistInterfaceLm() {
    if (useBackingOff_) {
	backingOffStatistics_ << XmlOpen("backing-off-statistics") + XmlAttribute("history", maxHistoryLength() + 1);
	for (size_t i = 0; i < maxHistoryLength() + 1; ++i) {
	    backingOffStatistics_ << XmlOpen("history") + XmlAttribute("length", i + 1);
	    for (size_t j = 0; j <= i; ++j) {
		backingOffStatistics_ << " " << backingOffCounter_[i][j];
	    }
	    backingOffStatistics_ << XmlClose("history");
	}
	backingOffStatistics_ << XmlClose("backing-off-statistics");
	delete[] backingOffCounter_;
	delete[] backingOffCounterBase_;
    }
    if (wl) delete wl;
}

void WordlistInterfaceLm::setSignificantHistoryLength(u32 l) {
    log("language model file defines a %d-gram", l+1);

    if (l > historyLengthLimit)
	warning("support for %d-grams is not implemented (increase WordlistInterfaceLm::historyLengthLimit)", l+1);

    if (l < maxHistoryLength())
	setMaxHistoryLength(l);

    log("language model will be used as a %d-gram", maxHistoryLength()+1);

    ensure(maxHistoryLength() <= l);
}

HistoryHash WordlistInterfaceLm::hashKey(HistoryHandle hd) const {
    const HistoryDescriptor *cd = (const HistoryDescriptor*) hd;

    u32 s = 0;
    for (unsigned int i = 0; i < cd->length; i++) {
	s = (s << 8) + cd->history[i];
	u32 o = s & 0xff000000;
	if (o) {
	    s = s ^ (o >> 24);
	    s = s ^ o;
	}
    }
    return s;
}

bool WordlistInterfaceLm::isEquivalent(HistoryHandle hda, HistoryHandle hdb) const
{
    const HistoryDescriptor *cda = (const HistoryDescriptor*) hda;
    const HistoryDescriptor *cdb = (const HistoryDescriptor*) hdb;

    if (cda->length != cdb->length) return false;
    for (unsigned int i = 0; i < cda->length; i++)
	if (cda->history[i] != cdb->history[i]) return false;
    return true;
}

std::string WordlistInterfaceLm::format(HistoryHandle hd) const {
    const HistoryDescriptor *cd = (const HistoryDescriptor*) hd;
    std::string result;
    for (unsigned int i = 0; i < cd->length; i++) {
	result += Core::form("%d ", cd->history[i]);
    }
    return result;
}

History WordlistInterfaceLm::startHistory() const {
    require(sentenceBeginToken());
    HistoryDescriptor *result = new HistoryDescriptor(std::min(u32(1), maxHistoryLength_));
    if (result->length > 0)
	result->history[0] = internalClassIndex(sentenceBeginToken());
    if (useBackingOff_) {
	u32 length = backingOffHistoryLength(result);
	if (backingOffStatistics_.isOpen())
	    backingOffCounter_[result->length][length]++;
	result->length = length;
    }
    return history(result);
}

u32 WordlistInterfaceLm::backingOffHistoryLength(const HistoryDescriptor *h) const {
    return h->length;
}

History WordlistInterfaceLm::extendedHistory(const History &h, Token w) const {
    const HistoryDescriptor *cd = descriptor<Self>(h);

    HistoryDescriptor *result = new HistoryDescriptor(std::min(cd->length + 1, maxHistoryLength_));
    result->history[0] = internalClassIndex(w);
    for (unsigned int i = 1; i < result->length; ++i)
	result->history[i] = cd->history[i-1];
    if (useBackingOff_) {
	u32 length = backingOffHistoryLength(result);
	if (backingOffStatistics_.isOpen())
	    backingOffCounter_[result->length][length]++;
	result->length = length;
    }
    return history(result);
}

History WordlistInterfaceLm::reducedHistory(const History &h, u32 limit) const {
    const HistoryDescriptor *hd = descriptor<Self>(h);
    if (limit >= hd->length) {
	return h;
    } else {
	if (limit > maxHistoryLength_)
	    limit = maxHistoryLength_;
	HistoryDescriptor *result = new HistoryDescriptor(limit);
	for (unsigned int i = 0; i < result->length; ++i)
	    result->history[i] = hd->history[i];
	return history(result);
    }
}

std::string WordlistInterfaceLm::formatHistory(const History &h) const {
    if (!wl) return h.format();

    const HistoryDescriptor *hd = descriptor<Self>(h);
    std::string result;
    for (unsigned int i = 0; i < hd->length; i++) {
	result += Core::form("%s (%d), ",
			      internalClassName(hd->history[i]),
			      hd->history[i]);
    }
    return result;
}
