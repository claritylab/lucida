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
#ifndef _FLOW_DUMP_HH
#define _FLOW_DUMP_HH

#include <Core/XmlStream.hh>
#include <Core/Application.hh>
#include "VectorScalarFunction.hh"
#include "Node.hh"



namespace Flow {

    /** Flow network. */
    class DumpNode : public SleeveNode {
    protected:
	static const Core::ParameterString paramFilename;
	static const Core::ParameterBool paramUnbuffered;

	bool isOpen_;
	bool unbuffered_;
	Core::XmlOutputStream f_;

	void open(const std::string &filename);
	void close();

    public:
	static std::string filterName() { return "generic-dump"; }
	DumpNode(const Core::Configuration &c);
	virtual ~DumpNode();

	virtual bool setParameter(const std::string &name, const std::string &value);
	virtual bool work(PortId output);
    };


    /**
     * Dump node for vector absolutes
     * @todo remove this node. can be implemented as a flow network.
     */
    template<typename content> class VectorAbsValDumpNode : public DumpNode {
    private:
	NormFunction<content> normFunction_;

    public:
	VectorAbsValDumpNode(const Core::Configuration &c) :
	    Core::Component(c), DumpNode(c), normFunction_(c) {}


	virtual bool configure() {
	    Core::Ref<const Attributes> a = getInputAttributes(0);
	    if (!configureDatatype(a, Vector<content>::type()))
		return false;
	    if (!putOutputAttributes(0, a))
		return false;

	    return true;
	}

	virtual bool work(PortId output){
	    DataPtr<Vector<content> > d;
	    getData(0, d);

	    if (!(isOpen_ && d)) {
		Core::Application::us()->log() << "Could not dump data.";
		close();
	    }
	    else {
		f64 absVal = normFunction_.apply(*d, (f64) 2);
		f_ << absVal << "\n";
	    }

	    return putData(0, d.get());
	}

	static std::string filterName() {
	    return std::string("generic-vector-") + Core::Type<content>::name + "-abs-dump";
	}
    };
}

#endif // _FLOW_DUMP_HH
