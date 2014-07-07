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
#include "nr.h"

namespace Math { namespace Nr {

    /** four1: Fast Fourier transformation
     * @param data input and output buffer, Re and Im alternately
     * @param isign 1 transformation, -1 inverse transformation
     */

    void four1(std::vector<f32> &data, const s32 isign) {

	s32 n,mmax,m,j,istep,i;
	f64 wtemp,wr,wpr,wpi,wi,theta,tempr,tempi;

	int nn=data.size()/2;
	n=nn << 1;
	j=1;
	for (i=1;i<n;i+=2) {
	    if (j > i) {
		std::swap(data[j-1],data[i-1]);
		std::swap(data[j],data[i]);
	    }
	    m=nn;
	    while (m >= 2 && j > m) {
		j -= m;
		m >>= 1;
	    }
	    j += m;
	}
	mmax=2;
	while (n > mmax) {
	    istep=mmax << 1;
	    theta=isign*(6.28318530717959/mmax);
	    wtemp=sin(0.5*theta);
	    wpr = -2.0*wtemp*wtemp;
	    wpi=sin(theta);
	    wr=1.0;
	    wi=0.0;
	    for (m=1;m<mmax;m+=2) {
		for (i=m;i<=n;i+=istep) {
		    j=i+mmax;
		    tempr=wr*data[j-1]-wi*data[j];
		    tempi=wr*data[j]+wi*data[j-1];
		    data[j-1]=data[i-1]-tempr;
		    data[j]=data[i]-tempi;
		    data[i-1] += tempr;
		    data[i] += tempi;
		}
		wr=(wtemp=wr)*wpr-wi*wpi+wr;
		wi=wi*wpr+wtemp*wpi+wi;
	    }
	    mmax=istep;
	}
    }


    /** realft: Fourier transformation for real vectors.
     * @param data input and output buffer;
     *   remark: real part of last data moved/should be moved to imaginary part of first data
     * @param isign 1 transformation, -1 inverse transformation
     */
    void realft(std::vector<f32> &data, const int isign) {

	s32 i,i1,i2,i3,i4;
	f64 c1=0.5,c2,h1r,h1i,h2r,h2i,wr,wi,wpr,wpi,wtemp,theta;

	int n=data.size();
	theta=3.141592653589793238/f64(n>>1);
	if (isign == 1) {
	    c2 = -0.5;
	    four1(data,1);
	} else {
	    c2=0.5;
	    theta = -theta;
	}
	wtemp=sin(0.5*theta);
	wpr = -2.0*wtemp*wtemp;
	wpi=sin(theta);
	wr=1.0+wpr;
	wi=wpi;
	for (i=1;i<(n>>2);i++) {
	    i2=1+(i1=i+i);
	    i4=1+(i3=n-i1);
	    h1r=c1*(data[i1]+data[i3]);
	    h1i=c1*(data[i2]-data[i4]);
	    h2r= -c2*(data[i2]+data[i4]);
	    h2i=c2*(data[i1]-data[i3]);
	    data[i1]=h1r+wr*h2r-wi*h2i;
	    data[i2]=h1i+wr*h2i+wi*h2r;
	    data[i3]=h1r-wr*h2r+wi*h2i;
	    data[i4]= -h1i+wr*h2i+wi*h2r;
	    wr=(wtemp=wr)*wpr-wi*wpi+wr;
	    wi=wi*wpr+wtemp*wpi+wi;
	}
	if (isign == 1) {
	    data[0] = (h1r=data[0])+data[1];
	    data[1] = h1r-data[1];
	} else {
	    data[0]=c1*((h1r=data[0])+data[1]);
	    data[1]=c1*(h1r-data[1]);
	    four1(data,-1);
	}
    }

} } // namespace Math::Nr
