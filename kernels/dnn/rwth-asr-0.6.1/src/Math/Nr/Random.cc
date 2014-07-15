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
#include "Random.hh"
#include <cstdlib>
#include <Core/Assertions.hh>

using namespace Math::Nr;

//========================================================================================
const int Ran0::IM=2147483647;

Ran0::Ran0(int idum) :
    idum_(idum)
{}

double Ran0::work()
{
    const int MASK=123459876;
    const double AM=1.0/double(IM);
    double ans;

    idum_ ^= MASK;
    next();
    ans=AM*idum_;
    idum_ ^= MASK;
    return ans;
}

//========================================================================================
const int Ran1::NTAB = 32;
const int Ran1::IM=2147483647;

Ran1::Ran1(int idum) :
    idum_(0), iy_(0), iv_(NTAB)
{
    require(idum >= 0);

    idum_ = std::max(idum, 1);
    for (int j=NTAB+7;j>=0;j--) {
	next();
	if (j < NTAB) iv_[j] = idum_;
    }
    iy_=iv_[0];
}

double Ran1::work()
{
    const int NDIV=(1+(IM-1)/NTAB);
    const double EPS=3.0e-16,AM=double(1.0)/double(IM),RNMX=(1.0-EPS);
    int j;
    double temp;

    next();
    j=iy_/NDIV;
    iy_=iv_[j];
    iv_[j] = idum_;
    if ((temp=AM*iy_) > RNMX) return RNMX;
    else return temp;
}

//========================================================================================
const int Ran2::NTAB = 32;
const int Ran2::IM1=2147483563;

Ran2::Ran2(int idum) :
    idum_(0), idum2_(123456789), iy_(0), iv_(NTAB)
{
    require(idum >= 0);
    idum_ = std::max(idum, 1);
    idum2_=idum_;
    for (int j=NTAB+7;j>=0;j--) {
	next();
	if (j < NTAB) iv_[j] = idum_;
    }
    iy_=iv_[0];
}

double Ran2::work()
{
    const int IMM1=IM1-1, NDIV=1+IMM1/NTAB;
    const double EPS=3.0e-16,RNMX=1.0-EPS,AM=1.0/double(IM1);
    int j;
    double temp;

    next();
    next2();
    j=iy_/NDIV;
    iy_=iv_[j]-idum2_;
    iv_[j] = idum_;
    if (iy_ < 1) iy_ += IMM1;
    if ((temp=AM*iy_) > RNMX) return RNMX;
    else return temp;
}

//========================================================================================
const int Ran3::MBIG = 1000000000;
const int Ran3::MZ = 0;

Ran3::Ran3(int idum) :
    idum_(idum), inext_(0), inextp_(0), ma_(56)
{
    const int MSEED=161803398;
    int i,ii,k,mj,mk;

    mj=labs(MSEED-labs(idum_));
    mj %= MBIG;
    ma_[55]=mj;
    mk=1;
    for (i=1;i<=54;i++) {
	ii=(21*i) % 55;
	ma_[ii]=mk;
	mk=mj-mk;
	if (mk < int(MZ)) mk += MBIG;
	mj=ma_[ii];
    }
    for (k=0;k<4;k++)
	for (i=1;i<=55;i++) {
	    ma_[i] -= ma_[1+(i+30) % 55];
	    if (ma_[i] < int(MZ)) ma_[i] += MBIG;
	}
    inext_=0;
    inextp_=31;
    idum_=1;
}

double Ran3::work()
{
    const double FAC=(1.0/MBIG);

    if (++inext_ == 56) inext_=1;
    if (++inextp_ == 56) inextp_=1;
    int mj=ma_[inext_]-ma_[inextp_];
    if (mj < int(MZ)) mj += MBIG;
    ma_[inext_]=mj;
    return mj*FAC;
}
