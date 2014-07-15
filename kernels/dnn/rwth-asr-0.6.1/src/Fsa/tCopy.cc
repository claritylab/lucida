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
#include <Core/Tokenizer.hh>
#include "tCopy.hh"
#include "tDfs.hh"
#include "Alphabet.hh"
#include "Types.hh"

namespace Ftl {
	template<class _Automaton> class CopyDfsState : public DfsState<_Automaton> {
		typedef DfsState<_Automaton> Precursor;
	public:
		typedef typename _Automaton::State _State;
		typedef typename _Automaton::ConstStateRef _ConstStateRef;
		typedef typename _Automaton::ConstRef _ConstAutomatonRef;
	private:
		StorageAutomaton<_Automaton> *fsa_;
	public:
		CopyDfsState(StorageAutomaton<_Automaton> *f, _ConstAutomatonRef f2) :
			Precursor(f2), fsa_(f) {
		}
		virtual void discoverState(_ConstStateRef sp) {
			fsa_->setState(new _State(*sp));
		}
	};

	template<class _Automaton> void copy(StorageAutomaton<_Automaton> *f,
			typename _Automaton::ConstRef f2) {
		f->setType(f2->type());

		Fsa::Property prop = f2->knownProperties();
		f->setProperties(prop, f2->properties());
		f->unsetProperties(~prop);

		f->setSemiring(f2->semiring());
		f->setInitialStateId(f2->initialStateId());
		f->setInputAlphabet(f2->getInputAlphabet());
		if (f->type() == Fsa::TypeTransducer)
			f->setOutputAlphabet(f2->getOutputAlphabet());
		CopyDfsState<_Automaton> s(f, f2);
		s.dfs();
	}

	template<class _Automaton> void copy(StorageAutomaton<_Automaton> *f,
			const std::string &str) {
		typedef typename _Automaton::Arc _Arc;
		typedef typename _Automaton::State _State;

		f->clear();
		f->setType(Fsa::TypeAcceptor);

		f->addProperties(Fsa::PropertySorted);
		f->addProperties(Fsa::PropertyLinear | Fsa::PropertyAcyclic);

		Fsa::StateId s = 0;
		_State *sp = new _State(s++);
		f->setInitialStateId(sp->id());
		sp->weight_ = f->semiring()->one();

		Fsa::StaticAlphabet *sinput(0);
		Fsa::ConstAlphabetRef input;
		if (f->getInputAlphabet())
			input = f->getInputAlphabet();
		else {
			sinput = new Fsa::StaticAlphabet();
			f->setInputAlphabet(Fsa::ConstAlphabetRef(sinput));
		}

		Core::StringTokenizer tokenizer(str);
		for (Core::StringTokenizer::Iterator token = tokenizer.begin(); token
				!= tokenizer.end(); ++token) {
			std::string tmp = *token;
			if (!tmp.empty()) {
				_Arc *a = sp->newArc();
				a->input_ = a->output_ = (sinput ? sinput->addSymbol(tmp)
						: input->index(tmp));
				a->weight_ = f->semiring()->one();
				f->setState(sp);
				sp = new _State(s++);
				a->target_ = sp->id();
			}
		}
		sp->setTags(Fsa::StateTagFinal);
		sp->weight_ = f->semiring()->one();
		f->setState(sp);
		f->normalize();
	}

	template<class _Automaton> void copy(StorageAutomaton<_Automaton> *f,
			const Fsa::Hash<std::string, Core::StringHash> &tokens,
			u32 sausage) {
		typedef typename _Automaton::Arc _Arc;
		typedef typename _Automaton::State _State;

		f->setType(Fsa::TypeAcceptor);
		f->addProperties(Fsa::PropertySorted);
		f->addProperties(Fsa::PropertyLinear | Fsa::PropertyAcyclic);

		_State *sp = new _State(0);
		f->setInitialStateId(sp->id());
		sp->weight_ = f->semiring()->one();
		Fsa::StaticAlphabet *input = new Fsa::StaticAlphabet();
		f->setInputAlphabet(Fsa::ConstAlphabetRef(input));

		if (sausage == 0) {
			for (Fsa::Hash<std::string, Core::StringHash>::const_iterator token =
					tokens.begin(); token != tokens.end(); ++token)
				sp->newArc(sp->id(), f->semiring()->one(), input->addSymbol(*token));
			sp->setTags(Fsa::StateTagFinal);
			sp->weight_ = f->semiring()->one();
			f->setState(sp);
		} else {
			for (u32 i = 0; i < sausage; ++i) {
				for (Fsa::Hash<std::string, Core::StringHash>::const_iterator
						token = tokens.begin(); token != tokens.end(); ++token)
					sp->newArc(i + 1, f->semiring()->one(), input->addSymbol(*token));
				f->setState(sp);
				sp = new _State(i + 1);
			}
		}

		sp->setTags(Fsa::StateTagFinal);
		sp->weight_ = f->semiring()->one();
		f->setState(sp);
		f->normalize();
	}

	template<class _Automaton> Core::Ref<StaticAutomaton<_Automaton> > staticCopy(
			typename _Automaton::ConstRef f) {
		StaticAutomaton<_Automaton> *result = new StaticAutomaton<_Automaton>;
		copy(result, f);
		return Core::ref(result);
	}

	template<class _Automaton> Core::Ref<StaticAutomaton<_Automaton> > staticCompactCopy(
			typename _Automaton::ConstRef f) {
		StaticAutomaton<_Automaton> *result = new StaticAutomaton<_Automaton>;
		copy(result, f);
		Fsa::StateMap stateMap;
		result->compact(stateMap);
		return Core::ref(result);
	}

	template<class _Automaton> Core::Ref<StaticAutomaton<_Automaton> > staticCopy(
			const std::string &str,
			typename _Automaton::ConstSemiringRef semiring) {
		StaticAutomaton<_Automaton> *result = new StaticAutomaton<_Automaton>;
		result->setSemiring(semiring);
		copy(result, str);
		return Core::ref(result);
	}

	template<class _Automaton> Core::Ref<StaticAutomaton<_Automaton> > staticCopy(
			const Fsa::Hash<std::string, Core::StringHash> strings,
			typename _Automaton::ConstSemiringRef semiring,
			u32 sausage = 0) {
		StaticAutomaton<_Automaton> *result = new StaticAutomaton<_Automaton>;
		result->setSemiring(semiring);
		copy(result, strings, sausage);
		return Core::ref(result);
	}
} // namespace Ftl
