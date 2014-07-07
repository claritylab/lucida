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
// $Id: check.cc 9246 2013-12-06 16:36:17Z golik $

#include <fstream>
#include <Core/Application.hh>
#include <Fsa/Output.hh>
#include "CorpusDescription.hh"
#include "EditDistance.hh"
#include "Lexicon.hh"
#include "Phonology.hh"

/*
class MyCorpusVisitor:
    public Core::Component,
    public Bliss::CorpusVisitor
{
    const Bliss::LexiconRef lexicon_;
    Bliss::OrthographicParser parser_;
    Bliss::EditDistance ed_;
    Bliss::ErrorStatistic es_;
    Core::Channel dc;
public:
    MyCorpusVisitor(const Core::Configuration&,
		    const Bliss::LexiconRef);

    virtual void visitSegment(Bliss::Segment*) ;
    virtual void visitSpeechSegment(Bliss::SpeechSegment*) ;
} ;

MyCorpusVisitor::MyCorpusVisitor(
    const Core::Configuration &c,
    const Bliss::LexiconRef l) :
    Component(c),
    CorpusVisitor(),
    lexicon_(l),
    parser_(c, l),
    ed_(config),
    es_(),
    dc(config, "dot")
{
}

void MyCorpusVisitor::visitSegment(Bliss::Segment *segment) {
    clog() << "segment: " << segment->fullName() << "\n"
	   << "audio:   " << segment->recording()->audio() << "\n"
	   << "range:   " << segment->start() << " - " << segment->end() << "\n"
	   << "\n";
}

void MyCorpusVisitor::visitSpeechSegment(Bliss::SpeechSegment *segment) {
}
*/

class TestApplication : public Core::Application {
public:
    virtual std::string getUsage() const { return "short program to test Bliss features\n"; }
    TestApplication() : Core::Application() { setTitle("check"); }

    int main(const std::vector<std::string> &arguments) {
	Bliss::LexiconRef lex = Bliss::Lexicon::create(select("lexicon"));

	//Bliss::PhonemeToLemmaTransducer *t = lex->createPhonemeToLemmaTransducer(true); // ok
	//Bliss::LemmaToSyntacticTokenTransducer *t = lex->createLemmaToSyntacticTokenTransducer(); // ok
	//Bliss::LemmaToEvaluationTokenTransducer *t = lex->createLemmaToEvaluationTokenTransducer(); // ok
	//Fsa::write(Fsa::ConstAutomatonRef(t), "xxx.fsa.gz");

	{
	    Core::XmlChannel ch(config, "dump-lexicon");
	    if (ch.isOpen()) lex->writeXml(ch);
	}
	/*
	Bliss::CorpusDescription corpus(select("corpus"));

	MyCorpusVisitor visitor(select("visitor"), lex);
	corpus.accept(&visitor);
	*/
	return 0;
    }
};

APPLICATION(TestApplication)
