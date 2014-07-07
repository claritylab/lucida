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
#include "tSemiring.hh"
#include "tStorage.hh"

namespace Ftl {
    template<class _Automaton>
    StorageAutomaton<_Automaton>::StorageAutomaton(Fsa::Type type) :
	type_(type),
	initial_(Fsa::InvalidStateId)
    {
	_Automaton::setProperties(Fsa::PropertyStorage | Fsa::PropertyCached, Fsa::PropertyAll);
    }

    template<class _Automaton>
    void StorageAutomaton<_Automaton>::setType(Fsa::Type type) {
	type_ = type;
	if (type_ == Fsa::TypeAcceptor) output_ = input_;
    }
    template<class _Automaton>
    Fsa::Type StorageAutomaton<_Automaton>::type() const { return type_; }

    template<class _Automaton>
    void StorageAutomaton<_Automaton>::addProperties(Fsa::Property properties) const {
	properties &= ~(Fsa::PropertyStorage | Fsa::PropertyCached);
	_Automaton::addProperties(properties);
    }

    template<class _Automaton>
    void StorageAutomaton<_Automaton>::setProperties(Fsa::Property knownProperties, Fsa::Property properties) const {
	knownProperties &= ~(Fsa::PropertyStorage | Fsa::PropertyCached);
	properties      &= ~(Fsa::PropertyStorage | Fsa::PropertyCached);
	_Automaton::setProperties(knownProperties, properties);
    }

    template<class _Automaton>
    void StorageAutomaton<_Automaton>::unsetProperties(Fsa::Property unknownProperties) const {
	unknownProperties &= ~(Fsa::PropertyStorage | Fsa::PropertyCached);
	_Automaton::unsetProperties(unknownProperties);
    }

    template<class _Automaton>
    Core::Ref<const typename _Automaton::Semiring> StorageAutomaton<_Automaton>::semiring() const { return semiring_; }

    template<class _Automaton>
    Fsa::StateId StorageAutomaton<_Automaton>::initialStateId() const { return initial_; }

    template<class _Automaton>
    void StorageAutomaton<_Automaton>::setInputAlphabet(Fsa::ConstAlphabetRef a) {
	input_ = a;
	if (type_ == Fsa::TypeAcceptor) output_ = a;
    }

    template<class _Automaton>
    Fsa::ConstAlphabetRef StorageAutomaton<_Automaton>::getInputAlphabet() const {
	return input_;
    }

    template<class _Automaton>
    void StorageAutomaton<_Automaton>::setOutputAlphabet(Fsa::ConstAlphabetRef a) {
	require(type() == Fsa::TypeTransducer);
	output_ = a;
    }

    template<class _Automaton>
    Fsa::ConstAlphabetRef StorageAutomaton<_Automaton>::getOutputAlphabet() const {
	if (type() == Fsa::TypeAcceptor) return input_;
	return output_;
    }

    template<class _Automaton>
    void StorageAutomaton<_Automaton>::normalize() {}

    template<class _Automaton>
    typename _Automaton::State * StorageAutomaton<_Automaton>::createState(Fsa::StateId id, Fsa::StateTag tags) const {
	return new typename _Automaton::State(id, tags);
    }

    template<class _Automaton>
    typename _Automaton::State * StorageAutomaton<_Automaton>::createState(Fsa::StateId id, Fsa::StateTag tags, const typename _Automaton::Weight &weight) const {
	return new typename _Automaton::State(id, tags, weight);
    }

} // namespace Ftl
