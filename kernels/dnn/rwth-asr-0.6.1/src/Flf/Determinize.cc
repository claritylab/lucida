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
#include <Core/Parameter.hh>

#include "FlfCore/Basic.hh"
#include "FlfCore/Ftl.hh"
#include "Determinize.hh"

namespace Flf {

    // -------------------------------------------------------------------------
    ConstLatticeRef determinize(ConstLatticeRef l) {
	ConstLatticeRef k = FtlWrapper::determinize(l, false);
	k->setBoundaries(InvalidBoundaries);
	return k;
    }

    class DeterminizeNode : public FilterNode {
	friend class Network;
    public:
	static const Core::ParameterBool paramToLogSemiring;
	static const Core::ParameterFloat paramAlpha;
    private:
	bool toLogSemiring_;
	f32 alpha_;
	ConstSemiringRef lastSemiring_;
	ConstSemiringRef logSemiring_;
    protected:
	virtual ConstLatticeRef filter(ConstLatticeRef l) {
	    if (!l)
		return ConstLatticeRef();
	    if (toLogSemiring_) {
		if (!lastSemiring_ || (lastSemiring_.get() != l->semiring().get())) {
		    lastSemiring_ = l->semiring();
		    logSemiring_ = toLogSemiring(lastSemiring_, alpha_);
		}
		l = changeSemiring(l, logSemiring_);
	    }
	    l = determinize(l);
	    if (toLogSemiring_) {
		l = changeSemiring(l, lastSemiring_);
	    }
	    return l;
	}
    public:
	DeterminizeNode(const std::string &name, const Core::Configuration &config) :
	    FilterNode(name, config), toLogSemiring_(false) {}
	~DeterminizeNode() {}
	virtual void init(const std::vector<std::string> &arguments) {
	    toLogSemiring_ = paramToLogSemiring(config);
	    if (toLogSemiring_) {
		alpha_ = paramAlpha(select("log-semiring"));
		log() << "Use log-semiring with alpha=" << alpha_;
	    }
	}
    };
    const Core::ParameterBool DeterminizeNode::paramToLogSemiring(
	"log-semiring",
	"use log semiring",
	false);
   const Core::ParameterFloat DeterminizeNode::paramAlpha(
       "alpha",
       "scale dimensions for posterior calculation",
       0.0);

    NodeRef createDeterminizeNode(const std::string &name, const Core::Configuration &config) {
	return NodeRef(new DeterminizeNode(name, config));
    }
    // -------------------------------------------------------------------------


    // -------------------------------------------------------------------------
    ConstLatticeRef minimize(ConstLatticeRef l) {
	ConstLatticeRef k = FtlWrapper::minimize(l);
	k->setBoundaries(InvalidBoundaries);
	return k;
    }

    class MinimizeNode : public FilterNode {
	friend class Network;
    protected:
	virtual ConstLatticeRef filter(ConstLatticeRef l) {
	    return minimize(l);
	}
    public:
	MinimizeNode(const std::string &name, const Core::Configuration &config) :
	    FilterNode(name, config) {}
	~MinimizeNode() {}
    };

    NodeRef createMinimizeNode(const std::string &name, const Core::Configuration &config) {
	return NodeRef(new MinimizeNode(name, config));
    }
    // -------------------------------------------------------------------------

} // namespace Flf
