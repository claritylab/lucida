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
#ifndef _MATH_NR_RANDOM_HH_
#define _MATH_NR_RANDOM_HH_

#include <vector>
#include <cmath>

namespace Math { namespace Nr {

    /** Random number generator
     *  (Copied from Numerical Recipes in C++)
     *  Properties:
     *   -Simple and fast
     *   -Fails on some statistical tests
     *   -Period: 2^31 - 2 ~ 2.1e9
     */
    class Ran0 {
    private:
	static const int IM;
    private:
	int idum_;
    private:
	void next() {
	    const int IA=16807,IQ=127773,IR=2836;
	    int k=idum_/IQ;
	    idum_=IA*(idum_-k*IQ)-IR*k;
	    if (idum_ < 0) idum_ += IM;
	}
    public:
	Ran0(int idum);
	double work();
    };

    /** Random number generator
     *  (Copied from Numerical Recipes in C++)
     *  Properties:
     *   -Passes those statistical tests that Ran0 is known to fail if number of calls < 10^8
     */
    class Ran1 {
    private:
	static const int NTAB;
	static const int IM;
    private:
	int idum_;
	int iy_;
	std::vector<int> iv_;
    private:
	void next() {
	    const int IA=16807,IQ=127773,IR=2836;
	    int k=idum_/IQ;
	    idum_=IA*(idum_-k*IQ)-IR*k;
	    if (idum_ < 0) idum_ += IM;
	}
    public:
	/** Initializes the random generator by @param idum
	 *  @param idum is positive integer, unlike in "Numerical Recipes in C++".
	 */
	Ran1(int idum);
	double work();
    };

    /** Random number generator
     *  (Copied from Numerical Recipes in C++)
     *  Properties:
     *   -"Perfect" random number generator for floating point precision
     *   -Period: ~ 2e18
     */
    class Ran2 {
    private:
	static const int NTAB;
	static const int IM1;
    private:
	int idum_;
	int idum2_;
	int iy_;
	std::vector<int> iv_;
    private:
	void next() {
	    const int IA1=40014,IQ1=53668,IR1=12211;
	    int k=idum_/IQ1;
	    idum_=IA1*(idum_-k*IQ1)-k*IR1;
	    if (idum_ < 0) idum_ += IM1;
	}
	void next2() {
	    const int IA2=40692,IQ2=52774,IR2=3791;
	    const int IM2=2147483399;

	    int k=idum2_/IQ2;
	    idum2_=IA2*(idum2_-k*IQ2)-k*IR2;
	    if (idum2_ < 0) idum2_ += IM2;
	}
    public:
	/** Initializes the random generator by @param idum
	 *  @param idum is positive integer, unlike in "Numerical Recipes in C++".
	 */
	Ran2(int idum);
	double work();
    };

    /** Random number generator
     *  (Copied from Numerical Recipes in C++)
     *  Knuth suggestion, see also Knuth: The Art of Computer Programming, Vol 2.
     */
    class Ran3 {
    private:
	static const int MBIG;
	static const int MZ;
    private:
	int idum_;
	int inext_;
	int inextp_;
	std::vector<int> ma_;
    public:
	/** Initializes the random generator by @param idum
	 *  @param idum is positive integer, unlike in "Numerical Recipes in C++".
	 */
	Ran3(int idum);
	double work();
    };

    /** Random number generator with normal distribution
     *  Template parameter Ran is type of the uniform random generator.
     */
    template<class Ran>
    class Gasdev {
	Ran ran_;
	int iset_;
	double gset_;
    public:
	/** Initializes the random generator by @param idum
	 *  @param idum is positive integer, unlike in "Numerical Recipes in C++".
	 */
	Gasdev(int idum);
	double work();
    };

    template<class Ran>
    Gasdev<Ran>::Gasdev(int idum) :
	ran_(idum), iset_(0), gset_(0)
    {}

    template<class Ran>
    double Gasdev<Ran>::work()
    {
	double fac,rsq,v1,v2;

	if (iset_ == 0) {
	    do {
		v1=2.0*ran_.work()-1.0;
		v2=2.0*ran_.work()-1.0;
		rsq=v1*v1+v2*v2;
	    } while (rsq >= 1.0 || rsq == 0.0);
	    fac=std::sqrt(-2.0*std::log(rsq)/rsq);
	    gset_=v1*fac;
	    iset_=1;
	    return v2*fac;
	} else {
	    iset_=0;
	    return gset_;
	}
    }

} } // namespace Math::Nr

#endif // _MATH_NR_RANDOM_HH_
