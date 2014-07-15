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
#include <Core/Application.hh>

#include "FlfCore/LatticeInternal.hh"
#include "Filter.hh"
#include "Lexicon.hh"

namespace Flf {

    // -------------------------------------------------------------------------
    template<class ArcFilter>
    class ArcFilterLattice : public SlaveLattice {
	typedef SlaveLattice Precursor;
    private:
	ArcFilter filter_;
    public:
	ArcFilterLattice(ConstLatticeRef l, const ArcFilter &filter) :
	    Precursor(l), filter_(filter) {}
	virtual ~ArcFilterLattice() {}

	virtual ConstStateRef getState(Fsa::StateId sid) const {
	    ConstStateRef sr = fsa_->getState(sid);
	    State *sp = new State(sr->id(), sr->tags(), sr->weight());
	    for (State::const_iterator a = sr->begin(); a != sr->end(); ++a)
		if (filter_(*a))
		    *sp->newArc() = *a;
	    return ConstStateRef(sp);
	}

	virtual std::string describe() const {
	    return Core::form(
		"filter(%s,%s)",
		fsa_->describe().c_str(),
		filter_.describe().c_str());
	}
    };
    // -------------------------------------------------------------------------


    // -------------------------------------------------------------------------
    struct InputFilter {
	Fsa::ConstAlphabetRef alphabet;
	Fsa::LabelId input;
	InputFilter(Fsa::ConstAlphabetRef alphabet, Fsa::LabelId input) :
	    alphabet(alphabet), input(input) {}
	bool operator() (const Arc &a) const
	    { return a.input() == input; }
	std::string describe() const
	    { return Core::form("input=%s", alphabet->symbol(input).c_str()); }
    };
    ConstLatticeRef filterByInput(ConstLatticeRef l, Fsa::LabelId input) {
	return ConstLatticeRef(new ArcFilterLattice<InputFilter>(l, InputFilter(l->getInputAlphabet(), input)));
    }
    ConstLatticeRef filterByInput(ConstLatticeRef l, const std::string &label) {
	Fsa::LabelId input = l->getInputAlphabet()->index(label);
	if (input == Fsa::InvalidLabelId)
	    Core::Application::us()->criticalError(
		"Filter: Label \"%s\" is not in input alphabet of lattice \"%s\".",
		label.c_str(), l->describe().c_str());
	return filterByInput(l, input);
    }
    // -------------------------------------------------------------------------


    // -------------------------------------------------------------------------
    struct OutputFilter {
	Fsa::ConstAlphabetRef alphabet;
	Fsa::LabelId output;
	OutputFilter(Fsa::ConstAlphabetRef alphabet, Fsa::LabelId output) :
	    alphabet(alphabet), output(output) {}
	bool operator() (const Arc &a) const
	    { return a.output() == output; }
	std::string describe() const
	    { return Core::form("output=%s", alphabet->symbol(output).c_str()); }
    };
    ConstLatticeRef filterByOutput(ConstLatticeRef l, Fsa::LabelId output) {
	return ConstLatticeRef(new ArcFilterLattice<OutputFilter>(l, OutputFilter(l->getOutputAlphabet(), output)));
    }
    ConstLatticeRef filterByOutput(ConstLatticeRef l, const std::string &label) {
	Fsa::LabelId output = l->getOutputAlphabet()->index(label);
	if (output == Fsa::InvalidLabelId)
	    Core::Application::us()->criticalError(
		"Filter: Label \"%s\" is not in output alphabet of lattice \"%s\".",
		label.c_str(), l->describe().c_str());
	return filterByOutput(l, output);
    }
    // -------------------------------------------------------------------------


    // -------------------------------------------------------------------------
    class FilterArcNode : public FilterNode {
    public:
	static const Core::ParameterString paramInput;
	static const Core::ParameterString paramOutput;

    private:
	std::string input_;
	std::string output_;

    protected:
	virtual ConstLatticeRef filter(ConstLatticeRef l) {
	    if (!l)
		return ConstLatticeRef();
	    if (!input_.empty()) {
		Fsa::LabelId inputLabel = l->getInputAlphabet()->index(input_);
		if (inputLabel != Fsa::InvalidLabelId)
		    l = filterByInput(l, inputLabel);
		else
		    criticalError(
			"Label \"%s\" not in intput \"%s\" alphabet.",
			input_.c_str(),
			Lexicon::us()->alphabetName(Lexicon::us()->alphabetId(l->getInputAlphabet())).c_str());
	    }
	    if (!output_.empty()) {
		Fsa::LabelId outputLabel = l->getOutputAlphabet()->index(output_);
		if (outputLabel != Fsa::InvalidLabelId)
		    l = filterByOutput(l, outputLabel);
		else
		    criticalError(
			"Label \"%s\" not in intput \"%s\" alphabet.",
			output_.c_str(),
			Lexicon::us()->alphabetName(Lexicon::us()->alphabetId(l->getOutputAlphabet())).c_str());
	    }
	    return l;
	}

    public:
	FilterArcNode(const std::string &name, const Core::Configuration &config) :
	    FilterNode(name, config) {}
	~FilterArcNode() {}

	virtual void init(const std::vector<std::string> &arguments) {
	    input_ = paramInput(config);
	    output_ = paramOutput(config);
	    Core::Component::Message msg = log();
	    if (!input_.empty())
		msg << "Filter by input \"" << input_ << "\"\n";
	    if (!output_.empty())
		msg << "Filter by output \"" << output_ << "\"\n";
	}
    };
    const Core::ParameterString FilterArcNode::paramInput(
	"input",
	"input");
    const Core::ParameterString FilterArcNode::paramOutput(
	"output",
	"output");
    NodeRef createFilterArcNode(const std::string &name, const Core::Configuration &config) {
	return NodeRef(new FilterArcNode(name, config));
    }
    // -------------------------------------------------------------------------

} // namespace Flf
