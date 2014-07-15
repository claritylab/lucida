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
#include "CorpusParser.hh"

#include <Core/Directory.hh>
#include <Core/StringUtilities.hh>

using namespace Bliss;
using namespace Core;


// Note: Features marked "FFE" are placeholders "for furture extensions".

// ===========================================================================
class Bliss::SpeakerDescriptionElement :
    public  Core::XmlBuilderElement<Speaker, Core::XmlMixedElement, Core::CreateUsingNew>
{
    typedef Core::XmlBuilderElement<Speaker, Core::XmlMixedElement, Core::CreateUsingNew> Precursor;
    typedef SpeakerDescriptionElement Self;
protected:
    virtual void start(const Core::XmlAttributes atts);
    virtual void characters(const char *ch, int len) {};
    void gender(const std::string &s);
public:
    SpeakerDescriptionElement(Core::XmlContext *_context, Handler _handler = 0);
};

SpeakerDescriptionElement::SpeakerDescriptionElement(
    Core::XmlContext *_context, Handler _handler) :
    Precursor("speaker-description", _context, _handler)
{
    addChild(collect(new XmlStringBuilderElement(
			 "gender",this,
			 XmlStringBuilderElement::handler(&Self::gender))));
    addChild(collect(new XmlIgnoreElement("name",            this))); // FFE
    addChild(collect(new XmlIgnoreElement("age",             this))); // FFE
    addChild(collect(new XmlIgnoreElement("date-of-birth",   this))); // FFE
    addChild(collect(new XmlIgnoreElement("native-language", this))); // FFE
    ignoreUnknownElements();
}

void SpeakerDescriptionElement::start(const XmlAttributes atts) {
    Precursor::start(atts);
    Speaker *s = product_.get();
    const char *name = atts["name"] ;
    if (name) {
	s->setName(name);
    }
}

void SpeakerDescriptionElement::gender(const std::string &_s) {
    std::string s(_s);
    stripWhitespace(s);
    if (s == Speaker::genderId[Speaker::male])
	product_->gender_ = Speaker::male;
    else if (s == Speaker::genderId[Speaker::female])
	product_->gender_ = Speaker::female;
    else if (s == Speaker::genderId[Speaker::unknown])
	product_->gender_ = Speaker::unknown;
    else
	parser()->warning("Unrecognized gender specification \"%s\"", s.c_str());
}

// ===========================================================================
class Bliss::ConditionDescriptionElement :
    public  Core::XmlBuilderElement<AcousticCondition, Core::XmlMixedElement, Core::CreateUsingNew>
{
    typedef Core::XmlBuilderElement<AcousticCondition, Core::XmlMixedElement, Core::CreateUsingNew> Precursor;
    typedef ConditionDescriptionElement Self;
protected:
    virtual void start(const Core::XmlAttributes atts);
    virtual void characters(const char *ch, int len) {};
public:
    ConditionDescriptionElement(Core::XmlContext *_context, Handler _handler = 0);
};

ConditionDescriptionElement::ConditionDescriptionElement(
    Core::XmlContext *_context, Handler _handler) :
    Precursor( "condition-description", _context, _handler)
{
    addChild(collect(new XmlIgnoreElement("environment", this))); // FFE
    addChild(collect(new XmlIgnoreElement("microphone",  this))); // FFE
    addChild(collect(new XmlIgnoreElement("channel",     this))); // FFE
    ignoreUnknownElements();
}

void ConditionDescriptionElement::start(const XmlAttributes atts) {
    Precursor::start(atts);
    AcousticCondition *c = product_.get();
    const char *name = atts["name"] ;
    if (name) {
	c->setName(name);
    }
}

// ===========================================================================
class Bliss::OrthographyElement :
    public Core::XmlBuilderElement<std::string, Core::XmlMixedElement, Core::CreateStatic>
{
    typedef Core::XmlBuilderElement<std::string,Core::XmlMixedElement, Core::CreateStatic> Precursor;
    typedef OrthographyElement Self;
public:
    OrthographyElement(Core::XmlContext *_context, Handler _handler = 0);
    virtual void characters(const char *ch, int len);
};

OrthographyElement::OrthographyElement(
    Core::XmlContext *_context, Handler _handler /*= 0*/) :
    Precursor("orth", _context, _handler)
{
    addChild(collect(new XmlFlattenElement("noise",      this))); // FFE
    addChild(collect(new XmlFlattenElement("hesitation", this))); // FFE
    addChild(collect(new XmlFlattenElement("name",       this))); // FFE
    addChild(collect(new XmlFlattenElement("numeral",    this))); // FFE
    addChild(collect(new XmlFlattenElement("pron",       this))); // FFE
    flattenUnknownElements();
}

void OrthographyElement::characters(const char *ch, int len) {
    std::string s(ch, len);
    if (s.find_first_of(utf8::whitespace) == 0)
	enforceTrailingBlank(product_);
    normalizeWhitespace(s);
    product_ += s;
    verify(isWhitespaceNormalized(product_, tolerateTrailingBlank));
}

// ===========================================================================
const Core::ParameterString CorpusDescriptionParser::paramAudioDir(
    "audio-dir",
    "directory with audio files",
    "",
    "This path will be prefixed to all audio file names, unless they are absolute.  If unset, the directory of the corpus file will be used.");

const Core::ParameterString CorpusDescriptionParser::paramVideoDir(
    "video-dir",
    "directory with video files",
    "",
    "This path will be prefixed to all video file names, unless they are absolute.  If unset, the directory of the corpus file will be used.");

const Core::ParameterBool CorpusDescriptionParser::paramCaptializeTranscriptions(
    "capitalize-transcriptions",
    "convert all transcriptions to upper case: yes/no",
    false);

const Core::ParameterBool CorpusDescriptionParser::paramGemenizeTranscriptions(
    "gemenize-transcriptions",
    "convert all transcriptions to lower case: yes/no",
    false);

const Core::ParameterBool CorpusDescriptionParser::paramProgress(
	"progress",
	"show progress meter",
	false);

void CorpusDescriptionParser::initSchema() {
    XmlElement *conditionDesc =
	collect(new ConditionDescriptionElement(
		    this, ConditionDescriptionElement::handler(&Self::defineCondition)));
    XmlElement *segmentConditionDesc =
	collect(new ConditionDescriptionElement(
		    this, ConditionDescriptionElement::handler(&Self::defineSegmentCondition)));

    XmlElement *condition =
	collect(new Core::XmlEmptyElementRelay(
		    "condition", this, startHandler(&Self::selectDefaultCondition)));
    XmlElement *segmentCondition =
	collect(new Core::XmlEmptyElementRelay(
		    "condition", this, startHandler(&Self::selectSegmentCondition)));

    XmlElement *speakerDesc =
	collect(new SpeakerDescriptionElement(
		    this, SpeakerDescriptionElement::handler(&Self::defineSpeaker)));
    XmlElement *segmentSpeakerDesc =
	collect(new SpeakerDescriptionElement(
		    this, SpeakerDescriptionElement::handler(&Self::defineSegmentSpeaker)));

    XmlElement *speaker =
	collect(new Core::XmlEmptyElementRelay(
		    "speaker", this, startHandler(&Self::selectDefaultSpeaker)));
    XmlElement *segmentSpeaker =
	collect(new Core::XmlEmptyElementRelay(
		    "speaker", this, startHandler(&Self::selectSegmentSpeaker)));

    XmlElement *orth =
	collect(new OrthographyElement(
		    this, OrthographyElement::handler(&Self::setOrth)));

    XmlElement *description_decl = collect(new XmlIgnoreElement("description", this)); // FFE

    XmlRegularElement *segment = new XmlRegularElementRelay(
	"segment", this,
	startHandler(&Self::startSegment),
	endHandler(&Self::endSegment));
    collect(segment);
    segment->addTransition(0, 0, description_decl);
    segment->addTransition(0, 1, segmentConditionDesc);
    segment->addTransition(0, 1, segmentCondition);
    segment->addTransition(2, 3, segmentConditionDesc);
    segment->addTransition(2, 3, segmentCondition);
    segment->addTransition(0, 2, segmentSpeakerDesc);
    segment->addTransition(0, 2, segmentSpeaker);
    segment->addTransition(1, 3, segmentSpeakerDesc);
    segment->addTransition(1, 3, segmentSpeaker);
    segment->addTransition(0, 4, orth);
    segment->addTransition(1, 4, orth);
    segment->addTransition(2, 4, orth);
    segment->addTransition(3, 4, orth);
    segment->addFinalState(0);
    segment->addFinalState(1);
    segment->addFinalState(2);
    segment->addFinalState(3);
    segment->addFinalState(4);
    segment->ignoreUnknownElements();

    XmlRegularElement *recording = new XmlRegularElementRelay(
	"recording", this,
	startHandler(&Self::startRecording),
	endHandler(&Self::endRecording));
    collect(recording);
    recording->addTransition(0, 0, description_decl);
    recording->addTransition(0, 0, conditionDesc);
    recording->addTransition(0, 0, condition);
    recording->addTransition(0, 0, speakerDesc);
    recording->addTransition(0, 0, speaker);
    recording->addTransition(0, 0, segment);
    recording->addFinalState(0);
    recording->ignoreUnknownElements();

    XmlElement *include_decl =
	collect(new XmlEmptyElementRelay(
		    "include", this,
		    startHandler(&Self::include)));

    XmlMixedElement *subcorpus = new XmlMixedElementRelay(
	// should use XmlRegularElement, but cannot due to nesting restriction
	"subcorpus", this,
	startHandler(&Self::startSubcorpus),
	endHandler(&Self::endSubcorpus),
	0,
	XML_NO_MORE_CHILDREN);
    collect(subcorpus);
    subcorpus->addChild(description_decl);
    subcorpus->addChild(subcorpus);
    subcorpus->addChild(include_decl);
    subcorpus->addChild(conditionDesc);
    subcorpus->addChild(condition);
    subcorpus->addChild(speakerDesc);
    subcorpus->addChild(speaker);
    subcorpus->addChild(recording);

    XmlRegularElement *corpus = new XmlRegularElementRelay(
	"corpus", this,
	startHandler(&Self::startCorpus),
	endHandler(&Self::endCorpus));
    collect(corpus);
    corpus->addTransition(0, 0, description_decl);
    corpus->addTransition(0, 0, subcorpus);
    corpus->addTransition(0, 0, include_decl);
    corpus->addTransition(0, 0, conditionDesc);
    corpus->addTransition(0, 0, condition);
    corpus->addTransition(0, 0, speakerDesc);
    corpus->addTransition(0, 0, speaker);
    corpus->addTransition(0, 0, recording),
    corpus->addFinalState(0);

    setRoot(corpus);
}

CorpusDescriptionParser::CorpusDescriptionParser(const Configuration &c) :
    XmlSchemaParser(c), progressIndicator_(paramProgress(c) ? new Core::ProgressIndicator("CorpusDescriptionParser", "segments") : 0)
{
    initSchema() ;

    isSubParser_    = false ;
    corpusVisitor_  = 0 ;
    superCorpus_    = 0 ;
    corpus_         = 0 ;
    recording_      = 0 ;
    segment_        = 0 ;
    segmentNum_     = 0 ;
    currentSection_ = 0 ;
}

CorpusDescriptionParser::CorpusDescriptionParser(
    const Configuration &c,
    Corpus *_corpus
    ) :
    XmlSchemaParser(c), progressIndicator_(paramProgress(c) ? new Core::ProgressIndicator("CorpusDescriptionParser", "segments") : 0)
{
    initSchema() ;

    isSubParser_    = true ;
    corpusVisitor_  = 0 ;
    superCorpus_    = 0 ;
    corpus_         = _corpus ;
    recording_      = 0 ;
    segment_        = 0 ;
    segmentNum_     = 0 ;
    currentSection_ = 0 ;
}

CorpusDescriptionParser::~CorpusDescriptionParser() {
	delete progressIndicator_;
}

void CorpusDescriptionParser::includeFile(const std::string &relativeFilename) {
    std::string filename(relativeFilename);
    filename = config.resolve(filename);
    filename = Core::joinPaths(corpusDir_, filename);

    CorpusDescriptionParser subParser(config, corpus_) ;
    subParser.accept(filename, corpusVisitor_);
}

void CorpusDescriptionParser::include(const XmlAttributes atts) {
    const char *corpus = atts["file"] ;
    if (!corpus) {
	error("attribute \"file\" missing in \"include\" tag");
	return;
    }
    includeFile(corpus) ;
}

void CorpusDescriptionParser::startCorpus(const XmlAttributes atts) {
    const char *name = atts["name"] ;

    if (!name) {
	error("attribute 'name' missing in 'corpus' tag") ;
	return ;
    }

    if (isSubParser_) {
	if (std::string(name) != corpus_->name()) {
	    warning("name mismatch on included corpus: ")
		<< "'" << name << "' instead of '" << corpus_->name() << "'";
	    return ;
	}
    } else {
	verify(!superCorpus_) ;
	corpus_ = new Corpus();
	corpus_->setName(name) ;
	if (corpusVisitor_)
	    corpusVisitor_->enterCorpus(corpus_) ;
    }
    currentSection_ = corpus_ ;
}

void CorpusDescriptionParser::startSubcorpus(const XmlAttributes atts) {
    const char *name = atts["name"] ;

    if (!name) {
	error("attribute 'name' missing in 'subcorpus' tag") ;
	return ;
    }

    superCorpus_ = corpus_ ;
    corpus_ = new Corpus(superCorpus_) ;

    if (superCorpus_->isNameReserved(name)) {
	error("subcorpus \"%s\" already defined in the section", name);
    } else {
	superCorpus_->reserveName(name);
	corpus_->setName(name);
    }

    currentSection_ = corpus_ ;

    if (corpusVisitor_)
	corpusVisitor_->enterCorpus(corpus_) ;
}

void CorpusDescriptionParser::startRecording(const XmlAttributes atts) {
    const char *name = atts["name"];
    const char *audio = atts["audio"];
    const char *video = atts["video"];

    recording_ = new Recording(corpus_);

    if (!name) {
	error("attribute 'name' missing in 'recording' tag");
    } else {
	if (corpus_->isNameReserved(name)) {
	    error("recording \"%s\" already defined in the section", name);
	} else {
	    corpus_->reserveName(name);
	    recording_->setName(name);
	}
    }

    if (!audio && !video) {
	error("At least one of attribute 'audio' or 'video' has "\
	      "to be specified in 'recording' tag.");
    } else {
	if (audio) {
	    std::string audioFilename(audio);
	    audioFilename = config.resolve(audioFilename);
	    audioFilename = joinPaths(audioDir_, audioFilename);
	    recording_->setAudio(audioFilename);
	}
	if (video) {
	    std::string videoFilename(video);
	    videoFilename = config.resolve(videoFilename);
	    videoFilename = joinPaths(videoDir_, videoFilename);
	    recording_->setVideo(videoFilename);
	}
    }

    recording_->duration_ = Core::Type<Time>::max;
    currentSection_ = recording_;
    segmentNum_ = 0;

    if (progressIndicator_) {
	progressIndicator_->setTask(std::string("CorpusDescriptionParser processing recording '")+name+"'");
	progressIndicator_->start();
    }

    if (corpusVisitor_)
	corpusVisitor_->enterRecording(recording_);
}

void CorpusDescriptionParser::startSegment(const XmlAttributes atts) {
    verify(recording_) ;

    const char *name  = atts["name"] ;
    const char *start = atts["start"] ;
    const char *end   = atts["end"] ;
    const char *track = atts["track"];

    segment_ = new SpeechSegment(recording_) ;
    ++segmentNum_;

    if (progressIndicator_)
	progressIndicator_->notify(segmentNum_);

    if (name) {
	if (recording_->isNameReserved(name)) {
	    error("segment \"%s\" already defined in the section", name);
	} else {
	    recording_->reserveName(name);
	    segment_->setName(name);
	}
    } else {
	std::string autoName = form("%d", segmentNum_);
	while (recording_->isNameReserved(autoName)) autoName += "+";
	recording_->reserveName(autoName);
	segment_->setName(autoName);
    }
    segment_->start_ = (start) ? atof(start) : 0.0;
    segment_->end_   = (end)   ? atof(end)   : recording_->duration();
    if (segment_->start() > segment_->end())
	error("illegal segment boundary times");
    segment_->track_ = (track) ? atoi(track) : 0;
}

void CorpusDescriptionParser::setOrth(const std::string &_orth) {
    verify(segment_);
    std::string orth(_orth);
    enforceTrailingBlank(orth);
    if (shallCaptializeTranscriptions_)
	orth = Core::convertToUpperCase(orth);
    if (shallGemenizeTranscriptions_)
	orth = Core::convertToLowerCase(orth);
    segment_->setOrth(orth);
}

void CorpusDescriptionParser::endSegment() {
    if (corpusVisitor_)
	segment_->accept(corpusVisitor_);
    delete segment_ ;
    segment_ = 0 ;
}

void CorpusDescriptionParser::endRecording() {
    if (corpusVisitor_)
	corpusVisitor_->leaveRecording(recording_) ;
    delete recording_ ;
    recording_ = 0 ;
    currentSection_ = corpus_ ;

    if (progressIndicator_)
	progressIndicator_->finish();
}

void CorpusDescriptionParser::endSubcorpus() {
    if (corpusVisitor_)
	corpusVisitor_->leaveCorpus(corpus_) ;
    delete corpus_ ;
    verify(superCorpus_);
    corpus_ = superCorpus_ ;
    superCorpus_ = dynamic_cast<Corpus*>(corpus_->parent()) ;
    currentSection_ = corpus_ ;
}

void CorpusDescriptionParser::endCorpus() {
    if (!isSubParser_) {
	verify(!superCorpus_) ;
	if (corpusVisitor_)
	    corpusVisitor_->leaveCorpus(corpus_) ;
	delete corpus_ ;
	corpus_ = 0 ;
	currentSection_ = corpus_ ;
    }
}

// ===========================================================================
// speakers

const Speaker *CorpusDescriptionParser::getSpeaker(const std::string &name) const {
    require(currentSection_);

    const Speaker *speaker = currentSection_->speaker(name);
    if (!speaker) {
	error("speaker \"%s\" not defined", name.c_str());
    }

    return speaker;
}

void CorpusDescriptionParser::defineSpeaker(std::auto_ptr<Speaker> &speaker) {
    verify(currentSection_) ; // speaker declaration outside corpus

    Speaker *s = speaker.release();
    s->setParent(currentSection_);
    if (s->isAnonymous()) {
	currentSection_->defaultSpeaker_ = s;
    } else {
	if (currentSection_->speakers_.hasKey(s->name())) {
	    error("speaker \"%s\" already defined in this section" , s->name().c_str());
	    delete s;
	    return;
	}
	currentSection_->speakers_.add(s);
    }
}

void CorpusDescriptionParser::selectDefaultSpeaker(const Core::XmlAttributes atts) {
    verify(currentSection_) ; // speaker declaration outside corpus
    verify(!segment_);

    const char *name = atts["name"];
    if (!name) {
	error("attribute \"name\" missing in \"speaker\" element");
	return;
    }
/*
    if (currentSection_->defaultSpeaker_) {
	error("default speaker for this section already defined");
	return;
    }
*/
    currentSection_->defaultSpeaker_ = getSpeaker(name);
}

void CorpusDescriptionParser::defineSegmentSpeaker(std::auto_ptr<Speaker> &speaker) {
    verify(segment_);

    Speaker *s = speaker.release();
    if (!s->isAnonymous())
	warning("speaker declaration within segment should be anonymous");

    if (segment_->speaker_) {
	error("speaker for this segment already defined");
	delete s;
	return;
    }

    s->setParent(segment_);
    segment_->speaker_ = s;
}

void CorpusDescriptionParser::selectSegmentSpeaker(const Core::XmlAttributes atts) {
    verify(segment_);

    const char *name = atts["name"];
    if (!name) {
	error("attribute \"name\" missing in \"speaker\" element");
	return;
    }

    if (segment_->speaker_) {
	error("speaker for this segment already defined");
	return;
    }

    segment_->speaker_ = getSpeaker(name);
}

// ===========================================================================
// conditions

const AcousticCondition *CorpusDescriptionParser::getCondition(const std::string &name) const {
    require(currentSection_);

    const AcousticCondition *condition = currentSection_->condition(name);
    if (!condition) {
	error("condition \"%s\" not defined", name.c_str());
    }

    return condition;
}

void CorpusDescriptionParser::defineCondition(std::auto_ptr<AcousticCondition> &condition) {
    verify(currentSection_) ; // condition declaration outside corpus

    AcousticCondition *s = condition.release();
    s->setParent(currentSection_);
    if (s->isAnonymous()) {
	currentSection_->defaultCondition_ = s;
    } else {
	if (currentSection_->conditions_.hasKey(s->name())) {
	    error("condition \"%s\" already defined in this section" , s->name().c_str());
	    delete s;
	    return;
	}
	currentSection_->conditions_.add(s);
    }
}

void CorpusDescriptionParser::selectDefaultCondition(const Core::XmlAttributes atts) {
    verify(currentSection_) ; // condition declaration outside corpus
    verify(!segment_);

    const char *name = atts["name"];
    if (!name) {
	error("attribute \"name\" missing in \"condition\" element");
	return;
    }
/*
    if (currentSection_->defaultCondition_) {
	error("default condition for this section already defined");
	return;
    }
*/
    currentSection_->defaultCondition_ = getCondition(name);
}

void CorpusDescriptionParser::defineSegmentCondition(std::auto_ptr<AcousticCondition> &condition) {
    verify(segment_);

    AcousticCondition *s = condition.release();
    if (!s->isAnonymous())
	warning("condition declaration within segment should be anonymous");

    if (segment_->condition_) {
	error("condition for this segment already defined");
	delete s;
	return;
    }

    s->setParent(segment_);
    segment_->condition_ = s;
}

void CorpusDescriptionParser::selectSegmentCondition(const Core::XmlAttributes atts) {
    verify(segment_);

    const char *name = atts["name"];
    if (!name) {
	error("attribute \"name\" missing in \"condition\" element");
	return;
    }

    if (segment_->condition_) {
	error("condition for this segment already defined");
	return;
    }

    segment_->condition_ = getCondition(name);
}

// ===========================================================================
int CorpusDescriptionParser::accept(
    const std::string &filename,
    CorpusVisitor *visitor)
{
    corpusVisitor_ = visitor ;
    corpusDir_ = Core::directoryName(filename);
    audioDir_ = paramAudioDir(config, corpusDir_);
    videoDir_ = paramVideoDir(config, corpusDir_);
    shallCaptializeTranscriptions_ = paramCaptializeTranscriptions(config);
    shallGemenizeTranscriptions_ = paramGemenizeTranscriptions(config);
    return XmlParser::parseFile(filename.c_str()) ;
}
