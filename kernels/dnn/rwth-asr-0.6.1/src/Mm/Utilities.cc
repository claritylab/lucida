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
#include "Utilities.hh"

using namespace Mm;


ProbabilityStatistics::ProbabilityStatistics(const std::string &name, u32 nBuckets) :
    name_(name),
    nBuckets_(nBuckets)
{
    counts_ = new Mm::Weight[nBuckets];
    sum_ = new Mm::Sum[nBuckets];
    for (int i=0; i<nBuckets_; ++i) {
	counts_[i] = 0.0;
	sum_[i] = 0.0;
    }
}

ProbabilityStatistics::~ProbabilityStatistics() {
    delete[] counts_;
    delete[] sum_;
}

void ProbabilityStatistics::operator+=(Mm::Weight probability)
{
    int bin = int(probability * nBuckets_);
    if (bin >= nBuckets_)
	bin = nBuckets_ - 1;
    else if (bin < 0)
	bin = 0;
    counts_[bin] += 1;
    sum_[bin] += probability;
}

void ProbabilityStatistics::writeXml(Core::XmlWriter &xml) const
{
    Mm::Sum totalCounts = 0.0;
    Mm::Sum totalSum = 0.0;
    for (int i=0; i<nBuckets_; ++i) {
	totalCounts += counts_[i];
	totalSum += sum_[i];
    }
    xml << Core::XmlOpen("statistic")
	+ Core::XmlAttribute("name", name_)
	+ Core::XmlAttribute("type", "probability-histogram")
	<< Core::XmlFull("count", totalCounts)
	<< Core::XmlFull("sum", totalSum);
    if (totalCounts > 0 && totalSum > 0) {
	xml << Core::XmlOpen("table");
	std::ostream &out = xml;
	out << "prob  rel.counts  rel.sum" << std::endl;
	for (int i=0; i<nBuckets_; ++i) {
	    out << Core::form("%1.3f %8.9f %8.9f",
			      (i+0.5)/nBuckets_, counts_[i]/totalCounts, sum_[i]/totalSum)
		<< std::endl;
	}
	xml << Core::XmlClose("table");
    }
    xml << Core::XmlClose("statistic");
}
