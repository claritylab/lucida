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
#ifndef _LM_INDEX_MAPPED_HH
#define _LM_INDEX_MAPPED_HH

#include <Core/Types.hh>
#include "LanguageModel.hh"

namespace Lm {

    class IndexMappedLm :
	public LanguageModel
    {
    protected:
	typedef u32 InternalClassIndex;
	static const InternalClassIndex invalidClass;
    private:
	InternalClassIndex nInternalClasses_;
	Bliss::TokenMap<InternalClassIndex> tokenMap_;
	std::vector<Token> classMap_; // index: InternalClassIndex
	std::vector<u32> classSizes_; // index: InternalClassIndex
	std::vector<Score> classEmissionScores_;

	void checkForUnmappedTokens();
	void checkForUnusedClasses();
	void initClassEmissionScores();

    protected:
	virtual const char *internalClassName(InternalClassIndex) const = 0;

	void initializeMapping(InternalClassIndex nInternalClasses);
	InternalClassIndex newClass();
	void mapToken(Token, InternalClassIndex idx);
	bool isTokenMapped(Token t) const {
	    return tokenMap_[t] != invalidClass;
	}
	void finalizeMapping();

	InternalClassIndex nInternalClasses() const {
	    return nInternalClasses_;
	}

	InternalClassIndex internalClassIndex(Token t) const {
	    return tokenMap_[t];
	}

	Token externalToken(InternalClassIndex i) const {
	    require_(0 <= i && i < nInternalClasses_);
	    return classMap_[i];
	}

	bool isClassUsed(InternalClassIndex i) const {
	    require_(0 <= i && i < nInternalClasses_);
	    return (classSizes_[i] > 0);
	}

	Score classEmissionScore(InternalClassIndex c) const {
	    require_(0 <= c && c < nInternalClasses_);
	    return classEmissionScores_[c];
	}

    public:
	IndexMappedLm(const Core::Configuration&, Bliss::LexiconRef);
	virtual ~IndexMappedLm();
    };

} // namespace Lm

#endif // _LM_INDEX_MAPPED_HH
