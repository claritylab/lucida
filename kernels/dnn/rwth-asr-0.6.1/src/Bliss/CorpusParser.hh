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
#ifndef _BLISS_CORPUS_PARSER_HH
#define _BLISS_CORPUS_PARSER_HH

#include <Core/XmlBuilder.hh>
#include <Core/ProgressIndicator.hh>
#include "CorpusDescription.hh"

namespace Bliss {

class SpeakerDescriptionElement;
class ConditionDescriptionElement;
class OrthographyElement;

/**
 * Parser for Bliss corpus description files.
 * This class implements parsing of the corpus description XML format
 * described in <a href="../../doc/Corpus.pdf">Corpus Description
 * Format Reference</a>.  It is normally not used directly but through
 * CorpusDescription.
 */

class CorpusDescriptionParser :
    public Core::XmlSchemaParser
{
private:
    typedef CorpusDescriptionParser Self;

    CorpusVisitor *corpusVisitor_ ;

    Core::ProgressIndicator *progressIndicator_;

    std::string   corpusDir_;
    std::string   audioDir_;
    std::string   videoDir_;
    bool          shallCaptializeTranscriptions_;
    bool          shallGemenizeTranscriptions_;
    bool          isSubParser_ ;
    Corpus        *superCorpus_, *corpus_ ;
    Recording     *recording_ ;
    SpeechSegment *segment_ ;
    int           segmentNum_;      // counter for enumeration of unnamed segments
    CorpusSection *currentSection_; // current innermost corpus, subcorpus or recording

    const Speaker *getSpeaker(const std::string &name) const;
    const AcousticCondition *getCondition(const std::string &name) const;

    void startCorpus(const Core::XmlAttributes atts) ;
    void endCorpus() ;
    void startSubcorpus(const Core::XmlAttributes atts) ;
    void endSubcorpus() ;
    void include(const Core::XmlAttributes atts) ;
    void startRecording(const Core::XmlAttributes atts) ;
    void endRecording() ;
    void startSegment(const Core::XmlAttributes atts) ;
    void endSegment() ;

    void defineSpeaker(std::auto_ptr<Speaker>&);
    void selectDefaultSpeaker(const Core::XmlAttributes atts);
    void defineSegmentSpeaker(std::auto_ptr<Speaker>&);
    void selectSegmentSpeaker(const Core::XmlAttributes atts);

    void defineCondition(std::auto_ptr<AcousticCondition>&);
    void selectDefaultCondition(const Core::XmlAttributes atts);
    void defineSegmentCondition(std::auto_ptr<AcousticCondition>&);
    void selectSegmentCondition(const Core::XmlAttributes atts);

    void setOrth(const std::string&);

    /**
     * Include another corpus file at current (logical) position.
     * @param relativeFilename path of the corpus file to be included.
     * @c filename is resolved against the configuration and treated
     * as a path relative to the current file. */
    void includeFile(const std::string &relativeFilename) ;

    void initSchema() ;
    CorpusDescriptionParser(const Core::Configuration&, Corpus*) ;

public:
    static const Core::ParameterString paramAudioDir;
    static const Core::ParameterString paramVideoDir;
    static const Core::ParameterBool paramCaptializeTranscriptions;
    static const Core::ParameterBool paramGemenizeTranscriptions;
    static const Core::ParameterBool paramProgress;

    CorpusDescriptionParser(const Core::Configuration&) ;
    virtual ~CorpusDescriptionParser();

    int accept(const std::string &filename, CorpusVisitor*);
};

} // namespace Bliss

#endif // _BLISS_CORPUS_PARSER_HH
