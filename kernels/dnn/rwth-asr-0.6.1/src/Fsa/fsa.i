%module fsa

%{
#include "Alphabet.hh"
#include "Arithmetic.hh"
#include "Automaton.hh"
#include "Basic.hh"
#include "Best.hh"
#include "Cache.hh"
#include "Compose.hh"
#include "Determinize.hh"
#include "Input.hh"
#include "Output.hh"
#include "Permute.hh"
#include "Project.hh"
#include "Prune.hh"
#include "Python.hh"
#include "Rational.hh"
#include "RemoveEpsilons.hh"
#include "Sort.hh"
#include "Sssp.hh"

using namespace Fsa;
%}

%typemap(python,in) const std::string&, std::string&, std::string {
    if (!PyString_Check($input)) {
	PyErr_SetString(PyExc_TypeError, "not a string");
	return NULL;
    }
    $1 = new std::string(PyString_AsString($input));
}

%typemap(python, out) const std::string&, const std::string, std::string {
    $result = PyString_FromString(($1).c_str());
}

// from Arithmetic.hh
ConstAutomatonRef collect(ConstAutomatonRef f, Weight value);
ConstAutomatonRef extend(ConstAutomatonRef f, Weight value);
ConstAutomatonRef multiply(ConstAutomatonRef f, Weight value);

// from Basic.hh
ConstAutomatonRef mapInput(ConstAutomatonRef f, ConstAlphabetRef alphabet, bool reportUnknowns = true);
ConstAutomatonRef mapOutput(ConstAutomatonRef f, ConstAlphabetRef alphabet, bool reportUnknowns = true);
ConstAutomatonRef trim(ConstAutomatonRef f, bool progress = false);
ConstAutomatonRef partial(ConstAutomatonRef f, StateId initialStateOfPartialAutomaton);
ConstAutomatonRef changeSemiring(ConstAutomatonRef f, ConstSemiringRef semiring);

// from Best.hh
ConstAutomatonRef best(ConstAutomatonRef f);
ConstAutomatonRef nbest(ConstAutomatonRef f, size_t n = 1);

// from Cache.hh
ConstAutomatonRef cache(ConstAutomatonRef f, u32 maxAge = 10000);

// from Compose.hh
ConstAutomatonRef composeMatching(ConstAutomatonRef fl, ConstAutomatonRef fr, bool reportUnknowns = true);
ConstAutomatonRef composeSequencing(ConstAutomatonRef fl, ConstAutomatonRef fr, bool reportUnknowns = true);

// from Determinize.hh
ConstAutomatonRef determinize(ConstAutomatonRef f);
ConstAutomatonRef removeDisambiguationSymbols(ConstAutomatonRef f);

// from Permute.hh
ConstAutomatonRef permute(ConstAutomatonRef f, u32 windowSize);
ConstAutomatonRef localPermute(ConstAutomatonRef f, u32 windowSize);

// from Project.hh
ConstAutomatonRef projectInput(ConstAutomatonRef f);
ConstAutomatonRef projectOutput(ConstAutomatonRef f);
ConstAutomatonRef invert(ConstAutomatonRef f);

// from Prune.hh
ConstAutomatonRef prunePosterior(ConstAutomatonRef f, const Weight &threshold);
ConstAutomatonRef pruneSync(ConstAutomatonRef f, const Weight &threshold);

// from Rational.hh
ConstAutomatonRef closure(ConstAutomatonRef f);
ConstAutomatonRef kleeneClosure(ConstAutomatonRef f);
ConstAutomatonRef transpose(ConstAutomatonRef f, bool progress = false);

// from RemoveEpsilons.hh
ConstAutomatonRef removeEpsilons(ConstAutomatonRef f);

// from Sort.hh
ConstAutomatonRef sort(ConstAutomatonRef f, SortType type);

// from Sssp.hh
ConstAutomatonRef pushToInitial(ConstAutomatonRef f);
ConstAutomatonRef pushToFinal(ConstAutomatonRef f);
ConstAutomatonRef posterior(ConstAutomatonRef f);



// from Python.hh
class AutomatonCounts {
  public:
    StateId maxStateId_;
    StateId nStates_;
    StateId nFinals_;
    size_t nArcs_;
    size_t nIoEps_;
    size_t nIEps_;
    size_t nOEps_;
  public:
    AutomatonCounts() : maxStateId_(0), nStates_(0), nFinals_(0), nArcs_(0), nIoEps_(0), nIEps_(0), nOEps_(0) {}
};
AutomatonCounts count(ConstAutomatonRef f, bool progress = false);
size_t countInput(ConstAutomatonRef f, LabelId label, bool progress = false);
size_t countOutput(ConstAutomatonRef f, LabelId label, bool progress = false);

class AlphabetCounts {
  public:
    LabelId maxLabelId_;
    LabelId nLabels_;
  public:
    AlphabetCounts() : maxLabelId_(0), nLabels_(0) {}
};
//AlphabetCounts count(ConstAlphabetRef a, bool progress = false);
bool write(ConstAutomatonRef f, const std::string &file, bool progress = false);
const std::string draw(ConstAutomatonRef f, bool dumpStates = false, bool progress = false);
const std::string info(ConstAutomatonRef f, bool progress = false);
const std::string meminfo(ConstAutomatonRef f);
ConstAutomatonRef read(const std::string &file);
