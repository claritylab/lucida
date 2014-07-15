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
#ifndef _MM_TYPES_HH
#define _MM_TYPES_HH

#include "PointerVector.hh"
#include <string>
#include <Core/Types.hh>
#include <Core/Version.hh>
#include <Core/Hash.hh>

namespace Mm {

    typedef f32 Score;
    typedef f32 FeatureType;
    typedef f32 MeanType;
    typedef f32 VarianceType;
    typedef f64 Weight;
    typedef f64 IterationConstant;
    typedef u32 ComponentIndex;
    typedef u32 MixtureIndex;
    typedef MixtureIndex EmissionIndex;
    typedef u32 DensityIndex;
    typedef u32 DensityInMixture;
    typedef u32 MeanIndex;
    typedef u32 CovarianceIndex;
    typedef u32 Count;
    typedef f64 Sum;
    typedef std::string ClusterId;
    typedef f32 ScaleType;
    typedef u32 ExpertIndex;

    typedef std::vector<FeatureType> FeatureVector;

    typedef std::vector<ScaleType> Scales;

    const MixtureIndex invalidMixture = Core::Type<MixtureIndex>::max;
} //namespace Mm

#endif // _MM_TYPES_HH
