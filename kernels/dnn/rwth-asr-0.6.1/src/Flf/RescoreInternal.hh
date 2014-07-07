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
#ifndef _FLF_RESCORE_INTERNAL_HH
#define _FLF_RESCORE_INTERNAL_HH

#include <Core/Parameter.hh>

#include "FlfCore/Lattice.hh"
#include "Network.hh"

namespace Flf {


    RescoreMode getRescoreMode(const Core::Configuration &config);


    // -------------------------------------------------------------------------
    /**
     * Abstract rescore node
     **/
    class RescoreNode : public FilterNode {
	friend class Network;
	typedef FilterNode Precursor;
    public:
	static const Core::ParameterString paramRescoreMode;
    protected:
	RescoreMode rescoreMode;
    protected:
	virtual bool init_(NetworkCrawler &crawler, const std::vector<std::string> &arguments);
	virtual ConstLatticeRef filter(ConstLatticeRef l);
	virtual void setRescoreMode(RescoreMode rescoreMode);
	virtual ConstLatticeRef rescore(ConstLatticeRef l) = 0;
    public:
	RescoreNode(const std::string &name, const Core::Configuration &config);
	virtual ~RescoreNode() {}
    };


    /**
     * Abstract single dimension rescore node
     * - Extend the score of a single dimension; that dimension might be newly appended
     **/
    class RescoreSingleDimensionNode : public RescoreNode {
	friend class Network;
	typedef RescoreNode Precursor;
    public:
	static const Core::ParameterBool   paramAppend;
	static const Core::ParameterFloat  paramScale;
	static const Core::ParameterString paramKey;
    private:
	bool append_;
	Key key_;
	Score appendScale_;
	Score scoreScale_;
	ConstSemiringRef lastSemiring_;
	ConstSemiringRef lastExtendedSemiring_;
    protected:
	virtual bool init_(NetworkCrawler &crawler, const std::vector<std::string> &arguments);
	Score scale() const { return scoreScale_; }
	virtual ConstLatticeRef rescore(ConstLatticeRef l);
	virtual ConstLatticeRef rescore(ConstLatticeRef l, ScoreId id) = 0;
    public:
	RescoreSingleDimensionNode(const std::string &name, const Core::Configuration &config);
	virtual ~RescoreSingleDimensionNode() {}
    };
    // -------------------------------------------------------------------------

} // namespace Flf
#endif // _FLF_RESCORE_INTERNAL_HH
