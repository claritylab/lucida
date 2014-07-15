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
#include "Timestamp.hh"


using namespace Flow;


Timestamp::Timestamp(const Datatype *dt) :
    Data(dt),
    start_(0),
    end_(0)
{}

Timestamp::Timestamp(Time start, Time end) :
    Data(type()),
    start_(start),
    end_(end)
{}

const Datatype* Timestamp::type()
{
    static DatatypeTemplate<Self> dt("timestamp");
    return &dt;
}

Core::XmlOpen Timestamp::xmlOpen() const
{
    return (Data::xmlOpen()
	    + Core::XmlAttribute("start", startTime())
	    + Core::XmlAttribute("end", endTime()));
}

Core::XmlWriter& Timestamp::dump(Core::XmlWriter &o) const
{
    o << Core::XmlEmpty(datatype()->name())
	+ Core::XmlAttribute("start", startTime())
	+ Core::XmlAttribute("end", endTime());
    return o;
}

bool Timestamp::read(Core::BinaryInputStream &i)
{
    i >> start_;
    i >> end_;
    return i.good();
}

bool Timestamp::write(Core::BinaryOutputStream &o) const
{
    o << start_;
    o << end_;
    return o.good();
}

void Timestamp::setEndTime(Time end)
{
    if (end >= start_)
	end_ = end;
    else
	end_ = start_;
}

void Timestamp::setTimestamp(const Timestamp &t)
{
    setStartTime(t.getStartTime());
    setEndTime(t.getEndTime());
}

void Timestamp::expandTimestamp(const Timestamp &t)
{
    if (isValidTimestamp()) {
	if (start_ > t.start_)
	    start_ = t.start_;
	if (end_ < t.end_)
	    end_ = t.end_;
    } else
	setTimestamp(t);
}
