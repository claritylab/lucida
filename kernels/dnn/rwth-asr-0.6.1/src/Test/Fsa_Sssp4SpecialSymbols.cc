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
/**
 */

#include <Test/UnitTest.hh>

#include <Fsa/Automaton.hh>
#include <Fsa/Static.hh>
#include <Fsa/Sort.hh>
#include <Fsa/Sssp4SpecialSymbols.hh>
//#include <Fsa/Output.hh>
#include <Fsa/Basic.hh>

namespace Fsa {

ConstAutomatonRef getTestAutomaton() {
	StaticAlphabet *a = new StaticAlphabet();
	a->addIndexedSymbol("A", 0);
	a->addIndexedSymbol("B", 1);
	ConstAlphabetRef alphabet(a);
	LabelId A = alphabet->index("A");
	LabelId B = alphabet->index("B");
	verify(A == 0);
	verify(B == 1);

	Fsa::StaticAutomaton* f = new Fsa::StaticAutomaton();
	f->setSemiring(Fsa::LogSemiring);
	f->setInputAlphabet(alphabet);
	f->clear();
	f->setType(Fsa::TypeAcceptor);
	f->addProperties(Fsa::PropertyStorage | Fsa::PropertyAcyclic);

	Fsa::State* s0 = f->newState();
	Fsa::State* s1 = f->newState();
	Fsa::State* s2 = f->newState();
	Fsa::State* s3 = f->newState();
	Fsa::State* s4 = f->newState();
	Fsa::State* s5 = f->newState();
	Fsa::State* s6 = f->newState();
	Fsa::State* s7 = f->newState();

	f->setInitialStateId(s0->id());
	f->setStateFinal(s7);

	s0->newArc(s1->id(), Weight(1.0), A);
	s0->newArc(s2->id(), Weight(2.0), B);

	s1->newArc(s4->id(), Weight(2.0), A);
	s1->newArc(s3->id(), Weight(1.0), Failure);

	s2->newArc(s3->id(), Weight(2.0), Failure);
	s2->newArc(s5->id(), Weight(3.0), B);

	s3->newArc(s4->id(), Weight(1.0), A);
	s3->newArc(s5->id(), Weight(0.0), B);

	s4->newArc(s7->id(), Weight(2.0), A);
	s4->newArc(s6->id(), Weight(2.0), Failure);

	s5->newArc(s6->id(), Weight(1.0), Failure);
	s5->newArc(s7->id(), Weight(1.0), B);

	s6->newArc(s7->id(), Weight(3.0), A);
	s6->newArc(s7->id(), Weight(0.0), B);

	return ConstAutomatonRef(f);
}

ConstAutomatonRef getTestAutomatonWithAnyAndElse() {
	StaticAlphabet *a = new StaticAlphabet();
	a->addIndexedSymbol("A", 0);
	a->addIndexedSymbol("B", 1);
	ConstAlphabetRef alphabet(a);
	LabelId A = alphabet->index("A");
	LabelId B = alphabet->index("B");
	verify(A == 0);
	verify(B == 1);

	Fsa::StaticAutomaton* f = new Fsa::StaticAutomaton();
	f->setSemiring(Fsa::LogSemiring);
	f->setInputAlphabet(alphabet);
	f->clear();
	f->setType(Fsa::TypeAcceptor);
	f->addProperties(Fsa::PropertyStorage | Fsa::PropertyAcyclic);

	Fsa::State* s0 = f->newState();
	Fsa::State* s1 = f->newState();
	Fsa::State* s2 = f->newState();
	Fsa::State* s3 = f->newState();
	Fsa::State* s4 = f->newState();
	Fsa::State* s5 = f->newState();
	Fsa::State* s6 = f->newState();
	Fsa::State* s7 = f->newState();

	f->setInitialStateId(s0->id());
	f->setStateFinal(s7);

	s0->newArc(s1->id(), Weight(1.0), A);
	s0->newArc(s2->id(), Weight(2.0), B);

	s1->newArc(s4->id(), Weight(2.0), A);
	s1->newArc(s3->id(), Weight(1.0), Failure);

	s2->newArc(s3->id(), Weight(2.0), Failure);
	s2->newArc(s5->id(), Weight(3.0), B);

	s3->newArc(s4->id(), Weight(1.0), A);
	s3->newArc(s5->id(), Weight(0.0), Else);

	s4->newArc(s7->id(), Weight(2.0), A);
	s4->newArc(s6->id(), Weight(2.0), Failure);

	s5->newArc(s6->id(), Weight(1.0), Failure);
	s5->newArc(s7->id(), Weight(1.0), B);

	s6->newArc(s7->id(), Weight(1.0), Any);

	return ConstAutomatonRef(f);
}

TEST(Fsa, Sssp4SpecialSymbols, removeFailure4SpecialSymbols) {
	ConstAutomatonRef result = sort(removeFailure4SpecialSymbols(getTestAutomaton()), Fsa::SortTypeByInputAndOutput);
	LabelId A = result->getInputAlphabet()->index("A");
	LabelId B = result->getInputAlphabet()->index("B");

	//write(result, "result.fsa.gz");

	// state 0
	ConstStateRef s0 = result->getState(result->initialStateId());
	EXPECT_EQ((u32)2, s0->nArcs());

	EXPECT_EQ(A, s0->getArc(0)->input());
	EXPECT_DOUBLE_EQ(1.0, (f32)s0->getArc(0)->weight(), 0.001);
	EXPECT_EQ(B, s0->getArc(1)->input());
	EXPECT_DOUBLE_EQ(2.0, (f32)s0->getArc(1)->weight(), 0.001);

	// state 1
	ConstStateRef s1 = result->getState(s0->getArc(0)->target());
	EXPECT_EQ((u32)2, s1->nArcs());

	EXPECT_EQ(A, s1->getArc(0)->input());
	EXPECT_DOUBLE_EQ(2.0, (f32)s1->getArc(0)->weight(), 0.001);
	EXPECT_EQ(B, s1->getArc(1)->input());
	EXPECT_DOUBLE_EQ(1.0, (f32)s1->getArc(1)->weight(), 0.001);

	// state 2
	ConstStateRef s2 = result->getState(s0->getArc(1)->target());
	EXPECT_EQ((u32)2, s2->nArcs());

	EXPECT_EQ(A, s2->getArc(0)->input());
	EXPECT_DOUBLE_EQ(3.0, (f32)s2->getArc(0)->weight(), 0.001);
	EXPECT_EQ(B, s2->getArc(1)->input());
	EXPECT_DOUBLE_EQ(3.0, (f32)s2->getArc(1)->weight(), 0.001);

	// state 3
	EXPECT_EQ(s1->getArc(0)->target(), s2->getArc(0)->target());
	ConstStateRef s3 = result->getState(s1->getArc(0)->target());
	EXPECT_EQ((u32)2, s3->nArcs());

	EXPECT_EQ(A, s3->getArc(0)->input());
	EXPECT_DOUBLE_EQ(2.0, (f32)s3->getArc(0)->weight(), 0.001);
	EXPECT_EQ(B, s3->getArc(1)->input());
	EXPECT_DOUBLE_EQ(2.0, (f32)s3->getArc(1)->weight(), 0.001);

	// state 4
	EXPECT_EQ(s1->getArc(1)->target(), s2->getArc(1)->target());
	ConstStateRef s4 = result->getState(s1->getArc(1)->target());
	EXPECT_EQ((u32)2, s4->nArcs());

	EXPECT_EQ(A, s4->getArc(0)->input());
	EXPECT_DOUBLE_EQ(4.0, (f32)s4->getArc(0)->weight(), 0.001);
	EXPECT_EQ(B, s4->getArc(1)->input());
	EXPECT_DOUBLE_EQ(1.0, (f32)s4->getArc(1)->weight(), 0.001);

	// state 5
	EXPECT_EQ(s3->getArc(0)->target(), s3->getArc(1)->target());
	EXPECT_EQ(s4->getArc(0)->target(), s4->getArc(1)->target());
	EXPECT_EQ(s3->getArc(0)->target(), s4->getArc(0)->target());
	ConstStateRef s5 = result->getState(s3->getArc(0)->target());
	EXPECT_EQ((u32)0, s5->nArcs());
	EXPECT_TRUE(s5->isFinal());
}

TEST(Fsa, Sssp4SpecialSymbols, best4SpecialSymbols) {
	ConstAutomatonRef result = sort(best4SpecialSymbols(getTestAutomaton()), Fsa::SortTypeByInputAndOutput);
	LabelId A = result->getInputAlphabet()->index("A");
	LabelId B = result->getInputAlphabet()->index("B");

	//write(result, "result.fsa.gz");

	// state 0
	ConstStateRef s0 = result->getState(result->initialStateId());
	EXPECT_EQ((u32)1, s0->nArcs());

	EXPECT_EQ(A, s0->getArc(0)->input());
	EXPECT_DOUBLE_EQ(1.0, (f32)s0->getArc(0)->weight(), 0.001);

	// state 1
	ConstStateRef s1 = result->getState(s0->getArc(0)->target());
	EXPECT_EQ((u32)1, s1->nArcs());

	EXPECT_EQ(B, s1->getArc(0)->input());
	EXPECT_DOUBLE_EQ(1.0, (f32)s1->getArc(0)->weight(), 0.001);

	// state 2
	ConstStateRef s2 = result->getState(s1->getArc(0)->target());
	EXPECT_EQ((u32)1, s2->nArcs());

	EXPECT_EQ(B, s2->getArc(0)->input());
	EXPECT_DOUBLE_EQ(1.0, (f32)s2->getArc(0)->weight(), 0.001);

	// state 3
	ConstStateRef s3 = result->getState(s2->getArc(0)->target());
	EXPECT_EQ((u32)0, s3->nArcs());
	EXPECT_TRUE(s3->isFinal());
}

TEST(Fsa, Sssp4SpecialSymbols, posterior4SpecialSymbolsLog) {
	static const double precision = 0.00001;

	ConstAutomatonRef test = getTestAutomaton();
	Weight totalInv;
	ConstAutomatonRef result = sort(posterior4SpecialSymbols(test, totalInv, 100), Fsa::SortTypeByInputAndOutput);
	LabelId A = result->getInputAlphabet()->index("A");
	LabelId B = result->getInputAlphabet()->index("B");

	//write(test, "test.fsa.gz");
	//write(result, "result-log.fsa.gz");
	//write(sort(posterior4SpecialSymbols(removeFailure4SpecialSymbols(test)), Fsa::SortTypeByInputAndOutput), "result-log-remove.fsa.gz");

	// state 0
	ConstStateRef s0 = result->getState(result->initialStateId());
	EXPECT_EQ((u32)2, s0->nArcs());

	EXPECT_EQ(A, s0->getArc(0)->input());
	EXPECT_DOUBLE_EQ(0.065154, (f32)s0->getArc(0)->weight(), precision);
	EXPECT_EQ(B, s0->getArc(1)->input());
	EXPECT_DOUBLE_EQ(2.763408, (f32)s0->getArc(1)->weight(), precision);

	// state 1
	ConstStateRef s1 = result->getState(s0->getArc(0)->target());
	EXPECT_EQ((u32)2, s1->nArcs());

	EXPECT_EQ(A, s1->getArc(0)->input());
	EXPECT_DOUBLE_EQ(1.649985, (f32)s1->getArc(0)->weight(), precision);
	EXPECT_EQ(Failure, s1->getArc(1)->input());
	EXPECT_DOUBLE_EQ(0.294545, (f32)s1->getArc(1)->weight(), precision);

	// state 2
	ConstStateRef s2 = result->getState(s0->getArc(1)->target());
	EXPECT_EQ((u32)2, s2->nArcs());

	EXPECT_EQ(B, s2->getArc(0)->input());
	EXPECT_DOUBLE_EQ(3.294545, (f32)s2->getArc(0)->weight(), precision);
	EXPECT_EQ(Failure, s2->getArc(1)->input());
	EXPECT_DOUBLE_EQ(3.649985, (f32)s2->getArc(1)->weight(), precision);

	// state 3
	EXPECT_EQ(s1->getArc(1)->target(), s2->getArc(1)->target());
	ConstStateRef s3 = result->getState(s1->getArc(1)->target());
	EXPECT_EQ((u32)2, s3->nArcs());

	EXPECT_EQ(A, s3->getArc(0)->input());
	EXPECT_DOUBLE_EQ(3.649985, (f32)s3->getArc(0)->weight(), precision);
	EXPECT_EQ(B, s3->getArc(1)->input());
	EXPECT_DOUBLE_EQ(0.294545, (f32)s3->getArc(1)->weight(), precision);

	// state 4
	EXPECT_EQ(s1->getArc(0)->target(), s3->getArc(0)->target());
	ConstStateRef s4 = result->getState(s1->getArc(0)->target());
	EXPECT_EQ((u32)2, s4->nArcs());

	EXPECT_EQ(A, s4->getArc(0)->input());
	EXPECT_DOUBLE_EQ(2.216204, (f32)s4->getArc(0)->weight(), precision);
	EXPECT_EQ(Failure, s4->getArc(1)->input());
	EXPECT_DOUBLE_EQ(2.216205, (f32)s4->getArc(1)->weight(), precision);

	// state 5
	EXPECT_EQ(s2->getArc(0)->target(), s3->getArc(1)->target());
	ConstStateRef s5 = result->getState(s2->getArc(0)->target());
	EXPECT_EQ((u32)2, s5->nArcs());

	EXPECT_EQ(B, s5->getArc(0)->input());
	EXPECT_DOUBLE_EQ(0.294545, (f32)s5->getArc(0)->weight(), precision);
	EXPECT_EQ(Failure, s5->getArc(1)->input());
	EXPECT_DOUBLE_EQ(3.294545, (f32)s5->getArc(1)->weight(), precision);

	// state 6
	EXPECT_EQ(s4->getArc(1)->target(), s5->getArc(1)->target());
	ConstStateRef s6 = result->getState(s4->getArc(1)->target());
	EXPECT_EQ((u32)2, s6->nArcs());

	EXPECT_EQ(A, s6->getArc(0)->input());
	EXPECT_DOUBLE_EQ(3.294545, (f32)s6->getArc(0)->weight(), precision);
	EXPECT_EQ(B, s6->getArc(1)->input());
	EXPECT_DOUBLE_EQ(2.216205, (f32)s6->getArc(1)->weight(), precision);

	// state 7
	EXPECT_EQ(s4->getArc(0)->target(), s5->getArc(0)->target());
	EXPECT_EQ(s4->getArc(0)->target(), s6->getArc(0)->target());
	EXPECT_EQ(s6->getArc(0)->target(), s6->getArc(1)->target());
	ConstStateRef s7 = result->getState(s6->getArc(0)->target());
	EXPECT_EQ((u32)0, s7->nArcs());
	EXPECT_TRUE(s7->isFinal());

	// check the total weight
	EXPECT_DOUBLE_EQ((f32)test->semiring()->invert(totalInv), 2.65686, 0.0001);
}

TEST(Fsa, Sssp4SpecialSymbols, posterior4SpecialSymbolsTropical) {
	static const double precision = 0.00001;

	ConstAutomatonRef test = changeSemiring(getTestAutomaton(), TropicalSemiring);
	Weight totalInv;
	ConstAutomatonRef result = sort(posterior4SpecialSymbols(test, totalInv, 100), Fsa::SortTypeByInputAndOutput);
	LabelId A = result->getInputAlphabet()->index("A");
	LabelId B = result->getInputAlphabet()->index("B");

	//write(test, "test.fsa.gz");
	//write(result, "result-trop.fsa.gz");

	// state 0
	ConstStateRef s0 = result->getState(result->initialStateId());
	EXPECT_EQ((u32)2, s0->nArcs());

	EXPECT_EQ(A, s0->getArc(0)->input());
	EXPECT_DOUBLE_EQ(0.0, (f32)s0->getArc(0)->weight(), precision);
	EXPECT_EQ(B, s0->getArc(1)->input());
	EXPECT_DOUBLE_EQ(3.0, (f32)s0->getArc(1)->weight(), precision);

	// state 1
	ConstStateRef s1 = result->getState(s0->getArc(0)->target());
	EXPECT_EQ((u32)2, s1->nArcs());

	EXPECT_EQ(A, s1->getArc(0)->input());
	EXPECT_DOUBLE_EQ(2.0, (f32)s1->getArc(0)->weight(), precision);
	EXPECT_EQ(Failure, s1->getArc(1)->input());
	EXPECT_DOUBLE_EQ(0.0, (f32)s1->getArc(1)->weight(), precision);

	// state 2
	ConstStateRef s2 = result->getState(s0->getArc(1)->target());
	EXPECT_EQ((u32)2, s2->nArcs());

	EXPECT_EQ(B, s2->getArc(0)->input());
	EXPECT_DOUBLE_EQ(3.0, (f32)s2->getArc(0)->weight(), precision);
	EXPECT_EQ(Failure, s2->getArc(1)->input());
	EXPECT_DOUBLE_EQ(4.0, (f32)s2->getArc(1)->weight(), precision);

	// state 3
	EXPECT_EQ(s1->getArc(1)->target(), s2->getArc(1)->target());
	ConstStateRef s3 = result->getState(s1->getArc(1)->target());
	EXPECT_EQ((u32)2, s3->nArcs());

	EXPECT_EQ(A, s3->getArc(0)->input());
	EXPECT_DOUBLE_EQ(4.0, (f32)s3->getArc(0)->weight(), precision);
	EXPECT_EQ(B, s3->getArc(1)->input());
	EXPECT_DOUBLE_EQ(0.0, (f32)s3->getArc(1)->weight(), precision);

	// state 4
	EXPECT_EQ(s1->getArc(0)->target(), s3->getArc(0)->target());
	ConstStateRef s4 = result->getState(s1->getArc(0)->target());
	EXPECT_EQ((u32)2, s4->nArcs());

	EXPECT_EQ(A, s4->getArc(0)->input());
	EXPECT_DOUBLE_EQ(2.0, (f32)s4->getArc(0)->weight(), precision);
	EXPECT_EQ(Failure, s4->getArc(1)->input());
	EXPECT_DOUBLE_EQ(2.0, (f32)s4->getArc(1)->weight(), precision);

	// state 5
	EXPECT_EQ(s2->getArc(0)->target(), s3->getArc(1)->target());
	ConstStateRef s5 = result->getState(s2->getArc(0)->target());
	EXPECT_EQ((u32)2, s5->nArcs());

	EXPECT_EQ(B, s5->getArc(0)->input());
	EXPECT_DOUBLE_EQ(0.0, (f32)s5->getArc(0)->weight(), precision);
	EXPECT_EQ(Failure, s5->getArc(1)->input());
	EXPECT_DOUBLE_EQ(3.0, (f32)s5->getArc(1)->weight(), precision);

	// state 6
	EXPECT_EQ(s4->getArc(1)->target(), s5->getArc(1)->target());
	ConstStateRef s6 = result->getState(s4->getArc(1)->target());
	EXPECT_EQ((u32)2, s6->nArcs());

	EXPECT_EQ(A, s6->getArc(0)->input());
	EXPECT_DOUBLE_EQ(3.0, (f32)s6->getArc(0)->weight(), precision);
	EXPECT_EQ(B, s6->getArc(1)->input());
	EXPECT_DOUBLE_EQ(2.0, (f32)s6->getArc(1)->weight(), precision);

	// state 7
	EXPECT_EQ(s4->getArc(0)->target(), s5->getArc(0)->target());
	EXPECT_EQ(s4->getArc(0)->target(), s6->getArc(0)->target());
	EXPECT_EQ(s6->getArc(0)->target(), s6->getArc(1)->target());
	ConstStateRef s7 = result->getState(s6->getArc(0)->target());
	EXPECT_EQ((u32)0, s7->nArcs());
	EXPECT_TRUE(s7->isFinal());

	// check the total weight
	EXPECT_DOUBLE_EQ((f32)test->semiring()->invert(totalInv), 3.0, precision);
}

}
