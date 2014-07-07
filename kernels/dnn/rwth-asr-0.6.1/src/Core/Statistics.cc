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
// $Id: Statistics.cc 9621 2014-05-13 17:35:55Z golik $

#include <time.h>
#include <unistd.h>
#include <algorithm>
#include <cerrno>
#include <numeric>
#include "Statistics.hh"
#include <Application.hh>

using namespace Core;


template <typename T>
Statistics<T> &Statistics<T>::operator+= (const Self &rhs) {
    nObs_ += rhs.nObs_;
    if (max_ < rhs.max_) max_ = rhs.max_;
    if (min_ > rhs.min_) min_ = rhs.min_;
    sum_ += rhs.sum_;
    return *this;
}

template <typename T>
void Statistics<T>::write(XmlWriter &os) const {
    os << Core::XmlOpen("statistic")
	+ Core::XmlAttribute("name", name_)
	+ Core::XmlAttribute("type", "scalar");
    if (nObs_) {
	os << XmlFull("min", min_)
	   << XmlFull("avg", average())
	   << XmlFull("max", max_);
    } else {
	os << "n/a";
    }
    os << Core::XmlClose("statistic");
}

// ===========================================================================
namespace Core {

    template <typename T> const typename Statistics<T>::Value Statistics<T>::minInit = Type<T>::max;
    template <typename T> const typename Statistics<T>::Value Statistics<T>::maxInit = Type<T>::min;

    template class Statistics<s32>;
    template class Statistics<u32>;
    template class Statistics<f32>;
    template class Statistics<f64>;

}

// ===========================================================================
// class ChoiceStatistics

ChoiceStatistics::ChoiceStatistics(const std::string &n, const Choice &choice) :
    name_(n),
    choice_(choice)
{
    std::vector<Value> values;
    choice_.getValues(values);
    Value max = Type<Value>::min;
    for (std::vector<Value>::const_iterator v = values.begin(); v != values.end(); ++v) {
	require(*v >= 0);
	if (max < *v) max = *v;
    }
    counts_.resize(max + 1);
    clear();
}

void ChoiceStatistics::clear() {
    std::fill(counts_.begin(), counts_.end(), 0);
}

ChoiceStatistics &ChoiceStatistics::operator+= (const Self &rhs) {
    require(&rhs.choice_ == &choice_);
    for (Value v = 0; size_t(v) < counts_.size(); ++v)
	counts_[v] += rhs.counts_[v];
    return *this;
}

u32 ChoiceStatistics::totalCount() const {
    return std::accumulate(counts_.begin(), counts_.end(), 0);
}

void ChoiceStatistics::write(XmlWriter &os) const {
    u32 total = totalCount();
    std::vector<Value> values;
    choice_.getValues(values);

    os << XmlOpen("statistic")
	+ XmlAttribute("name", name_)
	+ XmlAttribute("type", "histogram")
       << XmlFull("count", total);
    for (std::vector<Value>::const_iterator v = values.begin(); v != values.end(); ++v) {
	os << XmlOpen("count") + XmlAttribute("event", choice_[*v]);
	os << form("%d (%1.2f%%)",
		counts_[*v],
		100.0 * float(counts_[*v]) / float(total));
	os << XmlClose("count");
    }
    os << XmlClose("statistic");
}

XmlWriter& Core::operator<<(XmlWriter &os, const ChoiceStatistics &t) {
    t.write(os);
    return os;
}

// ===========================================================================
// class Timer

float Timer::clocksPerSecond = -1.0;

Timer::Timer() {
    if (clocksPerSecond < 0.0)
	clocksPerSecond = sysconf(_SC_CLK_TCK);
    start_clock = stop_clock = -1;
}

void Timer::start() {
    start_clock = times(&start_tms);
    stop_clock  = -1;
    ensure(isRunning());
}

bool Timer::isRunning() const {
    return (start_clock != -1) && (stop_clock  == -1);
}

void Timer::stop() {
    require(start_clock != -1);
    require(stop_clock  == -1);
    stop_clock = times(&stop_tms);
    if (stop_clock == -1) {
	// try again
	stop_clock = times(&stop_tms);
	if (stop_clock == -1)
	    Core::Application::us()->error("cannot get process time using times(): %s", strerror(errno));
    }
    // ensure(stop_clock != -1);
}

float Timer::user() const {
    require(start_clock != -1);
    if(stop_clock == -1)
    {
	clock_t current;
	struct tms current_tms;
	current = times(&current_tms);
	if (current == -1) {
	    // try again
	    current = times(&current_tms);
	    if (current == -1)
		Core::Application::us()->error("cannot get process time using times(): %s", strerror(errno));
	}
	return (float)(current_tms.tms_utime - start_tms.tms_utime) / clocksPerSecond;
    }else{
	return (float)(stop_tms.tms_utime - start_tms.tms_utime) / clocksPerSecond;
    }
}

float Timer::system() const {
    require(start_clock != -1);
    if(stop_clock == -1)
    {
	clock_t current;
	struct tms current_tms;
	current = times(&current_tms);
	if (current == -1) {
	    // try again
	    current = times(&current_tms);
	    if (current == -1)
		Core::Application::us()->error("cannot get process time using times(): %s", strerror(errno));
	}
	return (float)(current_tms.tms_stime - start_tms.tms_stime) / clocksPerSecond;
    }else{
	return (float)(stop_tms.tms_stime - start_tms.tms_stime) / clocksPerSecond;
    }
}

float Timer::elapsed() const {
    require(start_clock != -1);
    if(stop_clock == -1)
    {
	clock_t current;
	struct tms current_tms;;
	current = times(&current_tms);
	if (current == -1) {
	    // try again
	    current = times(&current_tms);
	    if (current == -1)
		Core::Application::us()->error("cannot get process time using times(): %s", strerror(errno));
	}
	return (float)(current - start_clock) / clocksPerSecond;
    }else{
	return (float)(stop_clock - start_clock) / clocksPerSecond;
    }
}

void Timer::write(XmlWriter &os) const {
    os << XmlOpen("timer")
       << XmlFull("user",    user())
       << XmlFull("system",  system())
       << XmlFull("elapsed", elapsed())
       << XmlClose("timer");
}

XmlWriter& Core::operator<< (XmlWriter &os, const Timer &t) {
    t.write(os);
    return os;
}

// Class HistogramStatistics

void Core::HistogramStatistics::clear() {
    Precursor::clear();
    counts_.assign(counts_.size(), 0);
}

Core::HistogramStatistics::HistogramStatistics(std::string name, u32 bins, u32 arraySize) :
    Precursor(name),
    counts_((arraySize / 2)*2, 0),
    scale_(1),
    bins_(bins)
{}

void Core::HistogramStatistics::operator+=(unsigned int v) {
    Precursor::operator+=(v);
    addSample(v);
}

u32 HistogramStatistics::quantile(f32 percent) const
{
    u32 totalCount = 0;
    for(u32 i = 0; i < counts_.size(); ++i)
	totalCount += counts_[i];
    if(!totalCount)
	return 0;
    s32 n = percent * totalCount;
    u32 bin = 0;
    for(bin = 0; bin < counts_.size(); ++bin)
    {
	n -= (s32)counts_[bin];
	if(n <= 0)
	    break;
    }
    return bin * scale_;
}

void Core::HistogramStatistics::writeArray(XmlWriter &os) const {
    os << Core::XmlOpen("statistic-array")
	+ Core::XmlAttribute("name", name())
	+ Core::XmlAttribute("type", "scalar");
    if (nObservations()) {
	for(u32 i = 0; i < counts_.size(); ++i)
	    if(counts_[i])
		os << i * scale_ << ":" << counts_[i] << " ";
    } else {
	os << "n/a";
    }
    os << Core::XmlClose("statistic");
}


void Core::HistogramStatistics::write(XmlWriter &os) const {
    os << Core::XmlOpen("statistic")
	+ Core::XmlAttribute("name", name())
	+ Core::XmlAttribute("type", "scalar");
    if (nObservations()) {
	os << XmlFull("min", minimum())
	   << XmlFull("avg", average())
	   << XmlFull("max", maximum());

	os << XmlOpen("histogram");
	u32 previousArrayEnd = 0;
	u32 remainingArrayWeight = 0;
	for(u32 a = 0; a < counts_.size(); ++a)
	    remainingArrayWeight += counts_[a];

	for(u32 currentBin = 0; currentBin < bins_; ++currentBin) {

	    u32 arrayStart = previousArrayEnd;
	    //Skip all zero array items
	    while(arrayStart < counts_.size() && counts_[arrayStart] == 0)
		++arrayStart;

	    if(arrayStart == counts_.size()) {
		break; //End reached
	    }

	    u32 targetArrayWeight = remainingArrayWeight / (bins_ - currentBin) + 1;
	    u32 usedArrayWeight = 0;

	    u32 arrayEnd = arrayStart;

	    while(arrayEnd < counts_.size() && (currentBin == bins_ - 1 || usedArrayWeight < targetArrayWeight)) {
		usedArrayWeight += counts_[arrayEnd];
		++arrayEnd;
	    }

	    u32 oldArrayEnd = arrayEnd;
	    u32 count = 0;
	    for(u32 a = arrayStart; a < oldArrayEnd; ++a) {
		count += counts_[a];
		if(counts_[a])
		    arrayEnd = a+1; //update arrayEnd so arrayStart and arrayEnd enclose the real range as exactly as possible
	    }

	    if(count == 0)
		break;

	    os << XmlOpen("bin") + XmlAttribute("start", arrayStart * scale_) + XmlAttribute("end", arrayEnd * scale_);
	    os << count;
	    os << XmlClose("bin");
	    previousArrayEnd = arrayEnd;
	    remainingArrayWeight -= usedArrayWeight;

	    if(!remainingArrayWeight)
		break;
	}
	verify(remainingArrayWeight == 0);

	os << XmlClose("histogram");
    } else {
	os << "n/a";
    }
    os << Core::XmlClose("statistic");
}
