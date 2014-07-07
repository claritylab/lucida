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
#include "Delimiter.hh"

using namespace Signal;

const Core::ParameterInt Delimiter::paramNumberOfIterations(
    "number-of-iterations",
    "number of iterations to calculate approximate silence/speech/silence segmentation",
    3, 0);

const Core::ParameterFloat Delimiter::paramPenalty(
    "penalty",
    "penalty for second order statistic, zero for no penalty",
    100.0, 0);

const Core::ParameterFloat Delimiter::paramMinimumSpeechProportion(
    "minimum-speech-proportion",
    "minimum proportion of speech in segment",
    0.7, 0.0, 1.0);

Delimiter::Delimiter(const Core::Configuration& c) :
    Core::Component(c),
    numberOfIterations_(paramNumberOfIterations(config)),
    f_(paramPenalty(config)),
    minimumSpeechProportion_(paramMinimumSpeechProportion(config)),
    statisticsChannel_(config, "statistics")
{}

f64 Delimiter::logLikelihood(u32 ib, u32 ie) const
{
    const f64 e = 0.5;
    f64 yy = Core::Type<f64>::max;
    f64 s1 = ib - 1 + nFeatures() - ie;
    f64 m1 = M_[ib - 1] + M_[nFeatures()] - M_[ie];
    f64 qq = Q_[ib - 1] + Q_[nFeatures()] - Q_[ie];
    f64 xx = qq / s1 - ((m1 / s1) * (m1 / s1)) / (f_ > 0 ? f_ : 1);
    f64 xsil = s1 * ::log(std::max(xx, e));
    f64 s2 = ie - ib + 1;
    f64 m2 = M_[ie] - M_[ib - 1];
    qq = Q_[ie] - Q_[ib - 1];
    xx = qq / s2 - ((m2 / s2) * (m2 / s2)) / (f_ > 0 ? f_ : 1);
    f64 xspe = s2 * ::log(std::max(xx, e));
    if (((m1 / s1) < (m2 / s2)) &&
	((ie - ib) >= (minimumSpeechProportion_ * nFeatures()))) yy = xsil + xspe;
    return yy;
}

/*****************************************************************************/
/*
 * taken from old standard system (aka delimit(*)):
 *
 *       START/STOP-DETECTION USING TWO GAUSSIAN MODELS,
 *       ONE FOR SPEECH AND ONE FOR SILENCE;
 *       BRIDLE,SEDGWICK, PP.656-659, ICASSP'77.
 *
 *       ITERATIVE APPROXIMATION BY VARYING IB AND IE SEPARATELY
 *
 *                                          N
 *       F =  N*log(var) =  N * log( 1/N * SUM [x(n)-xx]**2 )
 *                                         n=1
 *
 *       SUM [x(n)-xx]**2 =  SUM [x(n)**2 - 2*xx*x(n)     + xx**2]
 *                        =  SUM x(n)**2  - 2*xx*SUM x(n) + SUM xx**2
 *                        =  SUM x(n)**2  - 2*xx*N*xx     +  N *xx**2
 *                        =  SUM x(n)**2  - N*(xx**2)
 *
 *        silence       speech      silence
 *       [1,...,IB-1] [IB,...,IE] [IE+1,...,NI]
 */
/*****************************************************************************/
Delimiter::Delimitation Delimiter::getDelimitation() const
{
    if (minimumSpeechProportion_ == 1.0)
	return std::make_pair(0, nFeatures() - 1);

    f64 x;
    f64 x_min = Core::Type<f64>::max;
    u32 ie, ib, iEnd = nFeatures() - 1, iBeg = 1;
    for (u32 i = 0; i < numberOfIterations_; ++i) {
	for (ib = 2; ib < iEnd - 1; ++ib) {
	    x = logLikelihood(ib, iEnd);
	    if (x < x_min) {
		x_min = x;
		iBeg = ib;
	    }
	}
	for (ie = iBeg + 1; ie < nFeatures() - 1; ie++) {
	    x = logLikelihood(iBeg, ie);
	    if (x < x_min) {
		x_min = x;
		iEnd = ie;
	    }
	}
    }

    if (statisticsChannel_.isOpen()) {
	statisticsChannel_ << Core::XmlOpen("delimiter-statistics")
			   << Core::XmlFull("frames", nFeatures())
			   << Core::XmlFull("speech-begin", iBeg)
			   << Core::XmlFull("speech-end", iEnd)
			   << Core::XmlFull("score", x_min)
			   << Core::XmlClose("delimiter-statistics");
    }

    return std::make_pair(iBeg - 1, iEnd - 1);
}

/**
 *  from /u/loof/teaching/speech-image-WS0405/3.1.solution/Training.c
 */
Delimiter::Delimitation SietillDelimiter::getDelimitation() const
{
    if (minimumSpeechProportion_ == 1.0)
	return std::make_pair(0, nFeatures() - 1);

    f64 x_min = Core::Type<f64>::max;
    u32 iEnd = nFeatures(), iBeg = 1;
    for (u32 i = 0; i < numberOfIterations_; ++i) {
	x_min = Core::Type<f64>::max;
	for (u32 ib = 2; ib < nFeatures(); ++ib) {
	    if (ib != iEnd) {
		f64 x = logLikelihood(std::min(iEnd, ib), std::max(ib, iEnd));
		if (x < x_min) {
		    x_min = x;
		    iBeg = ib;
		}
	    }
	}
	for (u32 ie = 2; ie < nFeatures(); ++ ie) {
	    if (ie != iBeg) {
		f64 x = logLikelihood(std::min(iBeg, ie), std::max(ie, iBeg));
		if(x < x_min) {
		    x_min = x;
		    iEnd = ie;
		}
	    }
	}

	if (iEnd < iBeg) {
	    u32 tmp = iBeg;
	    iBeg = iEnd;
	    iEnd = tmp;
	}
    }

    if (statisticsChannel_.isOpen()) {
	statisticsChannel_ << Core::XmlOpen("delimiter-statistics")
			   << Core::XmlFull("frames", nFeatures())
			   << Core::XmlFull("speech-begin", iBeg)
			   << Core::XmlFull("speech-end", iEnd)
			   << Core::XmlFull("score", x_min)
			   << Core::XmlClose("delimiter-statistics");
    }

    return std::make_pair(iBeg - 1, iEnd - 1);
}
