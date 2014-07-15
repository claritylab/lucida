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
#ifndef _FLF_TIME_ALIGNMENT_HH
#define _FLF_TIME_ALIGNMENT_HH

#include "FlfCore/Lattice.hh"
#include "FwdBwd.hh"
#include "Network.hh"


namespace Flf {

    class TimeAlignmentBuilder;
    typedef Core::Ref<TimeAlignmentBuilder> TimeAlignmentBuilderRef;
    class TimeAlignmentBuilder : public Core::ReferenceCounted {
    private:
	class Internal;
    private:
	Internal *internal_;
    public:
	TimeAlignmentBuilder(const Core::Configuration &config, FwdBwdBuilderRef fbBuilder);
	~TimeAlignmentBuilder();
	void dump(std::ostream &os) const;
	void dumpStatistics();
	ConstLatticeRef align(ConstLatticeRef lHyp, ConstPosteriorCnRef cnRef);
	ConstLatticeRef align(ConstLatticeRef lHyp, ConstLatticeRef lRef);
	ConstLatticeRef align(ConstLatticeRef lHyp, ConstLatticeRef lRef, ConstPosteriorCnRef cnRef);

	static TimeAlignmentBuilderRef create(const Core::Configuration &config, FwdBwdBuilderRef fbBuilder = FwdBwdBuilderRef());
    };

    NodeRef createTimeAlignmentNode(const std::string &name, const Core::Configuration &config);

} // namespace Flf


#endif // _FLF_TIME_ALIGNMENT_HH
