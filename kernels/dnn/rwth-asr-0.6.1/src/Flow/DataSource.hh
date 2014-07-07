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
#ifndef _FLOW_DATA_SOURCE_HH
#define _FLOW_DATA_SOURCE_HH

#include <Core/ReferenceCounting.hh>
#include <Flow/Network.hh>

namespace Flow {

    /** Flow::Network handling several ports
     */
    class DataSource :
		public Core::ReferenceCounted,
		public Network
    {
	protected:
		typedef Network Precursor;
    public:
		DataSource(const Core::Configuration &c, bool loadFromFile=true) :
			Component(c),
			Precursor(c, loadFromFile)
		{};

		~DataSource() {};

		/** Pulls one data object from the network
		 *  @return is false if the port could not deliver an output object.
		 */
		template<class T>
		bool getData(Flow::PortId out, Flow::DataPtr<T> &d);
		/** Pulls all the data object from the network
		 *  @return is false if the port could not deliver any output object.
		 */
		template<class T>
		bool getData(Flow::PortId out, std::vector<T> &d);
    };

    template<class T>
    bool DataSource::getData(Flow::PortId portId, Flow::DataPtr<T> &d)
    {
		Flow::DataPtr<Flow::Data> out;
		Precursor::getData(portId, out);
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

} // namespace Flow

#endif // _FLOW_DATA_SOURCE_HH
