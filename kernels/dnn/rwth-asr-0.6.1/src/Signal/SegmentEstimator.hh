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
#ifndef _SIGNAL_SEGMENTESTIMATOR_HH
#define _SIGNAL_SEGMENTESTIMATOR_HH

#include <Core/Types.hh>
#include <vector>

namespace Signal
{

    // SegmentwiseEstimator
    ///////////////////////

    class SegmentwiseEstimator
    {
    public:
	typedef f32 _float;
    protected:
	s32 current_segment_begin_;
	s32 current_segment_end_;

    public:
	SegmentwiseEstimator() : current_segment_begin_(-1), current_segment_end_(-1) {};
	virtual ~SegmentwiseEstimator() {};

	bool setSegment(s32 segment_begin, s32 segment_end)
	    {
		current_segment_begin_ = segment_begin;
		current_segment_end_ = segment_end;
		return checkSegment();
	    };

	bool checkSegment() const
	    {
		s32 max_segment_value = getMaxSegmentValue();
		return ((current_segment_begin_ >= 0 && current_segment_begin_ <= max_segment_value) &&
			(current_segment_end_ >= 0 && current_segment_end_ <= max_segment_value) &&
			(current_segment_begin_ < current_segment_end_));
	    };

	virtual bool work(_float& estimation_error) = 0;
	virtual bool work(_float* estimation_error, std::vector<_float>* theta, _float* energy) = 0;
	virtual bool setSignal(const std::vector<_float>& y) = 0;
	virtual void setOrder(u8 order) = 0;
	virtual s32 getMaxSegmentValue() const = 0;
    };

    // DynamicProgramingSegmentEstimator
    ////////////////////////////////////

    class DynamicProgramingSegmentEstimator
    {
    public:
	typedef f32 _float;
	static const _float infinite;
    private:
	SegmentwiseEstimator* segmentwise_estimator_;

	_float* segment_error_;
	_float* error_matrix_;
	s32* back_pointer_;
	s32 row_size_;

	u8 max_nr_segment_;

	s32 step_;

	float search_interval_;
	s32 max_segment_value_;

	bool need_init_;
	bool need_realloc_;

	_float& segmentError__(s32 i, s32 j) { return segment_error_[i * row_size_ + j]; };
	_float& errorMatrix__(s32 i, s32 j) { return error_matrix_[i * row_size_ + j]; };
	s32& backPointer__(s32 i, s32 j) { return back_pointer_[i * row_size_ + j]; };

	bool init() {
	    if (segmentwise_estimator_ == 0) return false;
	    max_segment_value_ = (s32)((float)segmentwise_estimator_->getMaxSegmentValue() * search_interval_);
	    return !(need_init_ = (!precalculateSegmentError() || !buildBackPointerMatrix())); }

	bool reallocateBuffers();
	void freeBuffers();

	bool precalculateSegmentError();
	bool buildBackPointerMatrix();

    public:
	DynamicProgramingSegmentEstimator() :
	    segmentwise_estimator_(0), segment_error_(0), error_matrix_(0), back_pointer_(0), row_size_(0),
	    max_nr_segment_(0), step_(1), search_interval_(1.0), max_segment_value_(-1),
	    need_init_(true), need_realloc_(true) {}
	virtual ~DynamicProgramingSegmentEstimator() { freeBuffers(); }

	void setSegmentwiseEstimator(SegmentwiseEstimator* segmentwise_estimator)
	    { if (!segmentwise_estimator_ ||
		  segmentwise_estimator_->getMaxSegmentValue() != segmentwise_estimator->getMaxSegmentValue())
		need_realloc_ = true;
	    segmentwise_estimator_ = segmentwise_estimator; need_init_ = true; }

	void setMaxNumberOfSegments(u8 max_nr_segment){
	    if (max_nr_segment_ != max_nr_segment) { max_nr_segment_ = max_nr_segment; need_realloc_ = true;
	    }
	}

	void setStep(s32 step) { if (step_ != step) { step_ = step; need_init_ = true; } }

	bool setSearchInterval(float search_interval) {
	    if (search_interval <= 0.0 || search_interval > 1.0) return false;
	    if (search_interval_ != search_interval) {
		search_interval_ = search_interval; need_init_ = true;
	    }
	    return true;
	}

	bool work(std::vector<s32>& segments);
    };
}


#endif // _SIGNAL_SEGMENTESTIMATOR_HH
