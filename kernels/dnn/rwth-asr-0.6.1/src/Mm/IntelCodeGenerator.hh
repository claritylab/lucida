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
#ifndef _MM_INTEL_CODE_GENERATOR_HH
#define _MM_INTEL_CODE_GENERATOR_HH


#include <Core/CodeGenerator.hh>


namespace Mm {

    class IntelMMXL2NormCodeGenerator : public Core::CodeGenerator {
    private:
	bool appendProlog();
	bool appendClearAccumulator();
	bool appendFetchA(int block, u8 offset);
	bool appendFetchB(int block, u8 offset);
	bool appendDuplicate(int block);
	bool appendSubtract(int block);
	bool appendPrepareForUnpack(int block);
	bool appendUnpackAndAdd(int block);
	bool appendAccumulate(int block);
	bool appendEpilog();

    public:
	IntelMMXL2NormCodeGenerator(const Core::Configuration &c, size_t d);
	unsigned int run(const u8 *v1, const u8 *v2) const {
	    return ((unsigned int (*)(const u8*, const u8*))getCode())(v1, v2);
	}
    };

    class IntelMMXResetCodeGenerator : public Core::CodeGenerator {
    public:
	IntelMMXResetCodeGenerator(const Core::Configuration &c);
	void run() const { ((void (*)(void))getCode())(); }
    };

} // namespace Mm


#endif // _MM_INTEL_CODE_GENERATOR_HH
