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
#ifndef _BLISS_CORPUS_STATISTICS_HH
#define _BLISS_CORPUS_STATISTICS_HH

#include "CorpusDescription.hh"
#include <Core/Hash.hh>
#include <Core/ReferenceCounting.hh>
#include <list>
namespace Bliss {
    class Lexicon;
    class OrthographicParser;
}

namespace Bliss {

    class CorpusStatisticsVisitor :
	public Core::Component,
	public CorpusVisitor
    {
    public:
	CorpusStatisticsVisitor(const Core::Configuration &c) : Component(c) {};
	virtual void reset() = 0;
	virtual void writeReport(Core::XmlWriter&) const = 0;
    };

    class CorpusSizeStatisticsVisitor :
	public CorpusStatisticsVisitor
    {
    private:
	unsigned int nRecordings_, nSpeechSegments_, nOtherSegments_;
	Time totalNetDuration_, totalGrossDuration_;
	unsigned int nNetTimeFrames_;

    public:
	CorpusSizeStatisticsVisitor(const Core::Configuration &c) : CorpusStatisticsVisitor(c) {};
	virtual void enterRecording(Recording*);
	virtual void visitSegment(Segment*);
	virtual void visitSpeechSegment(SpeechSegment*);

	virtual void reset();
	virtual void writeReport(Core::XmlWriter&) const;
    };

    class CorpusSpeakerStatisticsVisitor :
	public CorpusStatisticsVisitor
    {
    private:
	struct SpeakerStatistics {
	    unsigned int nSegments;
	    Time totalDuration;
	    SpeakerStatistics() : nSegments(0), totalDuration(0.0) {}
	};
	void writeSpeakerStatistics(const SpeakerStatistics&, Core::XmlWriter&) const;
	typedef Core::StringHashMap<SpeakerStatistics> SpeakerStatisticsMap;
	SpeakerStatisticsMap speakerStatistics_;
	SpeakerStatistics noSpeaker_;
	SpeakerStatistics genderStatistics_[Speaker::nGenders];
	typedef Core::StringHashSet SpeakerSet;
	SpeakerSet genderSpeakers_[Speaker::nGenders];
	void accu(SpeakerStatistics&, const SpeechSegment*);
    public:
	CorpusSpeakerStatisticsVisitor(const Core::Configuration &c) : CorpusStatisticsVisitor(c) {};
	virtual void visitSpeechSegment(SpeechSegment*);
	virtual void reset();
	virtual void writeReport(Core::XmlWriter&) const;
    };

    class CorpusConditionStatisticsVisitor :
	public CorpusStatisticsVisitor
    {
    private:
	struct ConditionStatistics {
	    unsigned int nSegments;
	    Time totalDuration;
	    ConditionStatistics() : nSegments(0), totalDuration(0.0) {}
	};
	void writeConditionStatistics(const ConditionStatistics&, Core::XmlWriter&) const;
	typedef Core::StringHashMap<ConditionStatistics> ConditionStatisticsMap;
	ConditionStatisticsMap conditionStatistics_;
	ConditionStatistics noCondition_;
	typedef Core::StringHashSet ConditionSet;
	void accu(ConditionStatistics&, const Segment*);
    public:
	CorpusConditionStatisticsVisitor(const Core::Configuration &c) : CorpusStatisticsVisitor(c) {};
	virtual void visitSegment(Segment*);
	virtual void reset();
	virtual void writeReport(Core::XmlWriter&) const;
    };

    class CorpusLexicalStatisticsVisitor :
	public CorpusStatisticsVisitor
    {
    private:
	Core::Ref<const Lexicon> lexicon_;
	OrthographicParser *orthographicParser_;
	class Internal;
	Internal *internal_;

    public:
	CorpusLexicalStatisticsVisitor(const Core::Configuration&, Core::Ref<const Lexicon>);
	virtual ~CorpusLexicalStatisticsVisitor();
	virtual void visitSpeechSegment(SpeechSegment*);
	virtual void reset();
	virtual void writeReport(Core::XmlWriter&) const;
    };

    class CompositeCorpusStatisticsVisitor :
	public CorpusStatisticsVisitor
    {
    private:
	typedef std::list<CorpusStatisticsVisitor*> ComponentList;
	ComponentList components_;
    public:
	CompositeCorpusStatisticsVisitor(const Core::Configuration&);
	void add(CorpusStatisticsVisitor*);

	virtual void enterRecording(Recording*);
	virtual void leaveRecording(Recording*);
	virtual void visitSegment(Segment*);
	virtual void visitSpeechSegment(SpeechSegment*);

	virtual void reset();
	virtual void writeReport(Core::XmlWriter&) const;
    };

} // namespace Bliss

#endif // _BLISS_CORPUS_STATISTICS_HH
