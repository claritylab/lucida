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
#ifndef _SEARCH_HISTOGRAM_HH
#define _SEARCH_HISTOGRAM_HH

#include "Types.hh"
#include <Core/Assertions.hh>

namespace Search
{

/**
 * histogram of scores for histogram pruning
 */
class Histogram {
private:
	typedef u32 Bin;
	typedef u32 Count;
	Score lower_, upper_, scale_;
	std::vector<Count> bins_;
	Bin bin(Score s) const {
		require_(lower_ <= s);
		s -= lower_;
		Bin result = Bin(s * scale_);
		if (result >= bins_.size()) result =  bins_.size() - 1;
		ensure_(0 <= result && result < bins_.size());
		return result;
	}
public:
	Histogram() : bins_(0) {}
	Histogram(Bin bins) : bins_(bins) {
		require(bins > 0);
	}
	void setBins(Bin bins) {
	    require(bins_.size() == 0);
	    require(bins > 0);
	    bins_.resize(bins);
	}
	void clear() {
		std::fill(bins_.begin(), bins_.end(), 0);
	}
	void setLimits(Score lower, Score upper) {
		require(lower < upper);
		lower_ = lower;
		upper_ = upper;
		scale_ = Score(bins_.size() - 1) / (upper_ - lower_);
	}
	void operator+= (Score s) {
		bins_[bin(s)] += 1;
	}
	Score quantile(Count nn) const {
		Bin b = 0;
		for(s32 n = nn; b < bins_.size(); ++b) { // n must be signed!
			n -= bins_[b];
			if (n <= 0) break;
		}
		verify(b <= bins_.size());
		Score result = Score(b) / scale_ + lower_;
		ensure(lower_ <= result);
		ensure(result < upper_ + 2.0 / scale_);
		return result;
	}
	Score localQuantile( Histogram::Count nn ) const {
	    Bin b = 0;
	    for(; b < bins_.size(); ++b ) {    // n must be signed!
		if( bins_[b] >= nn )
		break;
	    }
	    verify( b <= bins_.size() );
	    Score result = Score( b + 1 ) / scale_ + lower_;
	    ensure( lower_ <= result );
	    return result;
	}

	u32 total() const {
	    Count ret = 0;
	    for( Bin b = 0; b <= bins_.size(); ++b )
		ret += bins_[b];
	    return ret;
	}

	u32 offset( Score s ) const {
	    Bin ownBin = bin( s );
	    u32 ret = 0;
	    for( Bin b = 0; b <= ownBin; ++b )
		ret += bins_[b];
	    return ret;
	}
};

}

#endif /* _SEARCH_HISTOGRAM_HH */
