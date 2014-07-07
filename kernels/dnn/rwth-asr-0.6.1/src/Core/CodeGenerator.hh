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
#ifndef _CORE_CODE_GENERATOR_HH
#define _CORE_CODE_GENERATOR_HH

#include "Component.hh"
#include "Types.hh"

namespace Core {

    class CodeGenerator : public Component {
    private:
	bool finalized_;
	size_t n_;
	u8 *code_;

    private:
	void grow(const size_t n);

    protected:
	bool append(const u8 *new_code, const size_t n);
	bool append(const std::vector<u8> &new_code);
	bool appendDWordOffset(unsigned int offset);
	void finalize();
	const u8* getCode() const { return code_; };

    public:
	CodeGenerator(const Configuration&);
	~CodeGenerator();
    };

} // namespace Core

#endif // _CORE_CODE_GENERATOR_HH
