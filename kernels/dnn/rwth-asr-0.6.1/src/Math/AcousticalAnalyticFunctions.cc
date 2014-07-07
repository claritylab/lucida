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
#include "AcousticalAnalyticFunctions.hh"
#include <Core/Utility.hh>

using namespace Math;

//==============================================================================================
AnalyticFunction::Result EqualLoudnessPreemphasis::value(Argument f) const
{
    Argument omega = 2 * M_PI * f;
    Argument omegaSquare = omega * omega;
    Argument omegaFourth = omegaSquare * omegaSquare;
    Argument omegaSixth = omegaFourth * omegaSquare;
    return (omegaFourth * (omegaSquare + 56.8e6)) /
	    ((omegaSquare + 6.3e6) * (omegaSquare + 6.3e6) * (omegaSquare + 0.38e9) * (omegaSixth / 9.58e26 + 1));
}

//==============================================================================================
AnalyticFunction::Result EqualLoudnessPreemphasis4Khz::value(Argument f) const
{
    Argument omega = 2 * M_PI * f;
    Argument omegaSquare = omega * omega;
    Argument termFourth = omegaSquare / (omegaSquare + (Argument)6.3e6);
    return termFourth * termFourth * (omegaSquare + (Argument)56.8e6) /
	(omegaSquare + (Argument)0.38e9);
}

//==============================================================================================
AnalyticFunction::Result EqualLoudnessPreemphasis40dB::value(Argument f) const {
    Argument fSquare = f * f;
    Argument fSquareEps = fSquare + 1.6e5;

    Argument result = (fSquare / fSquareEps) * (fSquare / fSquareEps)
	    * ((fSquare + 1.44e6) / (fSquare + 9.61e6));

    return result;
}
