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
// $Id: CorpusStatistics.hh 5584 2006-02-13 21:36:40Z hoffmeister $

#ifndef _LM_CORPUSSTATISTICS_HH
#define _LM_CORPUSSTATISTICS_HH

#include "LanguageModel.hh"
#include "ClassLm.hh"
#include <Bliss/CorpusStatistics.hh>
#include <Bliss/Lexicon.hh>
#include <Bliss/Orthography.hh>
#include <Core/Parameter.hh>
#include <Core/Version.hh>

namespace Lm {

    /**
     * Corpus statistics for language model related information such
     * as perplexity.
     */

    class CorpusStatisticsVisitor :
	public Bliss::CorpusStatisticsVisitor
    {
    public:
	static const Core::ParameterBool paramIgnoreUnknowns;
	static const Core::ParameterBool paramUseClassEmissionProbabilities;

    private:
	struct SentenceStatistics;
	struct TextStatistics;

    private:
	Bliss::LexiconRef lexicon_;
	Core::Ref<LanguageModel> lm_;
	Lm::Score syntaxEmissionScale_;
	Lm::ClassLm *classLm_;
	Core::Ref<const Lm::ClassMapping> classMapping_;
	Lm::Score classEmissionScale_;

	Core::XmlChannel sentenceChannel_;
	Bliss::OrthographicParser orthographicParser_;

	bool ignoreUnk_;
	TextStatistics *lmStats_, *cmStats_;
	u32 what_;

    protected:
	SentenceStatistics buildSentenceStatistics(
	    Fsa::ConstAutomatonRef sentence,
	    Lm::Score syntaxEmissionScale, Lm::Score classEmissionScale);
	void writeReport(Core::XmlWriter &xml, TextStatistics &stats) const;

    public:
	CorpusStatisticsVisitor(const Core::Configuration&, Bliss::LexiconRef);
	~CorpusStatisticsVisitor();
	virtual void reset();
	virtual void visitSpeechSegment(Bliss::SpeechSegment*);
	virtual void writeReport(Core::XmlWriter&) const;
    };


} // namespace Lm

#endif // _LM_CORPUSSTATISTICS_HH
