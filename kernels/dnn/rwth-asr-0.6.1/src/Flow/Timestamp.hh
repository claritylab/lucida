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
#ifndef _FLOW_TIMESTAMP_HH
#define _FLOW_TIMESTAMP_HH

#include <Core/BinaryStream.hh>
#include <Core/XmlStream.hh>
#include <Core/Utility.hh>

#include "Data.hh"
#include "Datatype.hh"

namespace Flow {

    /**
     * Base class for Flow packets with time stamp information.
     *
     * The time stamp indicates the perdiod of time in the original
     * recording corresponding to the data packet.  Time values are
     * measured in seconds from the beginning of the recording.  start()
     * and end() are understood to denote the left-close, right-open
     * intervall [start, end).  This means that in a stream of
     * contiguous, non-overlapping packet, the end time of a packet is
     * equal to the start time of the next packet.
     */

    class Timestamp : public Data {
	typedef Timestamp Self;
    private:
	Time start_;
	Time end_;
    protected:
	Timestamp(const Datatype *dt);

	Core::XmlOpen xmlOpen() const;
    public:
	Timestamp(Time start = 0, Time end = 0);
	virtual ~Timestamp() {}

	static const Datatype *type();

	virtual Core::XmlWriter& dump(Core::XmlWriter &o) const;

	bool read(Core::BinaryInputStream &i);
	bool write(Core::BinaryOutputStream &o) const;

	void setStartTime(Time start) { start_ = start; }
	Time getStartTime() const { return start_; }
	Time startTime() const { return start_; }

	void setEndTime(Time end);
	Time getEndTime() const { return end_; }
	Time endTime() const { return end_; }

	void setTimestamp(const Timestamp &t);
	/** start_ = min(start_, t.start_) and end_ = max(end_, t.end_)*/
	void expandTimestamp(const Timestamp &t);

	void invalidateTimestamp() { start_ = Core::Type<Time>::max; end_ = Core::Type<Time>::min; }
	bool isValidTimestamp() { return start_ <= end_; }

	bool equalsToStartTime(Time t) const {
	    return Core::isAlmostEqualUlp(startTime(), t, timeToleranceUlp);
	}
	bool equalStartTime(const Timestamp &t) const {
	    return equalsToStartTime(t.startTime());
	}
	bool equalsToEndTime(Time t) const {
	    return Core::isAlmostEqualUlp(endTime(), t, timeToleranceUlp);
	}
	bool equalEndTime(const Timestamp &t) const {
	    return equalsToEndTime(t.endTime());
	}
	bool contains(Time t) const {
	    return equalsToStartTime(t) || equalsToEndTime(t) || (t > startTime() && t < endTime());
	}
	bool contains(const Timestamp &t) const {
	    return contains(t.startTime()) && contains(t.endTime());
	}
	bool overlap(const Timestamp &t) const {
	    return contains(t.startTime()) || contains(t.endTime());
	}
    };

} // namespace Flow

#endif // _FLOW_TIMESTAMP_HH
