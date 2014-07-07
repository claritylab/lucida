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
#ifndef _MM_ABSTRACT_ADAPTATION_ACCUMULATOR_HH
#define _MM_ABSTRACT_ADAPTATION_ACCUMULATOR_HH

#include <Core/ReferenceCounting.hh>
#include <Core/BinaryStream.hh>
#include <Core/XmlStream.hh>
#include "Types.hh"
#include "Feature.hh"

namespace Mm {

    /**
     * Abstract interface for adaptation accumulators
     */
    class AbstractAdaptationAccumulator: public Core::ReferenceCounted {
	public:
	virtual void accumulate(
		Core::Ref<const Feature::Vector>,
		DensityIndex,
		MixtureIndex,
		Core::Ref<MixtureSet>) = 0;

	virtual void accumulate(
		Core::Ref<const Feature::Vector>,
		DensityIndex,
		MixtureIndex,
		Core::Ref<MixtureSet>,
		Mm::Weight) = 0;

	virtual std::string typeName() const = 0;
	virtual u32 featureDimension() { return 0; };
	virtual void combine(AbstractAdaptationAccumulator&, Mm::Weight) { defect(); };

	virtual bool read(Core::BinaryInputStream &i) = 0;
	virtual bool write(Core::BinaryOutputStream &o) const = 0;
	virtual void dump(Core::XmlOutputStream&) { defect(); };
    };
}

#endif // _MM_ABSTRACT_ADAPTATION_ACCUMULATOR_HH
