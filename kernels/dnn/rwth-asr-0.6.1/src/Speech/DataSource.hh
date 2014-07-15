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
#ifndef _SPEECH_DATA_SOURCE_HH
#define _SPEECH_DATA_SOURCE_HH

#include "Feature.hh"
#include <Core/ProgressIndicator.hh>
#include <Flow/DataSource.hh>
#include <Bliss/CorpusDescription.hh>

namespace Speech {

    /** Flow::Network handling several ports
     *
     *  Output is a sequence of Feature object assembled form the output sequence
     *  of the different output ports
     *  Remark: Output type of Flow::Network must be Flow::Vector<Mm::FeatureType> >
     */
    class DataSource :
		public Flow::DataSource
    {
    protected:
		typedef Flow::DataSource Precursor;
    private:
		static const Core::ParameterString paramMainStreamName;
		static const Core::ParameterBool paramNoProgressIndication;
		static const Core::ParameterBool paramPushSinks;
    protected:
	Flow::PortId mainPortId_;
    private:
	Flow::Time startTime_;
		Flow::Time endTime_;
		std::vector<size_t> nFrames_;
		Core::ProgressIndicator progressIndicator_;
		bool noProgressIndication_, pushSinks_;
    private:
		Flow::Time segmentDuration(Bliss::Segment *s);
    protected:
		void updateProgressStatus(Flow::PortId, const Flow::DataPtr<Flow::Data> d);
    public:
		DataSource(const Core::Configuration &c, bool loadFromFile=true);
		~DataSource();

		/** Initializes progress indication and statistics objects. */
		void initialize(Bliss::Segment*);
		/** Finalizes progress indication and statistics objects. */
		void finalize();

		/** Pulls one data object from the network
		 *  @return is false if the port could not deliver an output object.
		 */
		template<class T>
		bool getData(Flow::PortId out, Flow::DataPtr<T> &d);

		/** Pulls one data object from the main port of the network.
		 *  @return is false if the main port could not deliver an output object.
		 */
		template<class T>
		bool getData(Flow::DataPtr<T> &d) { return getData(mainPortId_, d); }

		/** Pulls all the data object from the network
		 *  @return is false if the port could not deliver any output object.
		 */
		template<class T>
		bool getData(Flow::PortId out, std::vector<T> &d);

		/** Pulls one object from the network and converts it into a Feature object.
		 *  @return is false:
		 *    -if the port could not deliver an output object,
		 *    -if the conversion fails.
		 *  The following network types are supported:
		 *    -Flow::Vector<f32>,
		 *    -Flow::PointerVector<Flow::Vector<f32> >.
		 */
		bool getData(Flow::PortId, Core::Ref<Feature> &);

		/** Pulls a feature from main port of the network
		 *  @see bool getData(Flow::PortId, Core::Ref<Feature> &).
		 */
		virtual bool getData(Core::Ref<Feature> &f) { return getData(mainPortId_, f); }
		/** Pulls data from main port of the network
		 *  @return is false if no output could be delivered.
		 */
		virtual bool getData() { Flow::DataPtr<Flow::Data> d; return getData(mainPortId_, d); }

		/** Converts an arbitrary Timestamp derived object to a Feature object.
		 *  @return is false if conversion fails. In this case error, messages
		 *  are generated.
		 */
		virtual bool convert(Flow::DataPtr<Flow::Timestamp> from, Core::Ref<Feature> &to);

		Flow::PortId mainPortId() const { return mainPortId_; }

		/** @return number of frames per port */
		const std::vector<size_t> &nFrames() const { return nFrames_; }
		Flow::Time realTime() const { return endTime_ - startTime_; }

		void setProgressIndication(bool off) { noProgressIndication_ = off; }
    };

    template<class T>
    bool DataSource::getData(Flow::PortId portId, Flow::DataPtr<T> &d)
    {
	Flow::DataPtr<Flow::Data> out;
		if (Precursor::Precursor::getData(portId, out))
			updateProgressStatus(portId, out);
		d = out;
		return d;
    }

    template<class T>
    bool DataSource::getData(Flow::PortId portId, std::vector<T> &v)
    {
		T in;
		v.clear();
		while (getData(portId, in))
			v.push_back(in);
		return v.size();
    }

} // namespace Speech

#endif // _SPEECH_DATA_SOURCE_HH
