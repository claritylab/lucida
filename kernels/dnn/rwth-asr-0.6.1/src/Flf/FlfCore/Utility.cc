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
#include <Core/Application.hh>

#include "Utility.hh"

namespace Flf {

    // -------------------------------------------------------------------------
    struct TropicalCollector : public Collector {
	f64 score;
	TropicalCollector() : score(Semiring::Zero) {}
	virtual ~TropicalCollector() {}
	virtual void reset()
	    { score = Semiring::Zero; }
	virtual void feed(f64 a)
	    { if (a < score) score = a; }
	virtual f64 get() const
	    { return score; }
    };

    struct LogCollector : public Collector {
	typedef std::vector<f64> f64List;
	f64 min;
	f64List scores;
	LogCollector() : min(Semiring::Zero) {}
	virtual ~LogCollector() {}
	virtual void reset() {
	    min = Semiring::Zero;
	    scores.clear();
	}
	virtual void feed(f64 a) {
	    if (a != Semiring::Zero) {
		if (a < min) { scores.push_back(min); min = a; }
		else scores.push_back(a);
	    }
	}
	virtual f64 get() const {
	    if (scores.size() <= 1) return min;
	    f64 sum = 0.0;
	    for (f64List::const_iterator it = scores.begin() + 1;
		 it != scores.end(); ++it) sum += ::exp(min - *it);
	    return min - ::log1p(sum);
	}
    };

    Collector * createCollector(Fsa::SemiringType semiringType) {
	switch (semiringType) {
	case Fsa::SemiringTypeTropical:
	    return new TropicalCollector;
	case Fsa::SemiringTypeLog:
	    return new LogCollector;
	default:
	    defect();
	    return 0;
	}
    }
    // -------------------------------------------------------------------------


    // -------------------------------------------------------------------------
	void CostCollector::reset() {
		expectedCosts_.clear();
	}

	void CostCollector::feed(f64 score, f64 cost) {
		if (cost != 0.0)
			expectedCosts_.push_back(
				(cost >= 0.0) ?
				std::make_pair(true,  ::log( cost) - score) :
				std::make_pair(false, ::log(-cost) - score));
	}

	f64 CostCollector::get(f64 norm) const {
		f64 cost = 0.0;
		for (CostList::const_iterator it = expectedCosts_.begin(), end = expectedCosts_.end(); it != end; ++it)
			cost += (it->first ? ::exp(it->second + norm) : -::exp(it->second + norm));
		return cost;
	}

    CostCollector * CostCollector::create() {
		return new CostCollector;
    }
    // -------------------------------------------------------------------------


    // -------------------------------------------------------------------------
    TextFileParser::TextFileParser(const std::string &filename, const std::string &encoding) {
	tis_.adopt(new Core::CompressedInputStream(filename));
	tis_.setEncoding(encoding);
	n_ = 0;
    }

    const TextFileParser::StringList & TextFileParser::next() {
	columns_.clear();
	while (tis_ && columns_.empty()) {
	    n_++;
	    std::string line;
	    std::getline(tis_, line);
	    bool isEscaped = false;
	    const char *comment = 0;
	    for (const char *c = line.c_str();;) {
		for (; (*c != '\0') && ::isspace(*c); ++c);
		if (*c == '\0') break;
		columns_.push_back(std::string());
		if (*c == '"') {
		    for (++c ;; ++c) {
			if (*c == '\0') {
			    Core::Application::us()->criticalError(
				"TextFileParser:%d Missing closing \"", n_);
			    break;
			} else if (isEscaped) {
			    columns_.back().push_back(*c);
			    isEscaped = false;
			} else if (*c == '"') {
			    break;
			} else if (*c == '\\') {
			    isEscaped = true;
			} else {
			    columns_.back().push_back(*c);
			}
		    }
		    ++c;
		} else {
		    bool isBreak = false;
		    for (;; ++c) {
			if (*c == '\0') {
			    isBreak = true;
			    break;
			} else if (isEscaped) {
			    columns_.back().push_back(*c);
			    isEscaped = false;
			} else if (::isspace(*c)) {
			    break;
			} else if (*c == '\\') {
			    isEscaped = true;
			} else if ((*c == '#') || ((*c == ';') && (*(c + 1) == ';'))) {
			    comment = c;
			    if (columns_.back().empty()) columns_.pop_back();
			    isBreak = true;
			    break;
			} else {
			    columns_.back().push_back(*c);
			}
		    }
		    if (isBreak) break;
		}
	    }
	    if (isEscaped)
		columns_.back().push_back('\\');
	}
	return columns_;
    }
    // -------------------------------------------------------------------------

} // namespace Flf
