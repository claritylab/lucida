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
// $Id: WordlistInterface.hh 5439 2005-11-09 11:05:06Z bisani $

#ifndef _LM_WORDLIST_INTERFACE_HH
#define _LM_WORDLIST_INTERFACE_HH

#include <Core/Channel.hh>
#include <Core/Parameter.hh>
#include <Core/Types.hh>
#include "LanguageModel.hh"
#include "IndexMap.hh"

namespace Lm {

    /**
     * Abstract bsse class for Philips- and (legacy) RWTH language
     * model adaptors.
     */

    class WordlistInterfaceLm:
	public IndexMappedLm,
	public ReferenceCountingHistoryManager
    {
	typedef WordlistInterfaceLm Self;
    public:
	/** Built-in limit: maximum number of word in the history
	 * (excluding the current word) */
	static const u32 historyLengthLimit = 2;

	static const Core::ParameterInt paramHistoryLimit;
	static const Core::ParameterBool paramUseBackingOff;
    protected:
	class InternalWordlist;
	InternalWordlist *wl;
	virtual const char *internalClassName(InternalClassIndex) const;
    private:
	u32 maxHistoryLength_;
	void setMaxHistoryLength(u32 l) {
	    require(l <= historyLengthLimit);
	    maxHistoryLength_ = l;
	}
	bool useBackingOff_;
	u32 *backingOffCounterBase_;
	u32 **backingOffCounter_;
	Core::XmlChannel backingOffStatistics_;
    protected:
	/** Maximum number of word considered in the history
	 * (excluding the current word) */
	u32 maxHistoryLength() const { return maxHistoryLength_; }

	/** Set the number of words which are significant to loaded
	 * language model (excluding the current word).   Should be
	 * called during initialization to indicate actual m-grammity
	 * of the loaded language model file. */
	void setSignificantHistoryLength(u32 l);

    public:
	class HistoryDescriptor :
	    public Core::ReferenceCounted
	{
	public:
	    InternalClassIndex history[historyLengthLimit];
	    u32 length;

	    HistoryDescriptor(u32 _length) {
		require(_length <= historyLengthLimit);
		length = _length;
	    }
	};

    protected: // HistoryManager interface
	virtual HistoryHash hashKey(HistoryHandle) const;
	virtual bool isEquivalent(HistoryHandle, HistoryHandle) const;
	virtual std::string format(HistoryHandle) const;

    protected:
	History history(const HistoryDescriptor *hd) const {
	    hd->acquireReference();
	    return LanguageModel::history(hd);
	}
	virtual u32 backingOffHistoryLength(const WordlistInterfaceLm::HistoryDescriptor*) const;

    public:
	WordlistInterfaceLm(const Core::Configuration&, Bliss::LexiconRef);
	virtual ~WordlistInterfaceLm();

	virtual History startHistory() const;
	virtual History extendedHistory(const History&, Token w) const;

	/**
	 * Reduce amount on conditioning information.
	 * @param limit number of predecessor word taken into account.
	 * (The effective m-grammity of the returned history is @c limit + 1.)
	 */
	virtual History reducedHistory(const History&, u32 limit) const;

	virtual std::string formatHistory(const History&) const;
    };

} // namespace Lm

#endif // _LM_WORDLIST_INTERFACE_HH
