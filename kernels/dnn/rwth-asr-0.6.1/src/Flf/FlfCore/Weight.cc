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
#include "Weight.hh"

namespace Flf {

#ifdef MEM_DBG
    u32 Scores::nScores = 0;
    void * Scores::operator new(size_t size, size_t n) {
	++nScoresRef_;
	return ::malloc(size + n * sizeof(Score));
    }
    void Scores::operator delete(void *ptr) {
	--nScoresRef_;
	::free(ptr);
    }
#endif

    // -------------------------------------------------------------------------
    Score Scores::project(const ScoreList &scales) const {
	Score score = 0.0;
	const_iterator itScore = begin();
	for (ScoreList::const_iterator itScale = scales.begin(), endScale = scales.end();
	     itScale != endScale; ++itScale, ++itScore)
	    if (*itScale != 0.0) {
		if (*itScore != Core::Type<Score>::max)
		    score += (*itScale) * (*itScore);
		else
		    return Core::Type<Score>::max;
	    }
	// score += (*itScale) * (*itScore);
	return score;
    }
    // -------------------------------------------------------------------------

} // namespace Flf
