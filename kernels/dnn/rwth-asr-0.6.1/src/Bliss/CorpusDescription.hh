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
#ifndef _BLISS_CORPUS_DESCRIPTION_HH
#define _BLISS_CORPUS_DESCRIPTION_HH

#include <string>
#include <map>
#include <Core/Assertions.hh>
#include <Core/Component.hh>
#include <Core/Parameter.hh>
#include <Core/StringUtilities.hh>


namespace Bliss {

typedef f64 Time; // measured in seconds
typedef u16 TrackId;

class ParentEntity;
class CorpusSection;
class SegmentVisitor;

/**
 * Abstract base class for the various "parts" of a corpus.
 * NamedCorpusEntity provides the name of a corpus part and the
 * concept of parents, reflecting the hierarchical structure of the
 * corpus.
 * @see CorpusDescription
 * @see <a href="../../doc/Bliss.pdf">Bliss documentation</a>
 */

class NamedCorpusEntity {
private:
	ParentEntity *parent_;
	std::string name_;
	static const char *const anonymous /*= "ANONYMOUS"*/;

protected:
	NamedCorpusEntity(ParentEntity *_parent = 0);
	void setParent(ParentEntity *_parent) { parent_ = _parent; }

public:
	virtual ~NamedCorpusEntity() {};

	ParentEntity *parent() const { return parent_; }

	const std::string &name() const {
		return name_;
	}

	std::string fullName() const;

	void setName(const std::string &n) {
		require(n != anonymous);
		require(n.empty() || (n[0] != '/' && n[n.size() - 1] != '/'));
		name_ = n;
	}

	bool isAnonymous() const {
		return name_ == anonymous;
	}
};

/**
 * Someone who speaks.
 * Each instance corresponds to a <speaker-description> element.
 * @see CorpusDescription
 * @see <a href="../../doc/Bliss.pdf">Bliss documentation</a>
 */

class Speaker :
public NamedCorpusEntity
{
	typedef NamedCorpusEntity Precursor;
	friend class SpeakerDescriptionElement;
public:
	enum Gender {unknown, male, female, nGenders};
	static const char *genderId[nGenders];
private:
	Gender gender_;
public:
	// real name
	// age
	Gender gender() const { return gender_; }

	Speaker(ParentEntity *_parent = 0);
	void setParent(ParentEntity *_parent) { Precursor::setParent(_parent); }
};


/**
 * Acoustic condition.
 * Each instance corresponds to a <condition-description> element.
 * @see CorpusDescription
 * @see <a href="../../doc/Bliss.pdf">Bliss documentation</a>
 */

class AcousticCondition :
public NamedCorpusEntity
{
	typedef NamedCorpusEntity Precursor;
	friend class ConditionDescriptionElement;
public:
	// environment
	// microphone
	// channel

	AcousticCondition(ParentEntity *_parent = 0);
	void setParent(ParentEntity *_parent) { Precursor::setParent(_parent); }
};


/**
 * A NamedCorpusEntity which can be parent of other entities.
 * This class exists to prevent Speaker or AcousticCondition to be
 * parent of anything.
 */

class ParentEntity :
public NamedCorpusEntity
{
private:
	std::set<std::string> childrensNames_;

protected:
	ParentEntity(ParentEntity *_parent = 0) :
		NamedCorpusEntity(_parent) {}

public:
	bool isNameReserved(const std::string &n) const {
		return childrensNames_.find(n) != childrensNames_.end();
	}

	void reserveName(const std::string &n) {
		require(!isNameReserved(n));
		childrensNames_.insert(n);
	}
};


template <class T>
class Directory :
private std::map<std::string, T*>
{
	typedef std::map<std::string, T*> Base;
public:
	typedef Directory<T> Self;
	typedef T Entry;

	Directory() {}

	Directory(const Directory &o) {
		for (typename Base::const_iterator i = o.begin(); i != o.end(); ++i)
			Base::insert(std::make_pair(i->first, new T(*i->second)));
	}

	~Directory() {
		for (typename Base::iterator i = this->begin(); i != this->end(); ++i)
			delete i->second;
	}

	bool hasKey(const std::string &k) const {
		return Base::find(k) != this->end() ;
	}

	/**
	 * Add an entry.
	 * The directory will take over ownership of the entry and delete
	 * it when the directory is destroyed.
	 */
	void add(Entry *e) {
		require(!hasKey(e->name()));
		Base::insert(std::make_pair(e->name(), e));
	}

	Entry *lookup(const std::string &k) const {
		typename Base::const_iterator i = Base::find(k);
		return (i != this->end()) ? i->second : 0;
	}
};


/**
 * Abstract base class for the "parts" of a corpus which contain
 * others.
 */

class CorpusSection :
public ParentEntity
{
	typedef ParentEntity Precursor;
	friend class CorpusDescriptionParser;
private:
	u32 level_;

	Directory<AcousticCondition> conditions_;
	const AcousticCondition *defaultCondition_;

	Directory<Speaker> speakers_ ;
	const Speaker *defaultSpeaker_ ;

	std::string defaultLanguage_;

public:
	CorpusSection(CorpusSection *_parent = 0);
	virtual ~CorpusSection() {}

	CorpusSection *parent() const {
		verify(Precursor::parent() == dynamic_cast<CorpusSection*>(Precursor::parent()));
		return static_cast<CorpusSection*>(Precursor::parent());
	}

	u32 level() const { return level_; }

	const Speaker *speaker(const std::string &name) const {
		const Speaker *result = speakers_.lookup(name);
		if (!result && parent()) result = parent()->speaker(name);
		return result;
	}
	const Speaker *defaultSpeaker() const;

	const AcousticCondition *condition(const std::string &name) const {
		const AcousticCondition *result = conditions_.lookup(name);
		if (!result && parent()) result = parent()->condition(name);
		return result;
	}
	const AcousticCondition *defaultCondition() const ;
};


/**
 * A collection of recordings.
 * Each instance corresponds to a <corpus> or <subcorpus> element.
 */

class Corpus :
public CorpusSection
{
	friend class CorpusDescriptionParser ;
public:
	Corpus(Corpus *parentCorpus = 0);
};


/**
 * Continuous audio data.
 * Each instance corresponds to a <recording> element.
 * @see CorpusDescription
 * @see <a href="../../doc/Bliss.pdf">Bliss documentation</a>
 */

class Recording :
public CorpusSection
{
	friend class CorpusDescriptionParser ;
private:
	std::string audio_;
	std::string video_;
	Time duration_;

public:
	const std::string &audio() const { return audio_; }
	void setAudio(const std::string &a) { audio_ = a; }

	const std::string &video() const { return video_; }
	void setVideo(const std::string &v) { video_ = v; }

	Time duration() const { return duration_; }
	void setDuration(Time d) { duration_ = d; }

	Recording(Corpus*);
};


/**
 * General segment of a Recording.
 * Each instance corresponds to a <segment> element.
 * @see CorpusDescription
 * @see <a href="../../doc/Bliss.pdf">Bliss documentation</a>
 */

class Segment :
public ParentEntity
{
	typedef ParentEntity Precursor;
	friend class CorpusDescriptionParser ;
public:
	enum Type {typeSpeech, typeOther, nTypes};
	static const char *typeId[nTypes];
private:
	Recording *recording_; // == parent_ actually redundant

	Type type_;
	Time start_, end_ ;
	TrackId track_;
	const AcousticCondition *condition_;

public:
	Segment(Type, Recording*);

	Recording *recording() const { return recording_ ; }
	void setRecording(Recording *r) {
		recording_ = r;
		setParent(recording_);
	}

	Recording *parent() const {
		verify(Precursor::parent() == dynamic_cast<Recording*>(Precursor::parent()));
		return static_cast<Recording*>(Precursor::parent());
	}

	Type type() const { return type_; }
	void setType(Type t) { type_ = t; }

	/** Start time in seconds from begin of recording */
	Time start() const { return start_; }
	void setStart(Time s) { start_ = s; }

	/** End time in seconds from begin of recording */
	Time end() const { return end_; }
	void setEnd(Time e) { end_ = e; }

	/** Track number of this segment */
	TrackId track() const { return track_; }
	void setTrack(TrackId track) { track_ = track; }

	/**
	 * Acoustic condition of this segment
	 * @return Condition of this segment, or 0 if no condition is
	 * specified in the corpus description.
	 * @see AcousticCondition
	 */
	const AcousticCondition *condition() const {
		return (condition_) ? condition_ : recording()->defaultCondition();
	}

	void setCondition(const AcousticCondition *condition) {
		condition_ = condition;
	}

	/**
	 * Have this segment processed by a SegmentVisitor.
	 * (This is an application of the Visitor pattern, in order to
	 * avoid type-information and downcasts.)
	 */
	virtual void accept(SegmentVisitor*);
};

/**
 * Segment of a Recording containing speech.
 * Extends Segment by (optional) speaker, language and transcription.
 */

class SpeechSegment :
public Segment
{
	friend class CorpusDescriptionParser;
private:
	const Speaker *speaker_ ;
	std::string lang_;
	std::string orth_ ;
public:
	const std::string &orth() const {
		ensure(Core::isWhitespaceNormalized(orth_, Core::requireTrailingBlank)) ;
		return orth_ ;
	}

	void setOrth(std::string o) {
		require(Core::isWhitespaceNormalized(o, Core::requireTrailingBlank)) ;
		orth_ = o ;
	}

	/**
	 * Speaker of this utterance.
	 * @return Speaker of this segment, or 0 if no speaker is
	 * specified in the corpus description
	 * @see Speaker
	 */
	const Speaker *speaker() const {
		return (speaker_) ? speaker_ : recording()->defaultSpeaker();
	}

	void setSpeaker(const Speaker *speaker) {
		speaker_ = speaker;
	}

	SpeechSegment(Recording*) ;

	virtual void accept(SegmentVisitor*);
};


/**
 * Abstract visitor base class for processing segments.
 * @see CorpusVisitor
 */

class SegmentVisitor {
public:
	/**
	 * Template method for processing segments.  Implement this to
	 * process speech and non-speech segments of a corpus.
	 */
	virtual void visitSegment(Segment*) {}

	/**
	 * Template method for processing speech segments. Implement
	 * this to process the speech segments of a corpus. By default the
	 * request will be forwarded to visitSegment(), so ignore this if
	 * you want to treat speech and non-speech segments alike.
	 */
	virtual void visitSpeechSegment(SpeechSegment *s) { visitSegment(s); }

	virtual ~SegmentVisitor() {}
};

/**
 * Abstract visitor base class for corpus traversal.
 * Implement the respective virtual function to process the part of
 * the Corpus.  Note: The concept of Visitors is described in Design
 * Patterns.
 */

class CorpusVisitor :
public SegmentVisitor
{
public:
	/**
	 * Template method called when corpus traversal enters a
	 * recording.  Implement this to take appropriate actions,
	 * e.g. opening the audio file.
	 */
	virtual void enterRecording(Recording*) {}

	/**
	 * Template method called when corpus traversal leaves a
	 * recording.  Implement this to take appropriate clean-up actions,
	 * e.g. closing the audio file.
	 */
	virtual void leaveRecording(Recording*) {}

	/**
	 * Template method notifying start of a (sub-) corpus.
	 * This is rarely needed.
	 */
	virtual void enterCorpus(Corpus*) {}

	/**
	 * Template method notifying end of a (sub-) corpus.
	 * This is rarely needed.
	 */
	virtual void leaveCorpus(Corpus*) {}
};


/**
 * Corpus description.
 *
 * See the <a href="../../doc/Bliss.pdf">Bliss documentation</a> and
 * <a href="../../doc/Corpus.pdf">Corpus Description Format Reference</a>
 * for comprehensive explanation.
 *
 * Design: This is an application of the Visitor pattern: This class
 * represents the corpus - a collection of recordings and segments.
 * The corpus can be traversed by creating an instance of (a subclass
 * of) CorpusVisitor and passing it to accept().
 *
 * Configuration Parameters:
 * - file: name of corpus description file
 * - audio-dir: path prefix for audio files
 * - video-dir: path prefix for video files
 * - progress-indication: show progress meter while traversing corpus (none, local or global)
 * - progress.channel: output XML markup reflecting the structure of the corpus
 */
class ProgressReportingVisitorAdaptor;
class SegmentOrderingVisitor;

class CorpusDescription :
public Core::Component
{
private:
	std::string filename_ ;
	class SegmentPartitionVisitorAdaptor;
	SegmentPartitionVisitorAdaptor *selector_;

	Core::XmlChannel progressChannel_;

	ProgressReportingVisitorAdaptor *reporter_;

	enum ProgressIndcationMode { noProgress, localProgress, globalProgress };
	static const Core::Choice progressIndicationChoice;
	static const Core::ParameterChoice paramProgressIndication;
	ProgressIndcationMode progressIndicationMode_;
	class SegmentCountingVisitor;
	class ProgressIndicationVisitorAdaptor;
	ProgressIndicationVisitorAdaptor *indicator_;
	SegmentOrderingVisitor *ordering_;

public:
	static const Core::ParameterString paramFilename;
	static const Core::ParameterString paramEncoding;
	static const Core::ParameterInt paramPartition;
	static const Core::ParameterInt paramPartitionSelection;
	static const Core::ParameterInt paramSkipFirstSegments;
	static const Core::ParameterStringVector paramSegmentsToSkip;
	static const Core::ParameterBool paramRecordingBasedPartition;
	static const Core::ParameterString paramSegmentOrder;
	static const Core::ParameterBool paramSegmentOrderLookupName;
	static const Core::ParameterBool paramProgressReportingSegmentOrth;

	CorpusDescription(const Core::Configuration&);
	~CorpusDescription();

	const std::string &file() const { return filename_; }

	/**
	 * Traverse corpus.
	 * Each corpus element is processed by the given visitor.  (This
	 * is an application of the Visitor pattern, in order to avoid
	 * concentrate the iteration logic in one place.)
	 */
	void accept(CorpusVisitor*) ;
};

class ProgressReportingVisitorAdaptor :
public CorpusVisitor
{
public:
	ProgressReportingVisitorAdaptor(Core::XmlChannel &ch, bool reportOrth=false);
	void setVisitor(CorpusVisitor *v) { visitor_ = v; }

	virtual void enterCorpus(Corpus *c);
	virtual void leaveCorpus(Corpus *c);
	virtual void enterRecording(Recording *r);
	virtual void leaveRecording(Bliss::Recording *r);
	virtual void visitSegment(Segment *s);
	virtual void visitSpeechSegment(SpeechSegment *s);

private:
	void openSegment(Segment *s);
	void closeSegment(Segment *s);

private:
	CorpusVisitor *visitor_;
	Core::XmlChannel &channel_;
	bool reportSegmentOrth_;
};


} // namespace Bliss

#endif // _BLISS_CORPUS_DESCRIPTION_HH
