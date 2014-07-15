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
#include <stdio.h>
#include "SegmentEstimator.hh"

using namespace Signal;


const DynamicProgramingSegmentEstimator::_float DynamicProgramingSegmentEstimator::infinite = Core::Type<DynamicProgramingSegmentEstimator::_float>::max;


bool DynamicProgramingSegmentEstimator::precalculateSegmentError()
{
    if (segmentwise_estimator_ == 0)
	return false;

    for (s32 segment_begin = 0; segment_begin < max_segment_value_ + step_; segment_begin += step_)
	{
	    if (segment_begin > max_segment_value_)
		segment_begin = max_segment_value_;

	    for (s32 segment_end = segment_begin;
		 segment_end < max_segment_value_ + step_; segment_end += step_)
		{
		    if (segment_end > max_segment_value_)
			segment_end = max_segment_value_;

		    //std::cout << segment_begin << " " << segment_end << std::endl;
		    if (!segmentwise_estimator_->setSegment(segment_begin, segment_end))
			segmentError__(segment_begin, segment_end) = 0.0;
		    else if (!segmentwise_estimator_->work(segmentError__(segment_begin, segment_end)))
			{
			    //return false;
			    segmentError__(segment_begin, segment_end) = infinite;
			}
		    //else
		    //segmentError__(segment_begin, segment_end) /= (_float)((segment_end - segment_begin) * (segment_end - segment_begin));
		}
	}

    return true;
}


bool DynamicProgramingSegmentEstimator::buildBackPointerMatrix()
{
    // dynamic programming

    errorMatrix__(0, 0) = 0.0;
    for (int i = 1; i <=  max_segment_value_; i++)
	errorMatrix__(0, i) = infinite;

    for(s32 segment_end = step_; segment_end < max_segment_value_ + step_; segment_end += step_)
	{
	    //Force to cover the rightmost frequency
	    if (segment_end > max_segment_value_)
		segment_end = max_segment_value_;

	    for (u8 nr_segment = 1; nr_segment <= std::min((s32)max_nr_segment_, segment_end); nr_segment ++)
		{
		    _float minimum = infinite;
		    s32 back_pointer = segment_end;
		    for (s32 segment_begin = (nr_segment - 1) * step_;
			 segment_begin < segment_end + step_; segment_begin += step_)
			{
			    //Force to cover the rightmost frequency
			    if (segment_begin > segment_end)
				segment_begin = segment_end;

			    f64 auxiliaryFunction = errorMatrix__(nr_segment - 1, segment_begin) +
				segmentError__(segment_begin, segment_end);

			    /*
			      std::cout << nr_segment - 1 << " " << segment_begin <<  " " << segment_end << " "
			      << errorMatrix__(nr_segment - 1, segment_begin) << " "
			      << segmentError__(segment_begin, segment_end) << std::endl;
			    */
			    if (auxiliaryFunction < minimum)
				{
				    minimum = auxiliaryFunction;
				    back_pointer = segment_begin;
				}
			}
		    errorMatrix__(nr_segment, segment_end) = minimum;
		    backPointer__(nr_segment, segment_end) = back_pointer;
		}
	}


    /*
      for(s32 segment_end = step_; segment_end < max_segment_value_ + step_; segment_end += step_)
      {
      if (segment_end > max_segment_value_)
      segment_end = max_segment_value_;
      for(u8 nr_segment = 0; nr_segment <= max_nr_segment_; nr_segment ++)
      std::cout << errorMatrix__(nr_segment, segment_end) << "("
      << backPointer__(nr_segment, segment_end) << ") ";
      std::cout << std::endl;
      }
      std::cout << std::endl << std::endl;
      //::exit(0);
      */

    return true;
}

bool DynamicProgramingSegmentEstimator::work(std::vector<s32>& segments)
{
    if (need_realloc_ && !reallocateBuffers())
	return false;
    if (need_init_ && !init())
	return false;

    std::vector<s32> tmp_segment(max_nr_segment_ + 1);

    // traceback
    s32 traceback = max_segment_value_;
    for(s16 nr_segment = (s16)max_nr_segment_; nr_segment >= 0; nr_segment --)
	{
	    tmp_segment[nr_segment] = traceback;
	    traceback = backPointer__(nr_segment, traceback);
	}

    segments.reserve(max_nr_segment_ + 1);
    segments.clear();

    segments.push_back(tmp_segment[0]);
    for(u8 i = 1; i <= max_nr_segment_; i ++)
	{
	    if (tmp_segment[i] != segments.back())
		segments.push_back(tmp_segment[i]);
	}

    return true;
}

bool DynamicProgramingSegmentEstimator::reallocateBuffers()
{
    row_size_ = segmentwise_estimator_->getMaxSegmentValue() + 1;

    if (segment_error_)
	delete segment_error_;
    segment_error_ = new _float[row_size_ * row_size_];

    if (error_matrix_)
	delete error_matrix_;
    error_matrix_ = new _float[(max_nr_segment_ + 1) * row_size_];

    if (back_pointer_)
	delete back_pointer_;
    back_pointer_ = new s32[(max_nr_segment_ + 1) * row_size_];

    need_init_ = true;
    return !(need_realloc_ = !(segment_error_ && error_matrix_ && back_pointer_));
}

void DynamicProgramingSegmentEstimator::freeBuffers()
{
    if (segment_error_) { delete segment_error_; segment_error_ = 0; }
    if (error_matrix_) { delete error_matrix_; error_matrix_ = 0; }
    if (back_pointer_) { delete back_pointer_; back_pointer_ = 0; }

    row_size_ = 0;
}
