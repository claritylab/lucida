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
#include "Registry.hh"
#include <Math/Complex.hh>

// predefined filter
#include "Aggregate.hh"
#include "Cache.hh"
#include "CorpusKeyMap.hh"
#include "Cutter.hh"
#include "Demo.hh"
#include "Dump.hh"
#include "Pipe.hh"
#include "Repeater.hh"
#include "SequenceFilter.hh"
#include "SimpleFunction.hh"
#include "Synchronization.hh"
#include "TypeConverter.hh"
#include "TypedAggregate.hh"
#include "VectorConcat.hh"
#include "VectorDemultiplex.hh"
#include "VectorInterleave.hh"
#include "VectorMalformed.hh"
#include "VectorMultiplicationNode.hh"
#include "VectorScalarFunction.hh"
#include "VectorSelect.hh"
#include "VectorSequence.hh"
#include "VectorSplit.hh"
#include "VectorSum.hh"
#include "VectorTextInput.hh"

// predefined datatypes
#include "Timestamp.hh"
#include "Vector.hh"
#include "DataAdaptor.hh"

using namespace Flow;

/*****************************************************************************/
Module_::Module_()
/*****************************************************************************/
{
    Registry::Instance &registry = Registry::instance();

    // register filters
    registry.registerFilter<RepeaterNode>();
    registry.registerFilter<DemoNode>();
    registry.registerFilter<DumpNode>();
    registry.registerFilter<VectorAbsValDumpNode<f32> >();
    registry.registerFilter<VectorAbsValDumpNode<f64> >();
    registry.registerFilter<CacheNode>();
    registry.registerFilter<CutterNode>();
    registry.registerFilter<PipeNode>();
    registry.registerFilter<VectorConcatNode<f32> >();
    registry.registerFilter<VectorInterleaveNode<f32> >();
    registry.registerFilter<VectorMultNode<f32> >();
    registry.registerFilter<VectorSelectNode<f32> >();
    registry.registerFilter<VectorSequenceNode<f32> >();
    registry.registerFilter<VectorSplitNode<f32> >();
    registry.registerFilter<VectorSumNode<f32> >();
    registry.registerFilter<AggregationNode>();
    registry.registerFilter<TypedDisaggregateNode<Vector<f32> > >();
    registry.registerFilter<TypedAggregateNode<Vector<f32> > >();
    registry.registerFilter<SimpleFunctionNode<VectorLogFunction<f32> > >();
    registry.registerFilter<SimpleFunctionNode<LogFunction<f32> > >();
    registry.registerFilter<SimpleFunctionNode<VectorLogPlusFunction<f32> > >();
    registry.registerFilter<SimpleFunctionNode<LogPlusFunction<f32> > >();
    registry.registerFilter<SimpleFunctionNode<VectorLnFunctionSave<f32> > >();
    registry.registerFilter<SimpleFunctionNode<VectorLnFunction<f32> > >();
    registry.registerFilter<SimpleFunctionNode<LnFunction<f32> > >();
    registry.registerFilter<SimpleFunctionNode<VectorExpFunction<f32> > >();
    registry.registerFilter<SimpleFunctionNode<ExpFunction<f32> > >();
    registry.registerFilter<SimpleFunctionNode<VectorPowerFunction<f32> > >();
    registry.registerFilter<SimpleFunctionNode<PowerFunction<f32> > >();
    registry.registerFilter<SimpleFunctionNode<VectorSqrtFunction<f32> > >();
    registry.registerFilter<SimpleFunctionNode<SqrtFunction<f32> > >();
    registry.registerFilter<SimpleFunctionNode<VectorCosFunction<f32> > >();
    registry.registerFilter<SimpleFunctionNode<CosFunction<f32> > >();
    registry.registerFilter<SimpleFunctionNode<VectorScalarMultiplicationFunction<f32> > >();
    registry.registerFilter<SimpleFunctionNode<MultiplicationFunction<f32> > >();
    registry.registerFilter<SimpleFunctionNode<VectorScalarAdditionFunction<f32> > >();
    registry.registerFilter<SimpleFunctionNode<AdditionFunction<f32> > >();
    registry.registerFilter<SimpleFunctionNode<VectorQuantizationFunction<f32> > >();
    registry.registerFilter<SimpleFunctionNode<VectorAbsoluteValueFunction<f32> > >();
    registry.registerFilter<SimpleFunctionNode<AbsoluteValueFunction<f32> > >();
    registry.registerFilter<SimpleFunctionNode<AdjacentDifference<f32> > >();
    // these 4 are used in Flow-based neural network forwarding implementation
    registry.registerFilter<SimpleFunctionNode<VectorLinearFunction<f32> > >();
    registry.registerFilter<SimpleFunctionNode<VectorSigmoidFunction<f32> > >();
    registry.registerFilter<SimpleFunctionNode<VectorSoftmaxFunction<f32> > >();
    registry.registerFilter<SimpleFunctionNode<VectorTanhFunction<f32> > >();

    registry.registerFilter<VectorScalarFunctionNode<NormFunction<f32> > >();
    registry.registerFilter<VectorScalarFunctionNode<EnergyFunction<f32> > >();

    registry.registerFilter<VectorMalformedNode<f32, CopyMalformedPolicy<f32> > >();
    registry.registerFilter<VectorMalformedNode<f32, DismissMalformedPolicy<f32> > >();
    registry.registerFilter<VectorMalformedNode<f32, FloorMalformedPolicy<f32> > >();
    registry.registerFilter<VectorMalformedNode<f32, KeepMalformedPolicy<f32> > >();

    registry.registerFilter<VectorTextInputNode<f32> >();

    registry.registerFilter<VectorDemultiplexNode<s8> >();
    registry.registerFilter<VectorDemultiplexNode<s16> >();

    registry.registerFilter<TypeConverterNode<VectorConverter<s8, f32 > > >();
    registry.registerFilter<TypeConverterNode<VectorConverter<s16, f32> > >();
    registry.registerFilter<TypeConverterNode<VectorConverter<f32, s16> > >();
    registry.registerFilter<TypeConverterNode<VectorConverter<f32, f64> > >();
    registry.registerFilter<TypeConverterNode<VectorConverter<f64, f32> > >();
    registry.registerFilter<TypeConverterNode<StringConverter<Float32> > >();
    registry.registerFilter<TypeConverterNode<StringConverter<Float64> > >();
    registry.registerFilter<TypeConverterNode<ScalarToStringConverter<Float32> > >();
    registry.registerFilter<TypeConverterNode<ScalarToStringConverter<Float64> > >();
    registry.registerFilter<TypeConverterNode<VectorToScalarConverter<f32, Float32> > >();
    registry.registerFilter<TypeConverterNode<ScalarToVectorConverter<Float32, f32> > >();
    registry.registerFilter<TypeConverterNode<VectorToScalarConverter<Vector<f32>, Vector<f32> > > >();

    registry.registerFilter<SynchronizationNode<Synchronization> >();
    registry.registerFilter<WeakSynchronizationNode<TimestampCopy> >();
    registry.registerFilter<CoprusKeyMapNode>();
    registry.registerFilter<SequenceFilterNode>();

    // register datatypes
    registry.registerDatatype<Timestamp>();
    registry.registerDatatype<Float32>();
    registry.registerDatatype<Float64>();
    registry.registerDatatype<Vector<s8> >();
    registry.registerDatatype<Vector<s16> >();
    registry.registerDatatype<Vector<f32> >();
    registry.registerDatatype<Vector<f64> >();
    registry.registerDatatype<Vector<std::complex<f32> > >();
    registry.registerDatatype<Vector<std::complex<f64> > >();
    registry.registerDatatype<Vector<Vector<f32> > >();
    registry.registerDatatype<Flow::Vector<bool> >();
    //registry.registerDatatype<new GatheredVector<f32> >();
    registry.registerDatatype<String>();
    registry.registerDatatype<Aggregate>();
    registry.registerDatatype<TypedAggregate<Vector<f32> > >();
}
