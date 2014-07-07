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
#ifndef _SPEECH_FEATURE_HH
#define _SPEECH_FEATURE_HH

#include <Mm/Feature.hh>
#include <Flow/Vector.hh>
#include <Flow/TypedAggregate.hh>

namespace Speech {

    /** Interface conversion between Flow::VectorPointer<...> and Mm::Feature.
     *  Although, Flow::VectorPointer<...> can be accessed by Core::TsRef<...> even outside of Flow,
     *  Feature copies the elements of Flow::VectorPointer<...> into an Mm::Feature object. Advantage
     *  is that Mm::Feature is not multithread safe thus it can be accessed by the light-weigt Core::Ref.
     *  @see Flow::TypedAggregate for more comments.
     */
    class Feature : public Mm::Feature {
	typedef Feature Self;
	typedef Mm::Feature Precursor;
    public:
	typedef Flow::Vector<Mm::FeatureType> FlowVector;
	typedef Flow::TypedAggregate<FlowVector> FlowFeature;

    private:
	/** Remark: Flow::Data part of this data structure is superflouos.
	 *   Redesign of Flow::Timestamp could solve this problem.
	 */
	Flow::Timestamp timestamp_;
    private:
	static Core::Ref<const Vector> convert(Flow::DataPtr<FlowVector>&);

    public:
	Feature() {}
	Feature(Flow::DataPtr<FlowVector> &v) { take(v); }
	Feature(Flow::DataPtr<FlowFeature> &f) { take(f); }
	virtual ~Feature() {}

	/** Converts the content of @param v.
	 *  Note: content of v gets destroyed (@see convert(..)).
	 */
	void take(Flow::DataPtr<FlowVector> &v);
	/** Converts the content of @param f.
	 *  Note: content of f gets destroyed (@see  convert(..)).
	 */
	void take(Flow::DataPtr<FlowFeature> &f);

	/** Converts the content of @param t.
	 *  Note: content of t gets destroyed (@see convert(...)).
	 *  @return is false if elements of @param t could not be converted.
	 */
	bool take(Flow::DataPtr<Flow::Timestamp> &t);

	void setTimestamp(const Flow::Timestamp &t) { timestamp_ = t; }
	const Flow::Timestamp &timestamp() const { return timestamp_; }

	virtual Mm::FeatureDescription* getDescription(const Core::Configurable &parent) const;
    };

} // namespace Core

#endif // _SPEECH_FEATURE_HH
