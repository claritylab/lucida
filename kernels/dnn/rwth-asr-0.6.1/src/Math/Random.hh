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
#ifndef _MATH_RANDOM_HH
#define _MATH_RANDOM_HH

#include <Core/Parameter.hh>

namespace Math {

    class RandomVectorGenerator {
    public:
	typedef f32 DataType;
	enum Type { typeUniformIndependent, typeGaussIndependent };
    public:
	static const Core::Choice choiceType;
	static const Core::ParameterChoice paramType;

	static RandomVectorGenerator* create(Type type);
    public:
	RandomVectorGenerator() {}
	virtual ~RandomVectorGenerator() {}
	/** Fills output vector with random numbers
	 *  Override this function to implement differently distributed vector sequences.
	 */
	virtual void work(std::vector<DataType> &out) = 0;
    };

    /** Random vector generator
     *  -Vector components are independent, as far as the template parameter RandomNumberGenerator
     *   generates independent sequences.
     *  -Distribution of the vector components is determined by the distribution of template parameter
     *   RandomNumberGenerator
     */
    template<class RandomNumberGenerator>
    class IndependentRandomVectorGenerator : public RandomVectorGenerator {
    private:
	RandomNumberGenerator randomNumberGenerator_;
    public:
	IndependentRandomVectorGenerator() : randomNumberGenerator_(time(0) & 0xffff) {}
	virtual ~IndependentRandomVectorGenerator() {}

	virtual void work(std::vector<DataType> &out) {
	    for(std::vector<DataType>::iterator i = out.begin(); i != out.end(); ++ i)
		(*i) = randomNumberGenerator_.work();
	}
    };

} // namespace Math

#endif // _MATH_RANDOM_HH
