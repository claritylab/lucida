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
#ifndef _FLF_PROCESSOR_HH
#define _FLF_PROCESSOR_HH

#include <Core/Component.hh>
#include "Network.hh"

namespace Flf {

    /**
     * Base class of a network processor.
     *
     * Processes a network as long as all nodes are 'good'.
     **/
    class Processor : virtual public Core::Component {
    protected:
	mutable Network *network_;
	NetworkCrawlerRef crawler_;
    public:
	Processor(const Core::Configuration &config, Network *network);
	virtual ~Processor();

	// processed network
	const Network& network() const;
	// default crawler
	const NetworkCrawler& crawler() const;
	// new crawler for asynchronous network processing
	NetworkCrawlerRef newCrawler() const;

	virtual bool init(const std::vector<std::string> &arguments);
	virtual void run();
	virtual void finalize();
    };

    /*
      - Reads a file line by line and interpret each line as list of arguments
      - Provides at port x argument number x
      - Alternatively, read a single list of arguments from command line
    */
    NodeRef createBatchNode(const std::string &name, const Core::Configuration &config);

} // namespace Flf

#endif // _FLF_PROCESSOR_HH
