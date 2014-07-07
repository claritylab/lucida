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
#ifndef _FSA_TYPES_HH
#define _FSA_TYPES_HH

#include <Core/Choice.hh>
#include <Core/Types.hh>
#include <Core/Vector.hh>

namespace Fsa {
    /**
     * state indices and tags
     **/
    typedef u32 StateId;
    typedef StateId StateTag;
    static const StateTag StateTagNone    = 0x00000000;
    static const StateTag StateTagFinal   = 0x80000000;
    static const StateTag StateTagUser    = 0x40000000;
    static const StateTag StateTagAll     = 0xc0000000;
    static const StateTag StateTagMask    = 0xc0000000;
    static const StateTag StateIdMask     = 0x3fffffff;
    static const int StateIdBits = 30;

    static const StateId InvalidStateId   = StateIdMask;

    typedef Core::Vector<StateId> StateMap;


    /**
       label indices and special symbols
    **/
    typedef s32 LabelId;
    // must be -1 due to optimized composition/determinization and hardcoded index shift for AT&T format
    static const LabelId Epsilon          = -1;
    static const LabelId FirstLabelId     = 0;
    static const LabelId LastLabelId      = 2147483647 - 4;

    // the following labels match
    static const LabelId Else             = 2147483647 - 3; // if no other arc matches, replaced by epsilon
    static const LabelId Failure          = 2147483647 - 2; // if no other arc matches
    static const LabelId Any              = 2147483647 - 1; // any of the own alphabet
    static const LabelId InvalidLabelId   = Core::Type<LabelId>::max;

    typedef enum { TypeUnknown = 0, TypeAcceptor = 1, TypeTransducer = 2 } Type;
    extern Core::Choice TypeChoice;


    /**
     * properties
     **/
    typedef u32 Property;
    static const Property PropertyNone                    = 0x00000000;
    static const Property PropertyAll                     = 0xffffffff;

    /**
     * automaton properties
     *
     * storage:
     * - automaton stores all states and arcs (instead of calculating them)
     *
     * linear:
     * - outdegree (and therefore indegree) of all states is 1
     *
     * sausages:
     * - automaton has an initial and a single final state
     * - indegree and outdegree of all states except initial and final is 1
     *
     * acyclic:
     * - automaton does not contain any cycle except the trivial ones
     *
     * cached:
     * - automaton is being cached; helps to avoid multiple caching
     *   (we will use a special class field for this in the future)
     *
     * for compatibility reasons only append new properties to the end
     *
     * CAUTION: Do not change the numbers!  Extend the list only with unused values!
     **/
    // storage
    static const Property PropertyStorage                 = 0x00000001;
    static const Property PropertyCached                  = 0x00000040;

    // structure
    static const Property PropertyLinear                  = 0x00000010;
    static const Property PropertyAcyclic                 = 0x00000020;
    static const Property PropertySausages                = 0x00000080;

    // ordering of arcs
    static const Property PropertySorted                  = 0x00000f0e;
    static const Property PropertySortedByArc             = 0x00000002;
    static const Property PropertySortedByInput           = 0x00000004;
    static const Property PropertySortedByOutput          = 0x00000008;
    static const Property PropertySortedByInputAndOutput  = 0x00000400;
    static const Property PropertySortedByInputAndTarget  = 0x00000100;
    static const Property PropertySortedByInputAndOutputAndTarget = 0x00000800;
    static const Property PropertySortedByWeight          = 0x00000200;


    extern Core::Choice PropertyChoice;


    /**
     * semiring properties
     *
     **/
    static const Property PropertySemiringAll             = 0x000007ff;
    // semiring, sum
    static const Property PropertyCommutative             = 0x00000001;
    static const Property PropertyRightDistributive       = 0x00000002;
    static const Property PropertyZeroSumFree             = 0x00000004;
    // semiring, divisibility
    static const Property PropertyWeaklyLeftDivisble      = 0x00000008;
    static const Property PropertyCancellative            = 0x00000012;
    static const Property PropertyLeftDivisible           = 0x00000038;
    // semiring, set
    static const Property PropertyPartialOrder            = 0x00000040;
    static const Property PropertyStrictOrder             = 0x00000080;
    // semiring, i/o
    static const Property PropertySemiringIo              = 0x00000300;
    static const Property PropertyStringIo                = 0x00000100;
    static const Property PropertyBinaryIo                = 0x00000200;


    /**
     * Formatting hints
     **/
    typedef u32 Hint;
    static const Hint HintNone                        = 0x00000000;
    static const Hint HintAll                         = 0x00000007;
    static const Hint HintShowDetails                 = 0x00000001;
    static const Hint HintMarkBest                    = 0x00000002;
    static const Hint HintAsProbability               = 0x00000004;
    static const Hint HintUserMask                    = 0xffff0000;


    /**
     * Components to be stored
     **/
    typedef u32 StoredComponents;
    static const StoredComponents storeStates         = 0x00000001;
    static const StoredComponents storeInputAlphabet  = 0x00000002;
    static const StoredComponents storeOutputAlphabet = 0x00000004;
    static const StoredComponents storeAlphabets      = 0x00000006;
    static const StoredComponents storeAll            = 0xffffffff;

    /**
     * Alphabet Tags
     * use higher 4 bits for user-defined tags
     * other tags may be defined in future versions
     */
    typedef u8 AlphabetTag;
    static const AlphabetTag alphabetTagDisambiguator = 0x01;
    static const AlphabetTag alphabetTagUserMask      = 0xf0;


    /**
     * semiring type
     * deprecated, use symbolic resolution
     **/
    typedef enum {
	SemiringTypeUnknown = 0,
	SemiringTypeLog = 1,
	SemiringTypeTropical = 2,
	SemiringTypeTropicalInteger = 3,
	SemiringTypeCount = 4,
	SemiringTypeProbability = 5,
	SemiringTypeExpectation = 6
    } SemiringType;



    /**
     * Optimization hints
     **/
    typedef u32 OptimizationHint;
    static const OptimizationHint OptimizationHintNone      = 0x00000000;
    static const OptimizationHint OptimizarionHintAll       = 0x00000003;
    static const OptimizationHint OptimizationHintMemory    = 0x00000001;
    static const OptimizationHint OptimizationHintSpeed     = 0x00000002;

    extern Core::Choice OptimizationHintChoice;
} // namespace Fsa

#endif // _FSA_TYPES_HH
