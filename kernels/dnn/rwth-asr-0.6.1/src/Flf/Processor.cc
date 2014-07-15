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
#include <Core/Directory.hh>

#include "FlfCore/Utility.hh"
#include "Processor.hh"

namespace Flf {

    // -------------------------------------------------------------------------
    Processor::Processor(const Core::Configuration &config, Network *network) :
	Core::Component(config), network_(network) {
	crawler_ = NetworkCrawlerRef(new NetworkCrawler(network));
    }

    Processor::~Processor() {}

    const Network& Processor::network() const {
	return *network_;
    }

    const NetworkCrawler& Processor::crawler() const {
	return *crawler_;
    }

    NetworkCrawlerRef Processor::newCrawler() const {
	return NetworkCrawlerRef(new NetworkCrawler(network_));
    }

    bool Processor::init(const std::vector<std::string> &arguments) {
	crawler_->reset();
	return network_->init(*crawler_, arguments);
    }

    void Processor::run() {
	do {
	    crawler_->reset();
	    network_->pull();
	} while (network_->sync(*crawler_));
    }

    void Processor::finalize() {
	crawler_->reset();
	network_->finalize(*crawler_);
    }
    // -------------------------------------------------------------------------


    // -------------------------------------------------------------------------
    class BatchNode : public Node {
    private:
	static const std::string emptyString;
    public:
	static const Core::ParameterString paramFile;
	static const Core::ParameterString paramEncoding;

    private:
	typedef std::vector<std::string> StringList;
	typedef std::vector<StringList> StringMatrix;

    private:
	StringMatrix batches_;
	StringMatrix::const_iterator itBatch_;

    protected:
	const std::string & getArgument(u32 i) {
	    require(itBatch_ != batches_.end());
	    if (!(i < itBatch_->size())) {
		// criticalError("BatchNode: Requested argument %d is out of range.", i);
		return emptyString;
	    }
	    return (*itBatch_)[i];
	}

    public:
	BatchNode(const std::string &name, const Core::Configuration &config) :
	    Node(name, config) {}
	virtual ~BatchNode() {}

	virtual void init(const std::vector<std::string> &arguments) {
	    u32 nMaxColumns = 0;
	    std::string filename = paramFile(config);
	    if (!filename.empty()) {
		if (!Core::isValidPath(filename))
		    criticalError("BatchNode: Could not find \"%s\"",
				  filename.c_str());
		TextFileParser tf(filename, paramEncoding(config));
		if (tf) {
		    log("BatchNode: Read from \"%s\"",
			filename.c_str());
		    for (;;) {
			const TextFileParser::StringList &columns = tf.next();
			if (tf) {
			    nMaxColumns = std::max(u32(columns.size()), nMaxColumns);
			    batches_.push_back(columns);
			} else
			    break;
		    }
		} else
		    criticalError("Could not open \"%s\"", filename.c_str());
	    } else if (!arguments.empty()) {
		log("BatchNode: Read from command line");
		nMaxColumns = arguments.size();
		batches_.push_back(arguments);
	    }
	    if (!batches_.empty())
		log("BatchNode: Found %d batches with (up to) %d arguments",
		    (u32)batches_.size(), nMaxColumns);
	    else
		error("BatchNode: No input");
	    itBatch_ = batches_.begin();
	}

	virtual void sync() {
	    require(itBatch_ != batches_.end());
	    ++itBatch_;
	}

	virtual bool good() {
	    return itBatch_ != batches_.end();
	}

	virtual void finalize() {
	    if (good())
		warning("BatchNode: Pending batches exists.");
	}

	virtual bool sendBool(Port to) {
	    bool b = false;
	    if (!Core::strconv(getArgument(to), b))
		criticalError("BatchNode: Failed to convert %s",
			      getArgument(to).c_str());
	    return b;
	}
	virtual s32 sendInt(Port to) {
	    s32 i = 0;
	    if (!Core::strconv(getArgument(to), i))
		criticalError("BatchNode: Failed to convert %s",
			      getArgument(to).c_str());
	    return i;
	}
	virtual f64 sendFloat(Port to) {
	    f64 f = 0.0;
	    if (!Core::strconv(getArgument(to), f))
		criticalError("BatchNode: Failed to convert %s",
			      getArgument(to).c_str());
	    return f;
	}
	virtual std::string sendString(Port to) {
	    return getArgument(to);
	}
    };
    const std::string BatchNode::emptyString = std::string();
    const Core::ParameterString BatchNode::paramFile(
	"file",
	"text file",
	"");
    const Core::ParameterString BatchNode::paramEncoding(
	"encoding",
	"encoding of file",
	"utf-8");

    NodeRef createBatchNode(const std::string &name, const Core::Configuration &config) {
	return NodeRef(new BatchNode(name, config));
    }
    // -------------------------------------------------------------------------

} // namespace Flf
