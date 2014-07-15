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
#include <Test/UnitTest.hh>
#include <Test/File.hh>
#include <Bliss/CorpusDescription.hh>

class TestCorpusVisitor : public Bliss::CorpusVisitor {
public:
    TestCorpusVisitor() : lCorpus_(0), lRecording_(0) {}
    void enterCorpus(Bliss::Corpus *corpus) {
	++lCorpus_;
	corpus_.push_back(corpus->fullName());
    }
    void leaveCorpus(Bliss::Corpus *corpus) {
	--lCorpus_;
    }
    void enterRecording(Bliss::Recording *recording) {
	++lRecording_;
	recordings_.push_back(recording->fullName());
    }
    void leaveRecording(Bliss::Recording *recording) {
	--lRecording_;
    }
    void visitSpeechSegment(Bliss::SpeechSegment *segment) {
	segments_.push_back(segment->fullName());
	if (segment->speaker())
	    speakers_.push_back(segment->speaker()->fullName());
	else
	    speakers_.push_back("");
	if (segment->condition())
	    conditions_.push_back(segment->condition()->fullName());
	else
	    conditions_.push_back("");
    }

    const std::vector<std::string>& recordings() const {
	return recordings_;
    }

    const std::vector<std::string>& segments() const {
	return segments_;
    }

    const std::vector<std::string>& corpus() const {
	return corpus_;
    }
    const std::vector<std::string>& speakers() const {
	return speakers_;
    }
    const std::vector<std::string>& conditions() const {
	return conditions_;
    }
    int nOpenCorpus() const { return lCorpus_; }
    int nOpenRecording() const { return lRecording_; }

protected:
    int lCorpus_, lRecording_;
    std::vector<std::string> corpus_, recordings_, segments_, speakers_, conditions_;
};


class SegmentOrderingTest : public Test::ConfigurableFixture
{
public:
    void setUp();
    void tearDown();

protected:
    void createCorpus(bool subCorpus = false);
    void processCorpus(Bliss::CorpusVisitor *visistor) const;
    static const size_t nRecordings, nSegments, nSubCorpus;
    static const std::string corpusName;
    ::Test::Directory *tmpDir_;
    std::string corpusFile_, orderFile_;
    std::vector<std::string> recordings_;
    std::vector<std::string> segments_;
    std::vector<std::string> speakers_;
    std::vector<std::string> conditions_;
};

const size_t SegmentOrderingTest::nRecordings = 10;
const size_t SegmentOrderingTest::nSegments = 3;
const size_t SegmentOrderingTest::nSubCorpus = 2;
const std::string SegmentOrderingTest::corpusName = "test";

void SegmentOrderingTest::setUp()
{
    tmpDir_ = new ::Test::Directory;
    corpusFile_ = ::Test::File(*tmpDir_, "test.corpus").path();
    orderFile_ = ::Test::File(*tmpDir_, "segments").path();
    createCorpus();
    setParameter("*.channel", "nil");
    setParameter("*.error.channel", "stderr");
    setParameter("*.corpus.file", corpusFile_);
    setParameter("*.corpus.segment-order", orderFile_);
}

void SegmentOrderingTest::tearDown()
{
    delete tmpDir_;
}

void SegmentOrderingTest::createCorpus(bool subCorpus)
{
    recordings_.clear();
    segments_.clear();
    speakers_.clear();
    conditions_.clear();
    Core::XmlOutputStream os(corpusFile_);
    os << Core::XmlOpen("corpus") + Core::XmlAttribute("name", corpusName);
    std::string condition = "cond";
    os << Core::XmlEmpty("condition-description") + Core::XmlAttribute("name", condition);
    std::string speaker = Core::form("speaker");
    os << Core::XmlEmpty("speaker-description") + Core::XmlAttribute("name", speaker);
    int nSub = (subCorpus ? nSubCorpus : 1);
    for (int c = 0; c < nSub; ++c) {
	std::string corpus = "";
	if (subCorpus) {
	    corpus = Core::form("sub-%d", c);
	    condition = Core::form("cond-%d", c);
	    os << Core::XmlOpen("subcorpus") + Core::XmlAttribute("name", corpus);
	    os << Core::XmlEmpty("condition-description") + Core::XmlAttribute("name", condition);
	    speaker = Core::form("speaker-%d", c);
	    os << Core::XmlEmpty("speaker-description") + Core::XmlAttribute("name", speaker);
	}
	for (uint r = 0; r < nRecordings; ++r) {
	    std::string recording = Core::form("recording-%d-%d", c, r);
	    os << Core::XmlOpen("recording") +
		    Core::XmlAttribute("name", recording) +
		    Core::XmlAttribute("audio", "none");

	    if (subCorpus)
		recording = corpusName + "/" + corpus + "/" + recording;
	    else
		recording = corpusName + "/" + recording;
	    recordings_.push_back(recording);
	    std::string recSpeaker = Core::form("speaker-%d-%d", c, r);
	    os << Core::XmlEmpty("speaker-description") + Core::XmlAttribute("name", recSpeaker);
	    for (uint s = 0; s < nSegments; ++s) {
		std::string segment = Core::form("segment-%d-%d-%d", c, r, s);
		os << Core::XmlOpen("segment") + Core::XmlAttribute("name", segment);
		os << Core::XmlEmpty("condition") + Core::XmlAttribute("name", condition);
		const std::string &curSpeaker = (s ? speaker : recSpeaker);
		os << Core::XmlEmpty("speaker") + Core::XmlAttribute("name", curSpeaker);
		os << Core::XmlClose("segment");
		segment = recording + "/" + segment;
		std::string speakerId = corpusName + "/";
		if (subCorpus) speakerId += corpus + "/";
		if (s)
		    speakerId += curSpeaker;
		else
		    speakerId = recording + "/" + curSpeaker;

		std::string conditionId = corpusName + "/";
		if (subCorpus) conditionId += corpus + "/";
		conditionId += condition;
		segments_.push_back(segment);
		speakers_.push_back(speakerId);
		conditions_.push_back(conditionId);
	    }
	    os << Core::XmlClose("recording");
	}
	if (subCorpus) {
	    os << Core::XmlClose("subcorpus");
	}
    }
    os << Core::XmlClose("corpus");
}

void SegmentOrderingTest::processCorpus(Bliss::CorpusVisitor *visitor) const
{
    Bliss::CorpusDescription description(select("corpus"));
    description.accept(visitor);
}


TEST_F(Bliss, SegmentOrderingTest, Standard) {
    {
	Core::TextOutputStream os(orderFile_);
	for (std::vector<std::string>::const_iterator s = segments_.begin(); s != segments_.end(); ++s)
	    os << *s << std::endl;
    }
    TestCorpusVisitor visitor;
    processCorpus(&visitor);
    const std::vector<std::string> &visitedRecordings = visitor.recordings();
    const std::vector<std::string> &visitedSegments = visitor.segments();
    EXPECT_EQ(0, visitor.nOpenCorpus());
    EXPECT_EQ(0, visitor.nOpenRecording());
    EXPECT_EQ(size_t(1), visitor.corpus().size());
    EXPECT_EQ(corpusName, visitor.corpus().front());
    EXPECT_EQ(nRecordings, visitedRecordings.size());
    EXPECT_EQ(nSegments * nRecordings, visitedSegments.size());

    for (uint r = 0; r < nRecordings; ++r)
	EXPECT_EQ(recordings_[r], visitedRecordings[r]);
    for (uint s = 0; s < nRecordings * nSegments; ++s)
	EXPECT_EQ(segments_[s], visitedSegments[s]);
}

TEST_F(Bliss, SegmentOrderingTest, ReverseOrder) {
    {
	Core::TextOutputStream os(orderFile_);
	for (std::vector<std::string>::const_reverse_iterator s = segments_.rbegin(); s != segments_.rend(); ++s)
	    os << *s << std::endl;
    }
    TestCorpusVisitor visitor;
    processCorpus(&visitor);
    const std::vector<std::string> &visitedSegments = visitor.segments();
    EXPECT_EQ(0, visitor.nOpenCorpus());
    EXPECT_EQ(0, visitor.nOpenRecording());
    EXPECT_EQ(size_t(1), visitor.corpus().size());
    EXPECT_EQ(corpusName, visitor.corpus().front());
    EXPECT_EQ(nSegments * nRecordings, visitedSegments.size());
    std::vector<std::string>::const_iterator v = visitedSegments.begin();
    for (std::vector<std::string>::const_reverse_iterator s = segments_.rbegin(); s != segments_.rend(); ++s, ++v)
	EXPECT_EQ(*s, *v);
}

TEST_F(Bliss, SegmentOrderingTest, RepeatedRecording)
{
    {
	Core::TextOutputStream os(orderFile_);
	for (uint s = 0; s < nSegments; ++s)
	    for (uint r = 0; r < nRecordings; ++r)
		os << segments_[r*nSegments + s] << std::endl;
    }
    TestCorpusVisitor visitor;
    processCorpus(&visitor);
    const std::vector<std::string> &visitedRecordings = visitor.recordings();
    const std::vector<std::string> &visitedSegments = visitor.segments();
    EXPECT_EQ(0, visitor.nOpenCorpus());
    EXPECT_EQ(0, visitor.nOpenRecording());
    EXPECT_EQ(size_t(1), visitor.corpus().size());
    EXPECT_EQ(corpusName, visitor.corpus().front());
    EXPECT_EQ(nSegments * nRecordings, visitedRecordings.size());
    EXPECT_EQ(nSegments * nRecordings, visitedSegments.size());
    int v = 0;
    for (uint s = 0; s < nSegments; ++s)
	for (uint r = 0; r < nRecordings; ++r, ++v)
	    EXPECT_EQ(segments_[r*nSegments + s], visitedSegments[v]);
}


TEST_F(Bliss, SegmentOrderingTest, SubCorpus)
{
    createCorpus(true);
    {
	Core::TextOutputStream os(orderFile_);
	for (uint s = 0; s < nSegments; ++s)
	    for (uint r = 0; r < nRecordings; ++r)
		for (uint c = 0; c < nSubCorpus; ++c)
		    os << segments_[c*nRecordings*nSegments + r*nSegments + s] << std::endl;
    }
    TestCorpusVisitor visitor;
    processCorpus(&visitor);
    const std::vector<std::string> &visitedRecordings = visitor.recordings();
    const std::vector<std::string> &visitedSegments = visitor.segments();
    size_t nseg = nSubCorpus * nRecordings * nSegments;
    EXPECT_EQ(0, visitor.nOpenCorpus());
    EXPECT_EQ(0, visitor.nOpenRecording());
    EXPECT_EQ(nseg + 1, visitor.corpus().size());
    EXPECT_EQ(corpusName, visitor.corpus().front());
    EXPECT_EQ(nseg, visitedRecordings.size());
    EXPECT_EQ(nseg, visitedSegments.size());
    int v = 0;
    for (uint s = 0; s < nSegments; ++s)
	for (uint r = 0; r < nRecordings; ++r)
	    for (uint c = 0; c < nSubCorpus; ++c, ++v)
		EXPECT_EQ(segments_[c*nRecordings*nSegments + r*nSegments + s], visitedSegments[v]);
}

TEST_F(Bliss, SegmentOrderingTest, Speaker)
{
    createCorpus(true);
    {
	Core::TextOutputStream os(orderFile_);
	for (std::vector<std::string>::const_iterator s = segments_.begin(); s != segments_.end(); ++s)
	    os << *s << std::endl;
    }
    TestCorpusVisitor visitor;
    processCorpus(&visitor);
    const std::vector<std::string> &speakers = visitor.speakers();
    EXPECT_EQ(speakers_.size(), speakers.size());
    for (uint s = 0; s < speakers_.size(); ++s)
	EXPECT_EQ(speakers_[s], speakers[s]);
}

TEST_F(Bliss, SegmentOrderingTest, Condition)
{
    createCorpus(true);
    {
	Core::TextOutputStream os(orderFile_);
	for (std::vector<std::string>::const_iterator s = segments_.begin(); s != segments_.end(); ++s)
	    os << *s << std::endl;
    }
    TestCorpusVisitor visitor;
    processCorpus(&visitor);
    const std::vector<std::string> &conditions = visitor.conditions();
    EXPECT_EQ(conditions_.size(), conditions.size());
    for (uint s = 0; s < conditions_.size(); ++s)
	EXPECT_EQ(conditions_[s], conditions[s]);
}
