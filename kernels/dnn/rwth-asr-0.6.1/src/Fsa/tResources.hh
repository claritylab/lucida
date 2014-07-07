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
#ifndef _T_FSA_RESOURCES_HH
#define _T_FSA_RESOURCES_HH

#include <iostream>
#include <string>

#include <Core/Assertions.hh>
#include <Core/Configuration.hh>
#include <Core/Hash.hh>
#include <Core/Types.hh>
#include "tStorage.hh"

namespace Ftl {
    template<class _Automaton>
    class Resources {
	typedef Resources<_Automaton> Self;
    public:
	typedef _Automaton Automaton;
	typedef typename _Automaton::ConstRef _ConstAutomatonRef;
	typedef typename _Automaton::ConstSemiringRef _ConstSemiringRef;
	typedef StorageAutomaton<_Automaton> _StorageAutomaton;

	typedef bool (*Writer)(const Self &, _ConstAutomatonRef, std::ostream &, Fsa::StoredComponents, bool progress);
	typedef bool (*Reader)(const Self &, _StorageAutomaton*, std::istream &);
	struct Format {
	    const std::string name;
	    std::string desc;
	    Reader reader;
	    Writer writer;
	    Format(const std::string &name, const std::string &desc) :
		name(name), desc(desc), reader(0), writer(0) {}
	    Format(const std::string &name, const std::string &desc, Reader reader, Writer writer) :
		name(name), desc(desc), reader(reader), writer(writer) {}
	};

    private:
	typedef Core::hash_map<std::string, _ConstSemiringRef, Core::StringHash> SemiringMap;
	typedef Core::hash_map<std::string, u32, Core::StringHash> NameToTypeMap;;
	typedef Core::hash_map<u32, std::string>                   TypeToNameMap;

	typedef Core::hash_map<std::string, Format*, Core::StringHash> FormatMap;

    private:
	const Core::Configuration *config_;
	SemiringMap semirings_;
	_ConstSemiringRef defaultSemiring_;
	NameToTypeMap nameToType_;
	TypeToNameMap typeToName_;

	FormatMap formats_;
	Format *defaultFormat_;
    public:
	Resources() : config_(new Core::Configuration()) { defaultFormat_ = 0; }
	Resources(const Core::Configuration &config) : config_(&config) { defaultFormat_ = 0; }

	const Core::Configuration & getConfiguration() const { return *config_; }

	bool registerSemiring(const std::string &name, _ConstSemiringRef semiring, bool isDefault = false) {
	    if (isDefault) defaultSemiring_ = semiring;
	    return semirings_.insert(std::make_pair(name, semiring)).second;
	}
	bool registerSemiring(_ConstSemiringRef semiring, bool isDefault = false) {
	    return registerSemiring(semiring->name(), semiring, isDefault);
	}
	_ConstSemiringRef getSemiring(const std::string &name) const {
	    typename SemiringMap::const_iterator it = semirings_.find(name);
	    if (it == semirings_.end()) return _ConstSemiringRef();
	    else return it->second;
	}
	_ConstSemiringRef getDefaultSemiring() const {
	    return defaultSemiring_;
	}
	/**
	 * \note for backward compatibility only
	 */
	bool registerSemiring(Fsa::SemiringType type, _ConstSemiringRef semiring, bool isDefault = false) {
	    verify(type != Fsa::SemiringTypeUnknown);
	    typeToName_.insert(std::make_pair(u32(type), semiring->name())).second;
	    nameToType_.insert(std::make_pair(semiring->name(), u32(type))).second;
	    return registerSemiring(semiring, isDefault);
	}
	_ConstSemiringRef getSemiring(Fsa::SemiringType type) const {
	    TypeToNameMap::const_iterator it = typeToName_.find(u32(type));
	    if (it == typeToName_.end()) return _ConstSemiringRef();
	    else return getSemiring(it->second);
	}
	Fsa::SemiringType getSemiringType(_ConstSemiringRef semiring) const {
	    NameToTypeMap::const_iterator it = nameToType_.find(semiring->name());
	    if (it == nameToType_.end()) return Fsa::SemiringTypeUnknown;
	    else return Fsa::SemiringType(it->second);
	}


	bool registerFormat(const std::string &name, Format *format, bool isDefault = false) {
	    if (isDefault) defaultFormat_ = format;
	    return formats_.insert(std::make_pair(name, format)).second;
	}
	bool registerFormat(Format *format, bool isDefault = false) {
	    return registerFormat(format->name, format, isDefault);
	}
	Format* getFormat(const std::string &name) const {
	    typename FormatMap::const_iterator it = formats_.find(name);
	    if (it == formats_.end()) return 0;
	    else return it->second;
	}
	Format* getDefaultFormat() const {
	    return defaultFormat_;
	}

	void dump(std::ostream &o) const;
    };
} // namespace Ftl

#include "tResources.cc"

#endif // _T_FSA_RESOURCES_HH
