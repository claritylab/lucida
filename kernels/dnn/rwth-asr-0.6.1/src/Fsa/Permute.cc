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
#include "Permute.hh"

namespace Fsa {

    template<class T> ConstAutomatonRef permute(ConstAutomatonRef f, u32 windowSize, u16 distortionLimit) {
	if (windowSize == 0) return f;
	return ConstAutomatonRef(new T(f, windowSize, distortionLimit, new NoProcessing));
    }

    ConstAutomatonRef permute(ConstAutomatonRef f, u32 windowSize, u16 distortionLimit) {
	return permute<IBMPermuteAutomaton<NoProcessing> >(f, windowSize, distortionLimit);
    }

    ConstAutomatonRef localPermute(ConstAutomatonRef f, u32 windowSize, u16 distortionLimit) {
	return permute<LocalPermuteAutomaton<NoProcessing> >(f, windowSize, distortionLimit);
    }

    ConstAutomatonRef inverseIBMPermute(ConstAutomatonRef f, u32 windowSize, u16 distortionLimit) {
	return permute<InverseIBMPermuteAutomaton<NoProcessing> >(f, windowSize, distortionLimit);
    }

    ConstAutomatonRef doubleLocalPermute(ConstAutomatonRef f, u32 windowSize, u16 distortionLimit) {
	return permute<DoubleLocalPermuteAutomaton<NoProcessing> >(f, windowSize, distortionLimit);
    }

    ConstAutomatonRef ITGPermute(ConstAutomatonRef f, u32 windowSize, u16 distortionLimit) {
	return permute<ITGPermuteAutomaton<NoProcessing> >(f, windowSize, distortionLimit);
    }

} // namespace Fsa
