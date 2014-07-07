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
#include "VectorResize.hh"

namespace Signal {

    const Core::ParameterFloat paramVectorResizeNewSize
    ("new-size", "new size of vector in continuous (or discrete) units", 0, 0);

    const Core::ParameterInt paramVectorResizeNewDiscreteSize
    ("new-discrete-size", "new size of vector in discrete units", 0);

    const Core::ParameterFloat paramVectorResizeInitialValue
    ("initial-value", "appended value", 0);

    const Core::ParameterBool paramVectorResizeChangeFront
    ("change-front", "change front/back", false);

    const Core::ParameterBool paramVectorResizeRelativeChange
    ("relative-change", "specified size is relative", false);

} // namespace Signal
