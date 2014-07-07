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
#include <Modules.hh>
#include "Module.hh"
#include <Flow/Registry.hh>
#include "ComplexVectorFunction.hh"
#include "CosineTransform.hh"
#include "DcDetection.hh"
#include "Delay.hh"
#include "FastFourierTransform.hh"
#include "Filterbank.hh"
#include "FramePrediction.hh"
#include "MatrixMult.hh"
#include "Mrasta.hh"
#include "FastMatrixMult.hh"
#include "Normalization.hh"
#include "Preemphasis.hh"
#include "Regression.hh"
#include "RepeatingFramePrediction.hh"
#include "SegmentClustering.hh"
#include "VectorNormalization.hh"
#include "VectorResize.hh"
#include "VectorSequenceAggregation.hh"
#include "VectorSequenceConcatenation.hh"
#include "Window.hh"
#ifdef MODULE_SIGNAL_PLP
#include "ArEstimator.hh"
#include "AutoregressionToCepstrum.hh"
#include "AutoregressionToSpectrum.hh"
#include "VectorTransform.hh"
#endif
#ifdef MODULE_SIGNAL_VOICEDNESS
#include "CrossCorrelation.hh"
#include "PeakDetection.hh"
#endif
#ifdef MODULE_SIGNAL_VTLN
#include "BayesClassification.hh"
#endif
#ifdef MODULE_SIGNAL_GAMMATONE
#include "GammaTone.hh"
#include "SpectralIntegration.hh"
#include "TemporalIntegration.hh"
#endif

/*****************************************************************************/
Signal::Module_::Module_()
{
    Flow::Registry::Instance &registry = Flow::Registry::instance();
    registry.registerFilter<CosineTransformNode>();
    registry.registerFilter<ComplexVectorFunctionNode<alternatingComplexVectorAmplitude<f32> > >();
    registry.registerFilter<ComplexVectorFunctionNode<alternatingComplexVectorImaginaryPart<f32> > >();
    registry.registerFilter<ComplexVectorFunctionNode<alternatingComplexVectorPhase<f32> > >();
    registry.registerFilter<ComplexVectorFunctionNode<alternatingComplexVectorRealPart<f32> > >();
    registry.registerFilter<ComplexVectorFunctionNode<vectorToAlternatingComplexVector<f32> > >();
    registry.registerFilter<ComplexVectorFunctionNode<alternatingComplexVectorToComplexVector<f32> > >();
    registry.registerFilter<ComplexVectorFunctionNode<complexVectorToAlternatingComplexVector<f32> > >();
    registry.registerFilter<DcDetectionNode>();
    registry.registerFilter<DelayNode>();
    registry.registerFilter<FastFourierTransformNode<RealFastFourierTransform> >();
    registry.registerFilter<FastFourierTransformNode<RealInverseFastFourierTransform> >();
    registry.registerFilter<FastFourierTransformNode<ComplexFastFourierTransform> >();
    registry.registerFilter<FastFourierTransformNode<ComplexInverseFastFourierTransform> >();
    registry.registerFilter<FilterBankNode>();
    registry.registerFilter<FramePredictionNode<RepeatingFramePrediction> >();
    registry.registerFilter<MatrixMultiplicationNode<f32> >();
    registry.registerFilter<MatrixMultiplicationNode<f64> >();
    registry.registerFilter<MrastaFilteringNode>();
    registry.registerFilter<FastMatrixMultiplicationNode<f32> >();
//    registry.registerFilter<FastMatrixMultiplicationNode<f64> >();
    registry.registerFilter<NormalizationNode>();
    registry.registerFilter<PreemphasisNode>();
    registry.registerFilter<RegressionNode>();
    registry.registerFilter<SegmentClusteringNode<CorrFullCovMonoGaussianModel> >();
    registry.registerFilter<VectorNormalizationNode<AmplitudeSpectrumEnergyVectorNormalization<f32> > >();
    registry.registerFilter<VectorNormalizationNode<EnergyVectorNormalization<f32> > >();
    registry.registerFilter<VectorNormalizationNode<MaximumVectorNormalization<f32> > >();
    registry.registerFilter<VectorNormalizationNode<MeanEnergyVectorNormalization<f32> > >();
    registry.registerFilter<VectorNormalizationNode<MeanVectorNormalization<f32> > >();
    registry.registerFilter<VectorNormalizationNode<VarianceVectorNormalization<f32> > >();
    registry.registerFilter<VectorResizeNode<f32> >();
    registry.registerFilter<VectorSequenceAggregation<f32> >();
    registry.registerFilter<VectorSequenceConcatenation<f32> >();
    registry.registerFilter<WindowNode>();

#ifdef MODULE_SIGNAL_VOICEDNESS
    registry.registerFilter<CrossCorrelationNode>();
    registry.registerFilter<PeakDetectionNode>();
#endif

#ifdef MODULE_SIGNAL_VTLN
    registry.registerFilter<BayesClassificationNode>();
    registry.registerFilter<BayesClassificationScoreNode>();
#endif

#ifdef MODULE_SIGNAL_PLP
    registry.registerFilter<AutocorrelationToAutoregressionNode>();
    registry.registerFilter<AutoregressionToCepstrumNode>();
    registry.registerFilter<AutoregressionToSpectrumNode>();
    registry.registerFilter<ContinuousVectorTransformNode>();
    registry.registerDatatype<AutoregressiveCoefficients>();
#endif


#ifdef MODULE_SIGNAL_GAMMATONE
    registry.registerFilter<GammaToneNode>();
    registry.registerFilter<SpectralIntegrationNode>();
    registry.registerFilter<TemporalIntegrationNode>();
#endif
}
