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
#include <Core/Assertions.hh>
#include <Core/CompressedStream.hh>
#include <Core/StringUtilities.hh>

#include "tCopy.hh"
#include "tInput.hh"
#include "tStatic.hh"
#include "tStorageXml.hh"
#include "Alphabet.hh"
#include "Utility.hh"
#include <Core/Vector.hh>

namespace Ftl {
    template<class _Automaton>
    typename _Automaton::ConstRef read(const Resources<_Automaton> &resources, const std::string &format, std::istream &i) {
	StorageAutomaton<_Automaton> *f = new StaticAutomaton<_Automaton>();
	verify(!f->getInputAlphabet() && ((f->type() == Fsa::TypeAcceptor) || !f->getOutputAlphabet()));
	if (read<_Automaton>(resources, f, format, i)) {
	    return typename _Automaton::ConstRef(f);
	} else {
	    delete f;
	    return typename _Automaton::ConstRef();
	}
    }

    template<class _Automaton>
    bool read(const Resources<_Automaton> &resources, StorageAutomaton<_Automaton> *f, const std::string &format, std::istream &i) {
	typename Resources<_Automaton>::Format *formatter = (format.empty()) ?
	    resources.getDefaultFormat() :
	    resources.getFormat(format);
	if (!formatter) {
	    std::cerr << "no format with name \"" << format << "\" found" << std::endl;
	    return false;
	}
	if (!formatter->reader) {
	    std::cerr << "format \"" << formatter->name << "\" does not support reading" << std::endl;
	    return false;
	}
	return formatter->reader(resources, f, i);
    }

    template<class _Automaton>
    typename _Automaton::ConstRef read(const Resources<_Automaton> &resources, const std::string &file) {
	StorageAutomaton<_Automaton> *f = new StaticAutomaton<_Automaton>();
	if (read<_Automaton>(resources, f, file))
	    return typename _Automaton::ConstRef(f);
	else {
	    delete f;
	    return typename _Automaton::ConstRef();
	}
    }

    namespace {
	template<class _Automaton>
	bool readSingleFile(const Resources<_Automaton> &resources, StorageAutomaton<_Automaton> *f, const std::string &format, const std::string &file) {
	    if (format == "combine") {
		std::cerr << "nested combine is not allowed" << std::endl;
		return false;
	    }
	    if (file.empty()) {
		std::cerr << "missing filename" << std::endl;
		return false;
	    }
	    Core::CompressedInputStream i(file);
	    if (!read<_Automaton>(resources, f, format, i)) {
		std::cerr << "could not read \"" << file << "\"" << std::endl;
		return false;
	    }
	    return true;
	}
    } // namespace
    template<class _Automaton>
    bool read(const Resources<_Automaton> &resources, StorageAutomaton<_Automaton> *f, const std::string &file) {
	Fsa::QualifiedFilename qf = Fsa::splitQualifiedFilename(file);
	if (qf.first == "combine") {
	    std::string tmp = qf.second;
	    std::string::size_type i;
	    while ((i = tmp.find("+")) != std::string::npos) {
		qf = Fsa::splitQualifiedFilename(tmp.substr(0, i));
		if (!readSingleFile<_Automaton>(resources, f, qf.first, qf.second))
		    return false;
		tmp = tmp.substr(i + 1);
	    }
	    qf = Fsa::splitQualifiedFilename(tmp);
	}
	return readSingleFile<_Automaton>(resources, f, qf.first, qf.second);
    }

    template<class _Automaton>
    void assertSemiring(const Resources<_Automaton> &resources, StorageAutomaton<_Automaton> *f) {
	if (!(f->semiring()))
	    f->setSemiring(resources.getDefaultSemiring());
	verify(f->semiring());
    }

    template<class _Automaton>
    bool readAtt(const Resources<_Automaton> &resources, StorageAutomaton<_Automaton> *f, std::istream &i) {
	typedef typename _Automaton::State _State;
	typedef typename _Automaton::Arc _Arc;
	assertSemiring<_Automaton>(resources, f);
	f->clear();
	if (!i) return false;
	f->setType(Fsa::TypeAcceptor);

	bool hasAlphabets = false;
	Fsa::StaticAlphabet *sinput = 0, *soutput = 0;
	Fsa::ConstAlphabetRef input, output;
	if (f->getInputAlphabet()) {
	    hasAlphabets = true;
	    input = f->getInputAlphabet();
	    if (f->type() == Fsa::TypeTransducer) {
		verify(f->getOutputAlphabet());
		output = f->getOutputAlphabet();
	    }
	}

	Core::Vector<_State*> states;
	while (i) {
	    std::string line;
	    Core::getline(i, line);
	    char tmp1[100], tmp2[100], tmp3[100], tmp4[100], tmp5[100];
	    int n = sscanf(line.c_str(), "%s %s %s %s %s", tmp1, tmp2, tmp3, tmp4, tmp5);
	    char *error;
	    int s = strtol(tmp1, &error, 10);
	    states.grow(s, 0);
	    if (!states[s]) {
		states[s] = f->createState(s);
		if (s == 0) f->setInitialStateId(0);
	    }
	    _State *sp = states[s];
	    _Arc *a = 0;
	    if (n > 2) {
		a = sp->newArc();
		a->setOutput(0);
		a->setWeight(f->semiring()->defaultWeight());
	    }
	    switch (n) {
	    case 1: // final state
		sp->setTags(Fsa::StateTagFinal);
		sp->setWeight(f->semiring()->defaultWeight());
		break;
	    case 2: // final state with weight
		sp->setTags(Fsa::StateTagFinal);
		sp->setWeight(f->semiring()->fromString(tmp2));
		break;
	    case 3: // acceptor arc
		a->setTarget(strtol(tmp2, &error, 10));
		a->setInput(strtol(tmp3, &error, 10) - 1);
		a->setOutput(a->input());
		if (*error != '\0') {
		    if (!hasAlphabets && !sinput) sinput = new Fsa::StaticAlphabet();
		    if (strcmp(tmp3, "eps") == 0) a->setInput(Fsa::Epsilon);
		    else a->setInput(hasAlphabets ? input->index(tmp3) : sinput->addSymbol(tmp3));
		    if (!hasAlphabets && !soutput) soutput = new Fsa::StaticAlphabet();
		    if (strcmp(tmp4, "eps") == 0) a->setOutput(Fsa::Epsilon);
		    else a->setOutput(hasAlphabets ? output->index(tmp3) : soutput->addSymbol(tmp3));
		}
		break;
	    case 4: // acceptor arc with weight or transducer arc
		a->setTarget(strtol(tmp2, &error, 10));
		a->setInput(strtol(tmp3, &error, 10) - 1);
		if (*error != '\0') {
		    if (!hasAlphabets && !sinput) sinput = new Fsa::StaticAlphabet();
		    a->setInput(hasAlphabets ? input->index(tmp3) : sinput->addSymbol(tmp3));
		}
		a->setOutput(strtol(tmp4, &error, 10) - 1);
		if (*error != '\0') {
		    a->weight_ = f->semiring()->fromString(tmp4);
		    if (*error != '\0') {
			f->setType(Fsa::TypeTransducer);
			if (!hasAlphabets && !soutput) soutput = new Fsa::StaticAlphabet();
			if (strcmp(tmp4, "eps") == 0) a->setOutput(Fsa::Epsilon);
			else a->setOutput(hasAlphabets ? output->index(tmp4) : soutput->addSymbol(tmp4));
		    }
		} else f->setType(Fsa::TypeTransducer);
		break;
	    case 5: // transducer arc with weight
		f->setType(Fsa::TypeTransducer);
		a->setTarget(strtol(tmp2, &error, 10));
		a->setInput(strtol(tmp3, &error, 10) - 1);
		if (*error != '\0') {
		    if (!hasAlphabets && !sinput) sinput = new Fsa::StaticAlphabet();
		    if (strcmp(tmp3, "eps") == 0) a->setInput(Fsa::Epsilon);
		    else a->setInput(hasAlphabets ? input->index(tmp3) : sinput->addSymbol(tmp3));
		}
		a->setOutput(strtol(tmp4, &error, 10) - 1);
		if (*error != '\0') {
		    if (!hasAlphabets && !soutput) soutput = new Fsa::StaticAlphabet();
		    if (strcmp(tmp4, "eps") == 0) a->setOutput(Fsa::Epsilon);
		    else a->setOutput(hasAlphabets ? output->index(tmp4) : soutput->addSymbol(tmp4));
		}
		a->setWeight(f->semiring()->fromString(tmp5));
		break;
	    }
	}
	if (sinput) f->setInputAlphabet(Fsa::ConstAlphabetRef(sinput));
	if (soutput) f->setOutputAlphabet(Fsa::ConstAlphabetRef(soutput));
	for (typename Core::Vector<_State*>::iterator i = states.begin(); i != states.end(); ++i)
	    f->setState(*i);
	return true;
    }

    template<class _Automaton>
    bool readBinary(const Resources<_Automaton> &resources, StorageAutomaton<_Automaton> *f, std::istream &i) {
	typedef typename _Automaton::State _State;
	typedef typename _Automaton::Arc _Arc;
	typedef typename _Automaton::ConstSemiringRef _ConstSemiringRef;

	assertSemiring<_Automaton>(resources, f);
	if (!i) return false;

	Core::BinaryInputStream bi(i);
	char magic[8];
	bi.read(magic, 8);
	if (strcmp(magic, "RWTHFSA") != 0) return false;
	u32 what, tmp;
	if (!(bi >> what)) return false;
	u32 formatVersion = (what & 0xff000000) >> 24;
	if (formatVersion == 0) {
	    tmp = what;
	    what = Fsa::storeStates | Fsa::storeInputAlphabet | Fsa::storeOutputAlphabet;
	} else if (formatVersion >= 1) {
	    if (!(bi >> tmp)) return false;
	} else return false;
	f->setType(Fsa::Type(tmp));

	if (what & Fsa::storeStates) {
	    if (!(bi >> tmp)) return false;
	    tmp = tmp & ~(Fsa::PropertyCached | Fsa::PropertyStorage);
	    if (formatVersion >= 2) { // read known field first
		u32 knownProperties;
		if (!(bi >> knownProperties)) return false;
		knownProperties |= Fsa::PropertyCached | Fsa::PropertyStorage;
		f->setProperties(knownProperties, tmp);
	    } else f->addProperties(tmp);
	    if (!(bi >> tmp)) return false;
	    _ConstSemiringRef semiring =
		resources.getSemiring(Fsa::SemiringType(tmp));
	    if (semiring)
		f->setSemiring(semiring);
	    else
		std::cerr << "semiring \"" << tmp << "\" not found. "
			  << "use " << f->semiring()->name() << " semiring" << std::endl;
	}
	if ((what & Fsa::storeInputAlphabet) && !f->getInputAlphabet()) {
	    Fsa::StaticAlphabet *a = new Fsa::StaticAlphabet();
	    if (!a->read(bi)) {
		return false;
	    }
	    f->setInputAlphabet(Fsa::ConstAlphabetRef(a));
	}

	if ((what & Fsa::storeOutputAlphabet) && (f->type() == Fsa::TypeTransducer) && !f->getOutputAlphabet()) {
	    Fsa::StaticAlphabet *a = new Fsa::StaticAlphabet();
	    if (!a->read(bi)) return false;
	    if (f->type() == Fsa::TypeTransducer)
		f->setOutputAlphabet(Fsa::ConstAlphabetRef(a));
	}

	if (what & Fsa::storeStates) {
	    f->clear();
	    if (!(bi >> tmp)) return false;
	    f->setInitialStateId(tmp);
	    Fsa::StateId idAndTags;
	    if (!(bi >> idAndTags)) return false;
	    while (bi) {
		_State *sp = f->createState(idAndTags & Fsa::StateIdMask, idAndTags & Fsa::StateTagMask);
		if (idAndTags & Fsa::StateTagFinal)
		    if (!f->semiring()->read(sp->weight_, bi)) return false;
		u32 narcs;
		if (!(bi >> narcs)) return false;
		for (u32 cnt = 0; cnt < narcs; ++cnt) {
		    _Arc *a = sp->newArc();
		    if (!(bi >> a->target_)) return false;
		    if (!f->semiring()->read(a->weight_, bi)) return false;
		    if (!(bi >> a->input_)) return false;
		    if (f->type() == Fsa::TypeTransducer) {
			if (!(bi >> a->output_)) return false;
		    } else if (f->type() == Fsa::TypeAcceptor) {
			a->output_ = a->input_;
		    }
		}
		sp->minimize();
		f->setState(sp);
		bi >> idAndTags;
	    }
	}
	return true;
    }

    template<class _Automaton>
    bool readLinear(const Resources<_Automaton> &resources, StorageAutomaton<_Automaton> *f, std::istream &i) {
	assertSemiring<_Automaton>(resources, f);
	f->clear();
	if (!i) return false;

	std::string line;
	Core::getline(i, line);
	copy<_Automaton>(f, line);
	return true;
    }

    template<class _Automaton>
    bool readXml(const Resources<_Automaton> &resources, StorageAutomaton<_Automaton> *f, std::istream &i) {
	assertSemiring<_Automaton>(resources, f);
	if (!i) return false;

	StorageAutomatonXmlParser<_Automaton> parser(resources, f);
	if (!parser.parseStream(i)) return false;
	return true;
    }

} // namespace Ftl
