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
#include "Module.hh"
#include "AligningFeatureExtractor.hh"
#include "AlignmentNode.hh"
#include "AlignmentWithLinearSegmentation.hh"
#include "DataSource.hh"
#include "TextDependentSequenceFiltering.hh"
#include "MixtureSetTrainer.hh"
#include <Flow/Registry.hh>
#include <Flow/DataAdaptor.hh>
#include <Mm/Module.hh>
#ifdef MODULE_SPEECH_ALIGNMENT_FLOW_NODES
#include "AlignmentGeneratorNode.hh"
#include "AlignmentTransformNode.hh"
#endif
#ifdef MODULE_SPEECH_LATTICE_FLOW_NODES
#include "AlignmentFromLattice.hh"
#include "LatticeNodes.hh"
#include "LatticeArcAccumulator.hh"
#endif
#ifdef MODULE_SPEECH_DT
#include "DiscriminativeMixtureSetTrainer.hh"
#include "EbwDiscriminativeMixtureSetTrainer.hh"
#include "RpropDiscriminativeMixtureSetTrainer.hh"
#include "LatticeExtractor.hh"
#include "SegmentwiseGmmTrainer.hh"
#endif
#ifdef MODULE_MM_DT
#include <Mm/EbwDiscriminativeMixtureSetEstimator.hh>
#include <Mm/RpropDiscriminativeMixtureSetEstimator.hh>
#endif
#ifdef MODULE_ADAPT_MLLR
#include "FeatureShiftAdaptor.hh"
#endif

using namespace Speech;

Module_::Module_()
{
    Flow::Registry::Instance &registry = Flow::Registry::instance();
    registry.registerFilter<AlignmentNode>();
    registry.registerFilter<AlignmentDumpNode>();
    registry.registerFilter<AlignmentWithLinearSegmentationNode>();
    registry.registerDatatype<Flow::DataAdaptor<Alignment> >();

#ifdef MODULE_SPEECH_ALIGNMENT_FLOW_NODES
    registry.registerFilter<AlignmentAddWeightNode>();
    registry.registerFilter<AlignmentCombineItemsNode>();
    registry.registerFilter<AlignmentExpmNode>();
    registry.registerFilter<AlignmentFilterWeightsNode>();
    registry.registerFilter<AlignmentGammaCorrectionNode>();
    registry.registerFilter<AlignmentGeneratorNode>();
    registry.registerFilter<AlignmentMultiplyAlignmentsNode>();
    registry.registerFilter<AlignmentMultiplyWeightsNode>();
    registry.registerFilter<AlignmentRemoveEmissionScoreNode>();
    registry.registerFilter<AlignmentResetWeightsNode>();
    registry.registerFilter<AlignmentMapAlphabet>();
    registry.registerFilter<SetAlignmentWeightsByTiedStateAlignmentWeightsNode>();
    registry.registerDatatype<Flow::DataAdaptor<AlignmentGeneratorRef> >();
#endif

#ifdef MODULE_SPEECH_LATTICE_FLOW_NODES
    registry.registerFilter<AlignmentFromLatticeNode>();
    registry.registerFilter<LatticeExpmNode>();
    registry.registerFilter<LatticeNBestNode>();
    registry.registerFilter<LatticeReadNode>();
    registry.registerFilter<LatticeSemiringNode>();
    registry.registerFilter<LatticeSimpleModifyNode>();
    registry.registerFilter<LatticeWordPosteriorNode>();
    registry.registerFilter<ModelCombinationNode>();
    registry.registerFilter<LatticeArcAccumulatorNode>();
    registry.registerDatatype<Flow::DataAdaptor<ModelCombinationRef> >();
#endif




#ifdef MODULE_ADAPT_MLLR
    registry.registerFilter<FeatureShiftAdaptor>();
#endif
}


AligningFeatureExtractor *Module_::createAligningFeatureExtractor(
    const Core::Configuration& configuration, AlignedFeatureProcessor &featureProcessor) const
{
    return new Speech::AligningFeatureExtractor(configuration, featureProcessor);
}

MixtureSetTrainer* Module_::createMixtureSetTrainer(const Core::Configuration &configuration) const
{
    Speech::MixtureSetTrainer *result = 0;
    switch(Mm::Module_::paramEstimatorType(configuration)) {
    case Mm::Module_::maximumLikelihood:
	result = new MlMixtureSetTrainer(configuration);
	break;
#ifdef MODULE_SPEECH_DT
    case Mm::Module_::discriminative:
    case Mm::Module_::discriminativeWithISmoothing:
#endif
#if defined(MODULE_SPEECH_DT) || defined(MODULE_SPEECH_ADVANCED)
	result = createDiscriminativeMixtureSetTrainer(configuration);
	break;
#endif
    default:
	defect();
    }
    return result;
}

Speech::DataSource* Module_::createDataSource(const Core::Configuration &c, bool loadFromFile) const
{
#if 1
    return new Speech::DataSource(c, loadFromFile);
#endif
}

#ifdef MODULE_SPEECH_DT
DiscriminativeMixtureSetTrainer* Module_::createDiscriminativeMixtureSetTrainer(
    const Core::Configuration &configuration) const
{
    Speech::DiscriminativeMixtureSetTrainer *result = 0;

     switch(Mm::Module_::paramEstimatorType(configuration)) {
#ifdef MODULE_MM_DT
    case Mm::Module_::discriminative:
	result = new EbwDiscriminativeMixtureSetTrainer(configuration);
	break;
    case Mm::Module_::discriminativeWithISmoothing:
	result = new EbwDiscriminativeMixtureSetTrainerWithISmoothing(configuration);
	break;
#endif
    default:
	defect();
    }
    return result;
}


SegmentwiseGmmTrainer* Module_::createSegmentwiseGmmTrainer(
    const Core::Configuration &config) const
{
    switch (AbstractSegmentwiseTrainer::paramCriterion(config)) {
	// standard error-based training criteria without and with
	// I-smoothing, e.g. MPE
    case AbstractSegmentwiseTrainer::minimumError:
    case AbstractSegmentwiseTrainer::minimumErrorWithISmoothing:
	return new MinimumErrorSegmentwiseGmmTrainer(config);
	break;

    default:
	defect();
    }
    return 0;
}

LatticeRescorer* Module_::createDistanceLatticeRescorer(
    const Core::Configuration &config, Bliss::LexiconRef lexicon) const
{
    DistanceLatticeRescorer::DistanceType type =
	static_cast<DistanceLatticeRescorer::DistanceType>(
	    DistanceLatticeRescorer::paramDistanceType(config));
    DistanceLatticeRescorer::SpokenSource source =
	static_cast<DistanceLatticeRescorer::SpokenSource>(
	    DistanceLatticeRescorer::paramSpokenSource(config));
    LatticeRescorer *rescorer = 0;
    switch (type) {
    case DistanceLatticeRescorer::approximateWordAccuracy:
	switch (source) {
	case DistanceLatticeRescorer::orthography:
	    rescorer = new OrthographyApproximateWordAccuracyLatticeRescorer(config, lexicon);
	    break;
	case DistanceLatticeRescorer::archive:
	    rescorer = new ArchiveApproximateWordAccuracyLatticeRescorer(config, lexicon);
	    break;
	default:
	    defect();
	}
	break;
    case DistanceLatticeRescorer::approximatePhoneAccuracy:
	switch (source) {
	case DistanceLatticeRescorer::orthography:
	    rescorer = new OrthographyApproximatePhoneAccuracyLatticeRescorer(config, lexicon);
	    break;
	case DistanceLatticeRescorer::archive:
	    rescorer = new ArchiveApproximatePhoneAccuracyLatticeRescorer(config, lexicon);
	    break;
	default:
	    defect();
	}
	break;
    default:
	defect();
    }
    return rescorer;

}

#endif // MODULE_SPEECH_DT
