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
#include "AlignerModelAcceptor.hh"
#include "FsaCache.hh"
#include "ModelCombination.hh"

using namespace Speech;


AlignerModelAcceptorGenerator::AlignerModelAcceptorGenerator(const Core::Configuration &c) :
    Component(c),
    Precursor(c),
    allophoneStateGraphBuilder_(0),
    cache_(0)
{
    ModelCombination modelCombination(
	select("model-combination"),
	ModelCombination::useAcousticModel,
	Am::AcousticModel::noEmissions | Am::AcousticModel::noStateTying);
    modelCombination.load();

    allophoneStateGraphBuilder_ = new AllophoneStateGraphBuilder(
	select("allophone-state-graph-builder"),
	modelCombination.lexicon(),
	modelCombination.acousticModel());

    cache_ = new FsaCache(select("model-acceptor-cache"), Fsa::storeStates);
    Core::DependencySet dependencies;
    modelCombination.getDependencies(dependencies);
    cache_->setDependencies(dependencies);
}

AlignerModelAcceptorGenerator::~AlignerModelAcceptorGenerator()
{
    delete cache_;
    delete allophoneStateGraphBuilder_;
}

void AlignerModelAcceptorGenerator::enterSpeechSegment(Bliss::SpeechSegment *s)
{
    require(s != 0);

    Precursor::enterSpeechSegment(s);
    cache_->get(allophoneStateGraphBuilder_->createFunctor(*s));
}
