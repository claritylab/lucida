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
#include "Random.hh"
#include <Nr/Random.hh>
#include <Core/Assertions.hh>

using namespace Math;


const Core::Choice RandomVectorGenerator::choiceType(
    "uniform-independent", typeUniformIndependent,
    "gauss-independent", typeGaussIndependent,
    Core::Choice::endMark());
const Core::ParameterChoice  RandomVectorGenerator::paramType(
    "type", &choiceType, "type of distribution", typeUniformIndependent);

RandomVectorGenerator *RandomVectorGenerator::create(Type type)
{
    switch(type) {
    case typeUniformIndependent: return new IndependentRandomVectorGenerator<Nr::Ran2>;
    case typeGaussIndependent: return new IndependentRandomVectorGenerator<Nr::Gasdev<Nr::Ran2> >;
    }
    defect();
    return 0;
}
