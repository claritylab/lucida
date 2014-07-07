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
#ifndef _SPEECH_DATA_EXTRACTOR_HH
#define _SPEECH_DATA_EXTRACTOR_HH

#include "CorpusProcessor.hh"
#include <Core/Dependency.hh>
#include <Mm/Feature.hh>
#include <Flow/Vector.hh>

namespace Am {
    class AcousticModel;
}

namespace Speech {

    /**
     * DataExtractor is a corpus visitor application for extraction of any data type.
     *
     * Output (XML format):
     * - number of recording/segments processed (channel: statistics)
     * - number of extracted data (channel: statistics)
     */

    class DataExtractor :
	public CorpusProcessor
    {
	typedef CorpusProcessor Precursor;
    private:
	Core::Ref<DataSource> dataSource_;
	Core::XmlChannel statisticsChannel_;
	u32 nRecordings_, nSegments_;
	std::vector<std::string> portNames_;
	std::vector<size_t> nFrames_;
    protected:
	void setDataSource(Core::Ref<DataSource> s) { dataSource_ = s; }
	Core::Ref<DataSource> dataSource() const { return dataSource_; }
    public:
	DataExtractor(const Core::Configuration &c, bool loadFromFile=true);
	virtual ~DataExtractor() {}

	virtual void signOn(CorpusVisitor &corpusVisitor);

	virtual void enterCorpus(Bliss::Corpus*);
	virtual void leaveCorpus(Bliss::Corpus*);
	virtual void enterRecording(Bliss::Recording*);
	virtual void enterSegment(Bliss::Segment*);
	virtual void leaveSegment(Bliss::Segment*);
	virtual void processSegment(Bliss::Segment*);
    };

    /**
     * FeatureExtractor is a corpus visitor application for feature extraction.
     */
    class FeatureExtractor :
	public DataExtractor
    {
	typedef DataExtractor Precursor;
    protected:
	/** Override this function to achieve the attributes of the featur streams.
	 *  This function is called once before the first feature is processed.
	 */
	virtual void setFeatureDescription(const Mm::FeatureDescription &) {}

	/** Override this function to achieve the features extacted form data source one-by-one.
	 */
	virtual void processFeature(Core::Ref<const Feature>) {}
    public:
	FeatureExtractor(const Core::Configuration &c, bool loadFromFile=true) : Core::Component(c), Precursor(c, loadFromFile) {}
	virtual ~FeatureExtractor() {}

	virtual void processSegment(Bliss::Segment* segment);
    };

    /**
     * FeatureVectorExtractor is a corpus visitor application for feature vector extraction.
     */
    class FeatureVectorExtractor :
	public FeatureExtractor
    {
	typedef FeatureExtractor Precursor;
    protected:
	virtual void setFeatureDescription(const Mm::FeatureDescription &d) {
	    d.verifyNumberOfStreams(1, true); setFeatureVectorDescription(d.mainStream());
	}
	/** Override this function to achieve the attributes of the feature vector.
	 *  This function is called once before the first feature vector is processed.
	 */
	virtual void setFeatureVectorDescription(const Mm::FeatureDescription::Stream &) {}

	virtual void processFeature(Core::Ref<const Feature> f) {
	    processFeatureVector(f->mainStream());
	}
	/** Override this function to achieve the feature vectors extacted form data source one-by-one.
	 */
	virtual void processFeatureVector(Core::Ref<const Feature::Vector>) {}
    public:
	FeatureVectorExtractor(const Core::Configuration &c) : Core::Component(c), Precursor(c) {}
	virtual ~FeatureVectorExtractor() {}
    };

} // namespace Speech

#endif // _SPEECH_DATA_EXTRACTOR_HH
