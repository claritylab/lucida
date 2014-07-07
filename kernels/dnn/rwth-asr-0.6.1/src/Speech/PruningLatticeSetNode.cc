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
#include "PruningLatticeSetNode.hh"
#include <Lattice/Posterior.hh>

using namespace Speech;

/**
 * pruning node
 */
const Core::ParameterFloat PruningLatticeSetNode::paramThreshold(
    "threshold",
    "threshold used for posterior pruning of word lattices",
    Core::Type<f32>::max);

const Core::ParameterBool PruningLatticeSetNode::paramThresholdIsRelative(
    "threshold-is-relative",
    "threshold used relative to best path",
    true);

const Core::ParameterBool PruningLatticeSetNode::paramHasFailArcs(
    "has-fail-arcs",
    "Used Automata have fail arcs",
    false);

Core::Choice PruningLatticeSetNode::choicePruningType(
    "forward-backward", forwardBackward,
    "forward", forward,
    Core::Choice::endMark());

const Core::ParameterChoice PruningLatticeSetNode::paramPruningType(
    "pruning-type",
    &choicePruningType,
    "type of pruning",
    forwardBackward);

PruningLatticeSetNode::PruningLatticeSetNode(const Core::Configuration &c) :
    Core::Component(c),
    Precursor(c),
    threshold_(Fsa::Weight(paramThreshold(config))),
    thresholdIsRelative_(paramThresholdIsRelative(config)),
    hasFailArcs_(paramHasFailArcs(config)),
    pruningType_((PruningType)paramPruningType(config))
{
    log("using pruning threshold: ") << f64(threshold_);
    if (thresholdIsRelative_)
	log("threshold is relative");
}

PruningLatticeSetNode::~PruningLatticeSetNode()
{}

void PruningLatticeSetNode::processWordLattice(
    Lattice::ConstWordLatticeRef lattice, Bliss::SpeechSegment *s)
{
    if (lattice->nParts() != 1) {
	criticalError("number of lattice parts must be 1");
    }
    Lattice::ConstWordLatticeRef pruned =
	Lattice::prune(lattice, threshold_, thresholdIsRelative_, true, hasFailArcs_);
    Precursor::processWordLattice(pruned, s);
}
