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
#ifndef _FLF_CORE_UTILITY_HH
#define _FLF_CORE_UTILITY_HH

#include <Core/Assertions.hh>
#include <Core/CompressedStream.hh>
#include <Core/TextStream.hh>
#include <Fsa/Types.hh>

#include "Lattice.hh"

namespace Flf {

    /*
      f(a, s * b) = a + s * b
    */
    inline Score add(Score a, Score b) {
	return (a == Semiring::Zero) || (b == Semiring::Zero) ?
	    Semiring::Zero : a + b;
    }

    /*
      f(a, b)^alpha = a^alpha + b^alpha, i.e.
    */
    inline Score logAdd(Score a, Score b) {
	if (b == Semiring::Zero) return a;
	if (a == Semiring::Zero) return b;
	return std::min(a, b) -
	    ::log1p(::exp(std::min(a, b) - std::max(a, b)));
    }

    /**
     * Collect over multiple values guaranteeing numerical stability;
     * semiring type must be tropical or log
     **/
    struct Collector {
	virtual ~Collector() {}
	virtual void reset() = 0;
	virtual void feed(f64) = 0;
	virtual f64 get() const = 0;
    };
    Collector * createCollector(Fsa::SemiringType semiringType);


    /**
     * Arc e with probability p and cost c:
     * v := c * p
     *
     * Expectation semiring:
     * (p1, v1) * (p2, v2) = (p1 * p2, p1 * v2 + p2 * v1)
     * (p1, v1) + (p2, v2) = (p1 + p2, v1 + v2)
     * One = (1, 0), Zero =(0, 0)
     * => (p1, v1) * ... * (pN, vN) = (p1 * ... * pN, (p1 * ... * pN) * (c1 + ... + cN))
     * => (p1, v1) + ... + (pN, vN) = (p1 + ... + pN, p1 * c1 + ... + cN * pN)
     * Generalized Fwd/Bwd. probability (aka expected cost aka risk)
     * for arc e with incoming (pIN, vIN) and outgoing (pOUT, vOUT)
     * risk(e) = vIN + v + vOUT = (pIN * p * pOUT) * (cIN + c + cOUT)
     *
     * Cost semiring:
     * (p1, c1) * (p2, c2) = (p1 * p2, c1 + c2)
     * (p1, c1) + (p2, c2) = (p1 + p2, (p1 * c1 + p2 * c2) / (p1 + p2))
     * One = (1, 0), Zero = (0, 0)
     * => (p1, c1) * ... * (pN, cN) = (p1 * ... * pN, (c1 + ... + cN))
     * => (p1, c1) + ... + (pN, cN) = (p1 + ... + pN, (p1 * c1 + ... + pN *cN) / (p1 + ... + pN))
     * Generalized Fwd/Bwd. probability (aka expected cost aka risk)
     * for arc e with incoming (pIN, cIN) and outgoing (pOUT, cOUT)
     * risk(e) = (pIN * p * pOUT) * (cIN + c + cOUT)
     *
     **/
    class CostCollector {
    private:
	typedef std::vector< std::pair<bool, f64> > CostList;
	CostList expectedCosts_;
    public:
	void reset();
	// score -> negative logarithm of probability
	// cost  -> any real value
	void feed(f64 score, f64 cost);
	// norm -> negative logarithm of probability
	f64 get(f64 norm) const;
	static CostCollector * create();
    };


    /**
     * Parse Textfile line by line and split at (sequences of) white spaces into columns;
     * empty lines and lines containing a comment only are discarded.
     *
     * Supports:
     * - Encoding -> see Core::TextInputStream
     * - Escaping -> if \<c> occurs, <c> is appended to the current column
     * - Quoting  -> Columns may be bracketed by "; escaping is active within quoting, comments are not
     * - Comments -> Text starting with # or ;; is discarded until the eol
     *
     * Example:
     * 1: a\ b\# "a b#" -> ["a b#", "a b#"]
     * 2: a\ b#  "a b#" -> ["a b"]
     * 3: "a b#" a\ b#  -> ["a b#", "a b"]
     *
     **/
    class TextFileParser {
    public:
	typedef std::vector<std::string> StringList;
    private:
	Core::TextInputStream tis_;
	u32 n_;
	std::vector<std::string> columns_;
    public:
	TextFileParser(const std::string &filename, const std::string &encoding = "utf-8");
	operator bool() { return bool(tis_); }
	u32 currentLineNumber() const { return n_; }
	const StringList & next();
    };

} // namespace Flf
#endif // _FLF_CORE_UTILITY_HH
