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
#ifndef _MM_FEATURE_HH
#define _MM_FEATURE_HH

#include "Types.hh"
#include <Mm/PointerVector.hh>
#include <Core/Component.hh>
#include <Core/ReferenceCounting.hh>
#include <Core/Dependency.hh>
#include <Core/Description.hh>

namespace Mm {

    /** Feature is container for objects comming from different feature stream at the same time.
     *  Remark: applications working on sinlge streams should use the "main stream"
     *    which is currently the first stream.
     *  Note: Currently it can contain only vectors.
     */
    class Feature : public Core::ReferenceCounted {
    public:
	class Vector : public FeatureVector, public Core::ReferenceCounted {
	    typedef FeatureVector Precursor;
	public:
	    Vector() {}
	    Vector(size_t size) : Precursor(size) {}
	    Vector(const Precursor &v) : Precursor(v) {}
	};

	typedef std::vector<Core::Ref<const Vector> >::const_iterator Iterator;

    public:
	static Core::Ref<const Vector> convert(const FeatureVector &f) {
	    return Core::Ref<const Vector>(new Vector(f));
	}

    private:
	std::vector<Core::Ref<const Vector> > streams_;

    public:
	/** Creates an empty feature with @param nStreams streams */
	Feature(size_t nStreams = 0) : streams_(nStreams) { }

	/** Creates a feature with a single stream containing the feature vector @param f. */
	explicit Feature(const Core::Ref<const Vector> &f) : streams_(1) { set(0, f); }
	/** Creates a feature with a single stream containing the feature vector @param f. */
	explicit Feature(const FeatureVector &f) : streams_(1) { set(0, convert(f)); }

	/** Creates a new stream with @param v
	 *  @return is the new stream index.
	 */
	size_t add(const Core::Ref<const Vector> &v) {
	    streams_.push_back(v); return streams_.size() - 1;
	}

	/** Creates a new stream at index @param newIndex with @param v */
	void add(size_t newIndex, const Core::Ref<const Vector> &v);

	/** Changes value of the stream at index @param newIndex to @param v */
	void set(size_t streamIndex, const Core::Ref<const Vector> &v) {
	    require_(streamIndex < nStreams()); streams_[streamIndex] = v;
	}

	/** Changes value of the stream at indices @param indices to @param v */
	void set(const std::vector<size_t> &indices, const Core::Ref<const Vector> &v) {
	    for(std::vector<size_t>::const_iterator i = indices.begin(); i != indices.end(); ++ i)
		set(*i, v);
	}


	void clear() { streams_.clear(); }

	/** @return is true if all streams are identical */
	bool operator==(const Feature &r) const {
	    for (size_t streamIndex = 0; streamIndex < nStreams(); ++ streamIndex)
		if (*(*this)[streamIndex] != *r[streamIndex]) return false;
	    return true;
	}

	/** @return is value of the stream @param streamIndex */
	Core::Ref<const Vector> operator[](size_t streamIndex) const {
	    return streams_[streamIndex];
	}

	/** Main stream is used in application working only with one stream.
	 *  For example: training, basic feature scorer, etc.
	 */
	Core::Ref<const Vector> mainStream() const {
	    return streams_.front();
	}

	Iterator begin() const { return streams_.begin(); }
	Iterator end() const { return streams_.end(); }

	void setNumberOfStreams(size_t nStreams) { return streams_.resize(nStreams); }
	size_t nStreams() const { return streams_.size(); }
    };

    /** Vector of stream descriptions
     * Stream descriptions if a set of attributes describing a feature stream.
     * For example: type, dimension, etc.
     */
    class FeatureDescription : public Core::Description {
	typedef Core::Description Precursor;
    public:
	static const std::string defaultName;
	static const std::string nameDimension;
	static const std::string namePortName;
    protected:
	/*
	 *  Initializes the FeatureDescription by parameters of an existing feature object.
	 */
	void initialize(const Feature &);
    public:
	/**
	 *  Initializes the FeatureDescription.
	 */
	FeatureDescription(const std::string &);
	/**
	 *  Initializes the FeatureDescription.
	 *  Name of the description is created form the full name of @c parent.
	 */
	FeatureDescription(const Core::Configurable &parent);
	/**
	 *  Initializes the FeatureDescription by parameters of an existing feature object.
	 */
	FeatureDescription(const std::string &, const Feature &);
	/**
	 *  Initializes the FeatureDescription by parameters of an existing feature object.
	 *  Name of the description is created form the full name of @c parent.
	 */
	FeatureDescription(const Core::Configurable &parent, const Feature &);
    };

} //namespace Mm

#endif // _MM_FEATURE_HH
