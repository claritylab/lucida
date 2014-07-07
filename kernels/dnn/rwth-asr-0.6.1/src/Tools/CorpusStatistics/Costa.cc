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
// $Id: Costa.cc 9621 2014-05-13 17:35:55Z golik $

#include <Modules.hh>
#include <Audio/Module.hh>
#include <Bliss/CorpusStatistics.hh>
#include <Core/Application.hh>
#include <Core/Parameter.hh>
#include <Lm/CorpusStatistics.hh>
#include <Flow/Module.hh>
#include <Flow/Network.hh>
#include <Signal/Module.hh>
#include <Speech/Module.hh>


class CorpusVisitor :
    public Bliss::CompositeCorpusStatisticsVisitor
{
    typedef Bliss::CompositeCorpusStatisticsVisitor Precursor;
private:
    Flow::Network *extraction_;
    Flow::PortId dataPort_;
    void correctDuration(Bliss::Segment*);
    Core::Channel recordingsChannel_;
public:
    static const Core::ParameterBool paramEvaluateRecordings;

    CorpusVisitor(const Core::Configuration&);

    virtual void enterRecording(Bliss::Recording*);
};

const Core::ParameterBool CorpusVisitor::paramEvaluateRecordings(
    "evaluate-recordings", "look at audio files using Flow network", true);

CorpusVisitor::CorpusVisitor(const Core::Configuration &c) :
    Precursor(c),
    extraction_(0),
    recordingsChannel_(config, "recordings")
{
    if (paramEvaluateRecordings(config)) {
	extraction_ = new Flow::Network(select("feature-extraction"));
	extraction_->respondToDelayedErrors();
	dataPort_ = extraction_->getOutput("out");
	if (dataPort_ == Flow::IllegalPortId) {
	    criticalError("Flow network does not have an output named \"out\"");
	}
    }
}

void CorpusVisitor::enterRecording(Bliss::Recording *r) {
    std::string inputFile(r->video());
    if (inputFile.empty()) inputFile = r->audio();

    recordingsChannel_ << inputFile << "\n";
    if (extraction_) {
	extraction_->setParameter("id", r->fullName());
	extraction_->setParameter("input-file", inputFile);

	std::istringstream durationAttr(extraction_->getAttribute(dataPort_, "total-duration"));
	Bliss::Time duration;
	if (durationAttr >> duration) {
	    r->setDuration(duration);
	}
    }
    Precursor::enterRecording(r);
}

class Costa :
    public Core::Application
{
public:
    virtual std::string getUsage() const {
	return "creating corpus statistics\n";
    }

    Costa() {
	INIT_MODULE(Flow);
	INIT_MODULE(Audio);
	setTitle("costa");
    }

    static const Core::ParameterBool paramLexcialStatistics;
    static const Core::ParameterBool paramLmStatistics;

    virtual int main(const std::vector<std::string> &arguments);
};

APPLICATION(Costa)

const Core::ParameterBool Costa::paramLexcialStatistics(
    "lexical-statistics", "create statistics about lexical properties", false);
const Core::ParameterBool Costa::paramLmStatistics(
    "lm-statistics", "create statistics about language model properties", false);

int Costa::main(const std::vector<std::string> &arguments) {
    const Core::Configuration statsConfig(config, "statistics");
    Bliss::LexiconRef lexicon;

    if (paramLexcialStatistics(config) || paramLmStatistics(config)) {
	const Core::Configuration lexConfig(statsConfig, "lexicon");
	lexicon = Bliss::Lexicon::create(lexConfig);
	if (lexicon) {
	    Core::XmlChannel ch(lexConfig, "dump");
	    if (ch.isOpen()) lexicon->writeXml(ch);
	} else
	    error("failed to initialize lexicon");
    }

    const Core::Configuration corpusConfig(statsConfig, "corpus");
    Bliss::CorpusDescription corpus(corpusConfig);

    Bliss::CompositeCorpusStatisticsVisitor *stat = 0;

    stat = new CorpusVisitor(statsConfig);
    stat->add(new Bliss::CorpusSizeStatisticsVisitor(statsConfig));
    stat->add(new Bliss::CorpusSpeakerStatisticsVisitor(statsConfig));
    stat->add(new Bliss::CorpusConditionStatisticsVisitor(statsConfig));
    if (paramLexcialStatistics(config) && lexicon) {
	stat->add(new Bliss::CorpusLexicalStatisticsVisitor(statsConfig, lexicon));
    }
    if (paramLmStatistics(config) && lexicon) {
	const Core::Configuration lmConfig(statsConfig, "lm");
	stat->add(new Lm::CorpusStatisticsVisitor(lmConfig, lexicon));
    }

    stat->reset();
    corpus.accept(stat);
    stat->writeReport(clog());

    delete stat;

    return 0;
}
