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
#include <set>

#include <Core/Assertions.hh>
#include <Core/BinaryStream.hh>
#include <Core/CompressedStream.hh>
#include <Core/ProgressIndicator.hh>
#include <Core/Unicode.hh>
#include <Core/Utility.hh>
#include <Core/XmlStream.hh>

#include "tDfs.hh"
#include "tLinear.hh"
#include "tOutput.hh"
#include "tProperties.hh"
#include "Alphabet.hh"
#include "Types.hh"
#include "Utility.hh"

namespace Ftl {

    template<class _Automaton>
    bool write(const Resources<_Automaton> &resources, typename _Automaton::ConstRef f,
	       const std::string &format, std::ostream &o, Fsa::StoredComponents what, bool progress) {
	typename Resources<_Automaton>::Format *formatter = (format.empty()) ?
	    resources.getDefaultFormat() :
	    resources.getFormat(format);
	if (!formatter) {
	    std::cerr << "no format with name \"" << format << "\" found" << std::endl;
	    return false;
	}
	if (!formatter->writer) {
	    std::cerr << "format \"" << formatter->name << "\" does not support writing" << std::endl;
	    return false;
	}
	return formatter->writer(resources, f, o, what, progress);
    }

    template<class _Automaton>
    bool write(const Resources<_Automaton> &resources, typename _Automaton::ConstRef f, const std::string &file, Fsa::StoredComponents what, bool progress) {
	Fsa::QualifiedFilename qf = Fsa::splitQualifiedFilename(file);
	Core::CompressedOutputStream o(qf.second);
	return write<_Automaton>(resources, f, qf.first, o, what, progress);
    }

    /**
     * Transformations done in order to support AT&T's ascii based format:
     * - with the RWTH FSA label ids can be in the range [0..InvalidLabelId), they are transformed to
     *   [1..InvalidLabelId] and 0 is being used for Epsilon, other special labels are not supported
     *   in AT&T's format
     **/
    template<class _Automaton>
    class WriteAttDfsState : public DfsState<_Automaton> {
    private:
	typedef DfsState<_Automaton> Precursor;
    public:
	typedef typename _Automaton::State _State;
	typedef typename _Automaton::ConstStateRef _ConstStateRef;
	typedef typename _Automaton::ConstRef _ConstAutomatonRef;
    private:
	std::ostream &o_;
	Fsa::ConstAlphabetRef input_, output_;
    public:
	WriteAttDfsState(_ConstAutomatonRef f, std::ostream &o) :
	    Precursor(f), o_(o), input_(f->getInputAlphabet()), output_(f->getOutputAlphabet()) {}
	void discoverState(_ConstStateRef sp) {
	    if (sp->isFinal()) {
		o_ << sp->id();
		if (!Precursor::fsa_->semiring()->isDefault(sp->weight_))
		    o_ << " " << Precursor::fsa_->semiring()->asString(sp->weight_);
		o_ << std::endl;
	    }
	    for (typename _State::const_iterator a = sp->begin(); a != sp->end(); ++a) {
		o_ << sp->id() << " " << a->target();
		if (a->input() != Fsa::Epsilon) {
		    if (input_) o_ << " " << input_->symbol(a->input());
		    else o_ << " " << a->input() + 1;
		} else {
		    if (input_) o_ << " eps";
		    else o_ << " 0";
		}
		if (Precursor::fsa_->type() == Fsa::TypeTransducer) {
		    if (a->output() != Fsa::Epsilon) {
			if (output_) o_ << " " << output_->symbol(a->output());
			else o_ << " " << a->output() + 1;
		    } else {
			if (output_) o_ << " eps";
			else o_ << " 0";
		    }
		}
		if (!Precursor::fsa_->semiring()->isDefault(a->weight())) {
		    std::string weightStr = Precursor::fsa_->semiring()->asString(a->weight());
		    // weight representation must not contain whitespaces
		    verify(weightStr.find_first_of(utf8::whitespace) == std::string::npos);
		    o_ << " " << weightStr;
		}
		o_ << std::endl;
	    }
	}
    };

    template<class _Automaton>
    bool writeAtt(const Resources<_Automaton> &resources, typename _Automaton::ConstRef f,
		  std::ostream &o, Fsa::StoredComponents what, bool progress) {
	/*
	if (!f->semiring()->hasProperty(Fsa::PropertyStringIo)) {
	    std::cerr << "semiring " << f->semiring()->name() << " does not support string i/o" << std::endl;
	    return false;
	}
	*/
	if (o) {
	    // input alphabet
	    // output alphabet
	    Core::ProgressIndicator *p = 0;
	    if (progress) p = new Core::ProgressIndicator("writing", "states");
	    WriteAttDfsState<_Automaton> s(f, o);
	    s.dfs(p);
	    delete p;
	}
	return true;
    }

    template<class _Automaton>
    bool writeAtt(const Resources<_Automaton> &resources, typename _Automaton::ConstRef f,
		  const std::string &file, Fsa::StoredComponents what, bool progress) {
	Core::CompressedOutputStream o(file);
	return writeAtt<_Automaton>(resources, f, o, what, progress);
    }


    template<class _Automaton>
    class WriteBinaryDfsState : public DfsState<_Automaton> {
    private:
	typedef DfsState<_Automaton> Precursor;
    public:
	typedef typename _Automaton::State _State;
	typedef typename _Automaton::ConstStateRef _ConstStateRef;
	typedef typename _Automaton::ConstRef _ConstAutomatonRef;
    private:
	Core::BinaryOutputStream &o_;
    public:
	WriteBinaryDfsState(_ConstAutomatonRef f, Core::BinaryOutputStream &o) : Precursor(f), o_(o) {}
	void discoverState(_ConstStateRef sp) {
	    Fsa::StateId idAndTags = sp->id() | sp->tags();
	    if (!(o_ << idAndTags)) return;
	    if (sp->isFinal()) Precursor::fsa_->semiring()->write(sp->weight_, o_);
	    if (!(o_ << sp->nArcs())) return;
	    for (typename _State::const_iterator a = sp->begin(); a != sp->end(); ++a) {
		if (!(o_ << a->target())) return;
		if (!Precursor::fsa_->semiring()->write(a->weight(), o_)) return;
		if (!(o_ << a->input())) return;
		if (Precursor::fsa_->type() == Fsa::TypeTransducer)
		    if (!(o_ << a->output())) return;
	    }
	}
    };

    template<class _Automaton>
    bool writeBinary(const Resources<_Automaton> &resources, typename _Automaton::ConstRef f,
		     std::ostream &o, Fsa::StoredComponents what, bool progress) {
	/*
	if (!f->semiring()->hasProperty(Fsa::PropertyBinaryIo)) {
	    std::cerr << "semiring " << f->semiring()->name() << " does not support binary i/o" << std::endl;
	    return false;
	}
	*/
	if (!o) return false;

	Core::BinaryOutputStream bo(o);

	static const char *magic = "RWTHFSA";
	static const u32 formatVersion = 2;
	if (!bo.write(magic, 8)) return false;

	Fsa::StoredComponents canStore = Fsa::storeStates;
	if (f->getInputAlphabet())
	    canStore |= Fsa::storeInputAlphabet;
	if ((f->type() == Fsa::TypeTransducer) && f->getOutputAlphabet())
	    canStore |= Fsa::storeOutputAlphabet;
	what &= canStore;

	bo << u32(what | (formatVersion << 24)) << u32(f->type());

	if (what & Fsa::storeStates)
	    bo << u32(f->properties())
	       << u32(f->knownProperties())
	       << u32(resources.getSemiringType(f->semiring()));

	if (!bo) return false;
	if (what & Fsa::storeInputAlphabet) f->getInputAlphabet()->write(bo);
	if (what & Fsa::storeOutputAlphabet) f->getOutputAlphabet()->write(bo);
	if (what & Fsa::storeStates) {
	    u32 tmp = f->initialStateId();
	    bo << tmp;
	    Core::ProgressIndicator *p = 0;
	    if (progress) p = new Core::ProgressIndicator("writing", "states");
	    WriteBinaryDfsState<_Automaton> s(f, bo);
	    s.dfs(p);
	    delete p;
	}

	return true;
    }

    template<class _Automaton>
    bool writeBinary(const Resources<_Automaton> &resources, typename _Automaton::ConstRef f,
		     const std::string &file, Fsa::StoredComponents what, bool progress) {
	Core::CompressedOutputStream o(file);
	return writeBinary<_Automaton>(resources, f, o, what, progress);
    }


    template<class _Automaton>
    class WriteLinearDfsState : public DfsState<_Automaton> {
    private:
	typedef DfsState<_Automaton> Precursor;
    public:
	typedef typename _Automaton::State _State;
	typedef typename _Automaton::ConstStateRef _ConstStateRef;
	typedef typename _Automaton::ConstRef _ConstAutomatonRef;
    private:
	bool first_;
	std::ostream &o_;
	Fsa::ConstAlphabetRef input_, output_;
	const bool printAll_;
    public:
	WriteLinearDfsState(_ConstAutomatonRef f, std::ostream &o, bool printAll=false) :
	    Precursor(f), first_(true), o_(o), input_(Precursor::fsa_->getInputAlphabet()), output_(Precursor::fsa_->getOutputAlphabet()), printAll_(printAll) {}
	void discoverState(_ConstStateRef sp) {
	    for (typename _State::const_iterator a = sp->begin(); a != sp->end(); ++a) {
		std::string out;
		if (printAll_ || ((a->input() != Fsa::Epsilon) && (a->input() <= Fsa::LastLabelId)))
		    out = input_ ? input_->symbol(a->input()) : Core::itoa(a->input());
		if (printAll_ || ((Precursor::fsa_->type() == Fsa::TypeTransducer) && (a->output() != Fsa::Epsilon)))
		    out += ":" + (output_ ? output_->symbol(a->output()) : Core::itoa(a->output()));
		if (!out.empty()) {
		    if (first_) first_ = false;
		    else o_ << " ";
		    o_ << out;
		}
	    }
	}
    };

    template<class _Automaton>
    bool writeLinear(const Resources<_Automaton> &resources, typename _Automaton::ConstRef f,
		     std::ostream &o, Fsa::StoredComponents what, bool progress, bool printAll) {
	if (!hasProperties<_Automaton>(f, Fsa::PropertyLinear)) {
	    std::cerr << "fsa is not linear" << std::endl;
	    return false;
	}
	if (o) {
	    Core::ProgressIndicator *p = 0;
	    if (progress) p = new Core::ProgressIndicator("writing", "states");
	    WriteLinearDfsState<_Automaton> s(f, o, printAll);
	    s.dfs(p);
	    delete p;
	}
	o << std::endl;
	return true;
    }

    template<class _Automaton>
    bool writeLinear(const Resources<_Automaton> &resources, typename _Automaton::ConstRef f,
		std::ostream &o, Fsa::StoredComponents what, bool progress) {

	return writeLinear(resources, f, o, what, progress, false);
    }

    template<class _Automaton>
    bool writeLinear(const Resources<_Automaton> &resources, typename _Automaton::ConstRef f,
		     const std::string &file, Fsa::StoredComponents what, bool progress) {
	if (!hasProperties<_Automaton>(f, Fsa::PropertyLinear)) {
	    std::cerr << "fsa '" << file << "' is not linear" << std::endl;
	    return false;
	}
	Core::CompressedOutputStream o(file);
	bool success = writeLinear<_Automaton>(resources, f, o, what, progress);
	o << std::endl;
	return success;
    }


    template<class _Automaton>
    class WriteXmlDfsState : public DfsState<_Automaton> {
    private:
	typedef DfsState<_Automaton> Precursor;
    public:
	typedef typename _Automaton::State _State;
	typedef typename _Automaton::ConstStateRef _ConstStateRef;
	typedef typename _Automaton::ConstRef _ConstAutomatonRef;
    private:
	Core::XmlWriter &o_;
    public:
	WriteXmlDfsState(_ConstAutomatonRef f, Core::XmlWriter &o) : Precursor(f), o_(o) {}
	void discoverState(_ConstStateRef sp) {
	    o_ << Core::XmlOpen("state") + Core::XmlAttribute("id", sp->id());
	    if (sp->isFinal()) o_ << Core::XmlEmpty("final");
	    if (sp->isFinal())
		if (!Precursor::fsa_->semiring()->isDefault(sp->weight_))
		    o_ << Core::XmlFull("weight", Precursor::fsa_->semiring()->asString(sp->weight_));
	    o_ << "\n";
	    for (typename _State::const_iterator a = sp->begin(); a != sp->end(); ++a) {
		o_ << Core::XmlOpen("arc") + Core::XmlAttribute("target", a->target());
		if (a->input() != Fsa::Epsilon) {
		    std::string tmp = Precursor::fsa_->getInputAlphabet()->specialSymbol(a->input());
		    if (tmp.empty()) o_ << Core::XmlFull("in", a->input());
		    else o_ << Core::XmlFull("in", tmp);
		}
		if (Precursor::fsa_->type() == Fsa::TypeTransducer)
		    if (a->output() != Fsa::Epsilon) {
			std::string tmp = Precursor::fsa_->getOutputAlphabet()->specialSymbol(a->output());
			if (tmp.empty()) o_ << Core::XmlFull("out", a->output());
			else o_ << Core::XmlFull("out", tmp);
		    }
		if (!Precursor::fsa_->semiring()->isDefault(a->weight()))
		    o_ << Core::XmlFull("weight", Precursor::fsa_->semiring()->asString(a->weight()));
		o_ << Core::XmlClose("arc") << "\n";
	    }
	    o_ << Core::XmlClose("state") << "\n";
	}
    };

    template<class _Automaton>
    bool writeXml(const Resources<_Automaton> &resources, typename _Automaton::ConstRef f,
		  std::ostream &o, Fsa::StoredComponents what, bool progress) {
	/*
	if (!f->semiring()->hasProperty(Fsa::PropertyXmlIo)) {
	    std::cerr << "semiring " << f->semiring()->name() << " does not support xml i/o" << std::endl;
	    return false;
	}
	*/
	if (!o) return false;
	Core::XmlWriter xo(o);
	xo.putDeclaration("UTF-8");
	xo << "\n";
	Core::ProgressIndicator *p = 0;
	if (f->initialStateId() != Fsa::InvalidStateId) {
	    xo << Core::XmlOpen("fsa")
		+ Core::XmlAttribute("type", Fsa::TypeChoice[f->type()])
		+ Core::XmlAttribute("semiring", f->semiring()->name())
		+ Core::XmlAttribute("initial", f->initialStateId()) << "\n";
	} else {
	    xo << Core::XmlOpen("fsa")
		+ Core::XmlAttribute("type", Fsa::TypeChoice[f->type()])
		+ Core::XmlAttribute("semiring", f->semiring()->name()) << "\n";
	}

	xo << "\n" << Core::XmlOpenComment() << "Properties were (only saved in bin format) '" << f->properties() << "'" << Core::XmlCloseComment() << "\n\n";

	if (what & Fsa::storeInputAlphabet)
	    if (f->getInputAlphabet()) {
		xo << Core::XmlOpen("input-alphabet") << "\n";
		f->getInputAlphabet()->writeXml(xo);
		xo << Core::XmlClose("input-alphabet") << "\n";
	    }
	if (what & Fsa::storeOutputAlphabet && f->type() == Fsa::TypeTransducer)
	    if (f->getOutputAlphabet()) {
		xo << Core::XmlOpen("output-alphabet") << "\n";
		f->getOutputAlphabet()->writeXml(xo);
		xo << Core::XmlClose("output-alphabet") << "\n";
	    }
	if (what & Fsa::storeStates) {
	    if (progress) p = new Core::ProgressIndicator("writing", "states");
	    WriteXmlDfsState<_Automaton> s(f, xo);
	    s.dfs(p);
	    delete p;
	}
	xo << Core::XmlClose("fsa") << "\n";
	return true;
    }

    template<class _Automaton>
    bool writeXml(const Resources<_Automaton> &resources, typename _Automaton::ConstRef f,
		  const std::string &file, Fsa::StoredComponents what, bool progress) {
	Core::CompressedOutputStream o(file);
	return writeXml<_Automaton>(resources, f, o, what, progress);
    }


    template<class _Automaton>
    class EdgeTrWGWriter;

    template<class _Automaton>
    class NodeTrWGWriter : public DfsState<_Automaton> {
	friend class EdgeTrWGWriter<_Automaton>;
    private:
	typedef DfsState<_Automaton> Precursor;
    public:
	typedef typename _Automaton::ConstStateRef _ConstStateRef;
	typedef typename _Automaton::ConstRef _ConstAutomatonRef;
    private:
	std::ostream &o_;
	Fsa::StateId max_id_;
	Fsa::StateId end_id_; // ID of end state
	std::set<Fsa::StateId> final_states_;
    public:
	NodeTrWGWriter(_ConstAutomatonRef f, std::ostream &o) : Precursor(f), o_(o), max_id_(0) {}

	virtual void discoverState(_ConstStateRef sp) {
	    if (max_id_ < sp->id()) max_id_ = sp->id();
	    if (!sp->isFinal())
		o_ << " <node> " << sp->id() << " </node>" << std::endl;
	    else
		final_states_.insert(sp->id());
	}

	virtual void finish() {
	    if (!final_states_.empty()) {
		if (new_end()) {
		    //introduce new final state
		    ++max_id_;
		    end_id_ = max_id_;
		    for (std::set<Fsa::StateId>::iterator s = final_states_.begin(); s != final_states_.end(); ++s)
			o_ << " <node> " << *s << " </node>" << std::endl;
		    o_ << " <node> " << end_id_ << " </node>" << std::endl;
		    //edges have to the new end states have to be written manually
		} else {
		    end_id_ = *final_states_.begin();
		    o_ << " <node> " << end_id_ << " </node>" << std::endl;
		}
	    }
	}

	bool new_end() {
	    return (final_states_.size() > 1);
	}

	bool hasFinalState() {
	    return (!final_states_.empty());
	}

	Fsa::StateId end() {
	    return end_id_;
	}
    };

    template<class _Automaton>
    class EdgeTrWGWriter : public DfsState<_Automaton> {
    private:
	typedef DfsState<_Automaton> Precursor;
    public:
	typedef typename _Automaton::Arc _Arc;
	typedef typename _Automaton::ConstStateRef _ConstStateRef;
	typedef typename _Automaton::ConstRef _ConstAutomatonRef;
    protected:
	NodeTrWGWriter<_Automaton> &nw_;
	std::ostream &o_;
	Fsa::ConstAlphabetRef inAlpha_;
    public:
	EdgeTrWGWriter(_ConstAutomatonRef f, NodeTrWGWriter<_Automaton> &nw, std::ostream &o) : Precursor(f), nw_(nw), o_(o) {
	    inAlpha_ = f->getInputAlphabet();
	}

	// taken from WordGraph.h in the WGTools Directory (Translation)
	std::string convert_to_xml_encoding(const std::string &s) const {
	    std::string result("");
	    for (u32 i = 0; i < s.size(); ++i) {
		if      (s[i] == '<') result += "\\<";
		else if (s[i] == '>') result += "\\>";
		else if (s[i] == '\\') result += "\\\\";
		else if (s[i] == '"' && (i == 0 || s[i-1] !=  '\\')) result += "\\\"";
		else result += s[i];
	    }
	    return result;
	}

	virtual void exploreTreeArc(_ConstStateRef from, const _Arc &a) {
	    std::string label = Core::itoa(a.input());
	    if (inAlpha_)
		label = inAlpha_->symbol(a.input());
	    if (label=="") {
		label = "UNKNOWN";
		std::cerr << "warning: unmapped index " << a.input() << std::endl;
	    }
	    o_ << " <edge weight=\"" << Precursor::fsa_->semiring()->asString(a.weight())
	       << "\" label=\"" << convert_to_xml_encoding(label) << "\" > ";
	    o_ << from->id() << "," << a.target() << " </edge>" << std::endl;
	}

	virtual void exploreNonTreeArc(_ConstStateRef from, const _Arc &a) {
	    exploreTreeArc(from, a);
	}

	virtual void finish() {
	    if (nw_.new_end()) {
		for (std::set<Fsa::StateId>::iterator s = nw_.final_states_.begin(); s != nw_.final_states_.end(); ++s) {
		    o_ << " <edge weight=\"0\" label=\"\\<\\/s\\>\" > ";
		    o_ << (*s) << "," << nw_.end() << " </edge>" << std::endl;
		}
	    }
	}
    };

    template<class _Automaton>
    bool writeTrXml(const Resources<_Automaton> &resources, typename _Automaton::ConstRef f,
		    std::ostream &o, Fsa::StoredComponents what, bool progress) {
	/*
	if (!f->semiring()->hasProperty(Fsa::PropertyStringIo)) {
	    std::cerr << "semiring " << f->semiring()->name() << " does not support string i/o" << std::endl;
	    return false;
	}
	*/
	o << "<graph>" << std::endl;
	NodeTrWGWriter<_Automaton> nodeWriter(f, o);
	nodeWriter.dfs();
	if (nodeWriter.hasFinalState()) {
	    EdgeTrWGWriter<_Automaton> edgeWriter(f, nodeWriter, o);
	    edgeWriter.dfs();
	}
	o << "</graph> " << std::endl;
	return o;
    }

    template<class _Automaton>
    bool writeTrXml(const Resources<_Automaton> &resources, typename _Automaton::ConstRef f,
		    const std::string &file, Fsa::StoredComponents what, bool progress) {
	Core::CompressedOutputStream o(file);
	return writeTrXml<_Automaton>(resources, f, o, what, progress);
    }

} // namespace Ftl
