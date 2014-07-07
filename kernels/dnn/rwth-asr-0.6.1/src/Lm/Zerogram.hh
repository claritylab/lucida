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
// $Id: Zerogram.hh 5439 2005-11-09 11:05:06Z bisani $

#ifndef _LM_ZEROGRAM_HH
#define _LM_ZEROGRAM_HH

#include <Bliss/Lexicon.hh>
#include "LanguageModel.hh"

namespace Lm {

    class Zerogram :
	public LanguageModel,
	private SingletonHistoryManager
    {
    private:
	Score score_;
	virtual std::string format(HistoryHandle) const;
    public:
	Zerogram(const Core::Configuration &c, Bliss::LexiconRef);
	virtual Fsa::ConstAutomatonRef getFsa() const;
	virtual History startHistory() const { return history(0); }
	virtual History extendedHistory(const History &h, Token) const { return h; }
	virtual Score score(const History&, Token) const { return score_; }
    };

} // namespace Lm

#endif //_LM_ZEROGRAM_HH
