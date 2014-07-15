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
// $Id: Statistics.hh 9621 2014-05-13 17:35:55Z golik $

#ifndef _CORE_STATISTICS_HH
#define _CORE_STATISTICS_HH

#include <sys/times.h>
#include <iostream>
#include <Core/Assertions.hh>
#include <Core/Choice.hh>
#include <Core/StringUtilities.hh>
#include <Core/Types.hh>
#include <Core/XmlStream.hh>

namespace Core {

    template <typename T>
    class Statistics {
    public:
	typedef T Value;
	typedef Statistics<T> Self;

    private:
	std::string name_;
	u32 nObs_;
	Value min_, max_, sum_;
	static const Value minInit, maxInit;

    public:
	void clear() {
	    nObs_ = 0;
	    max_ = maxInit;
	    min_ = minInit;
	    sum_ = Value();
	}

	void operator+= (Value v) {
	    ++nObs_;
	    if (max_ < v) max_ = v;
	    if (min_ > v) min_ = v;
	    sum_ += v;
	}

	Self &operator+= (const Self&);

	u32 nObservations() const { return nObs_; }
	Value minimum() const { return min_; }
	Value maximum() const { return max_; }
	f64   average() const { return f64(sum_) / f64(nObs_); }

	const std::string& name() const { return name_; }

	void write(XmlWriter&) const;

	Statistics(const std::string &n) : name_(n) { clear(); }
    };

    /**
     * A histogram statistics class for simple unsigned integers.
     * @param bins The count of bins that will be created to visualize the data
     * When printing the statistics, the number of occurences that fell into each
     * bin will be printed, together with the bin range.
     */
    class HistogramStatistics : public Statistics<u32> {
	typedef Statistics<Value> Precursor;
    public:
	HistogramStatistics(std::string name, u32 bins = 10, u32 arraySize=1024);
	void clear();
	void operator+= (Value v);
	void write(XmlWriter&) const;
	void writeArray(XmlWriter&) const;

	inline void addSample(unsigned int v) {
	    u32 binOffset = (u32)(v / scale_);
	    while(binOffset >= counts_.size())
	    {
		// Compress the table
		for(u32 a = 0; a < counts_.size()/2; ++a)
		    counts_[a] = counts_[a*2]+counts_[a*2+1];
		for(u32 a = counts_.size()/2; a < counts_.size(); ++a)
		    counts_[a] = 0;
		scale_ *= 2;
		binOffset = (u32)(v / scale_);
	    }

	    counts_[binOffset] += 1;
	}

	// Returns the value so that the given percentage of all samples have a lower value
	u32 quantile(f32 percent) const;

      private:
	std::vector<u32> counts_;
	u32 scale_, bins_;
    };

    template <class T>
    XmlWriter &operator<<(XmlWriter &os, const Statistics<T> &t) { t.write(os); return os; }
    inline XmlWriter &operator<<(XmlWriter &os, const HistogramStatistics &t) { t.write(os); return os; }

    class ChoiceStatistics {
    public:
	typedef ChoiceStatistics Self;
	typedef Choice::Value Value;
    private:
	std::string name_;
	const Choice &choice_;
	std::vector<u32> counts_;
	u32 totalCount() const;
    public:
	ChoiceStatistics(const std::string &name, const Choice&);

	void clear();

	void operator+= (Value v) {
	    require_(0 <= v && size_t(v) < counts_.size());
	    ++counts_[v];
	}

	Self &operator+= (const Self&);

	void write(XmlWriter&) const;
	friend XmlWriter &operator<<(XmlWriter&, const ChoiceStatistics&);
    };


    class Timer {
    private:
	static float clocksPerSecond;
	struct tms start_tms,   stop_tms;
	clock_t    start_clock, stop_clock;

    public:
	Timer();

	bool isRunning() const;
	void start();
	void stop();
	float user() const;
	float system() const;
	float elapsed() const;
	void write(XmlWriter &os) const;

	friend XmlWriter &operator<<(XmlWriter &os, const Timer &t);
    };

} // namespace Core

#endif // _CORE_STATISTICS_HH
