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
#ifndef _SPEECH_FSA_CACHE_HH
#define _SPEECH_FSA_CACHE_HH

#include <Core/Component.hh>
#include <Fsa/Output.hh>

namespace Core {
    class Archive;
    class DependencySet;
};

namespace Speech {

    /**
     *  Cache for Fsa object.
     *
     *  Cache stores FSA objects in a archive.
     *  Use 'find', to get an existing object.
     *  Use 'insert' to add a new object.
     *  Use 'get', to retrieve object which will be created if not existent.
     *
     *  Remark: This is a simple implementation, once Core::ObjectCache supports
     *  reading of Core::Ref, this implementation could be easly merged with it.
     */
    class FsaCache : public Core::Component {
    private:
	static const Core::ParameterString paramPath;
	static const Core::ParameterBool paramReadOnly;
    private:
	Core::Archive *archive_;
	/** If valid, input alphabet of new transducers is overwritten by this one. */
	Fsa::ConstAlphabetRef inputAlphabet_;
	/** If valid, output alphabet of new transducers is overwritten by this one. */
	Fsa::ConstAlphabetRef outputAlphabet_;
	Fsa::StoredComponents whatToStore_;
	bool readOnly_;
    private:
	Fsa::ConstAutomatonRef read(const std::string &id);
	void write(const std::string &id, Fsa::ConstAutomatonRef);
    public:
	FsaCache(const Core::Configuration &, Fsa::StoredComponents = Fsa::storeAll);
	virtual ~FsaCache();

	bool setDependencies(const Core::DependencySet &);

	/** Input alphabet of read transducer will be overwritten by @param input. */
	void forceInputAlphabet(Fsa::ConstAlphabetRef input) { inputAlphabet_ = input; }
	/** Output alphabet of read transducer will be overwritten by @param output. */
	void forceOutputAlphabet(Fsa::ConstAlphabetRef output) { outputAlphabet_ = output; }

	Fsa::ConstAutomatonRef find(const std::string &id) {
	    return archive_ ? read(id) : Fsa::ConstAutomatonRef();
	}
	void insert(const std::string &id, Fsa::ConstAutomatonRef f) {
	    if (!readOnly_ && archive_) write(id, f);
	}

	template<class Builder> Fsa::ConstAutomatonRef get(Builder);
    };

    template<class Builder>
    Fsa::ConstAutomatonRef FsaCache::get(Builder builder)
    {
	Fsa::ConstAutomatonRef result;
	const std::string &id(builder.id());
	if (!(result = find(id))) {
	    log("Building transducer for \"%s\".", id.c_str());
	    insert(id, result = builder.build());
	}
	return result;
    }
} // namespace Speech

#endif // _SPEECH_FSA_CACHE_HH
