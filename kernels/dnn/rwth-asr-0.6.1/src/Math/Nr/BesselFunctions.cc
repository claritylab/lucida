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

    /** modified bessel function of first kind and zeroth order (i0) */

    f64 bessi0(f64 x) {

	f64 ax, ans, y;

	if ((ax=fabs(x)) < 3.75)
	    {
		y=x/3.75;
		y*=y;
		ans=1.0+y*(3.5156229+y*(3.0899424+y*(1.2067492
						     +y*(0.2659732+y*(0.360768e-1+y*0.45813e-2)))));
	    }
	else
	    {
		y=3.75/ax;
		ans=(exp(ax)/sqrt(ax))*
		    (0.39894228+y*(0.1328592e-1
				   +y*(0.225319e-2+y*
				       (-0.157565e-2+y*(0.916281e-2
							+y*(-0.2057706e-1+y*(0.2635537e-1+y*(-0.1647633e-1
											     +y*0.392377e-2))))))));
	    }

	return ans;
    };

} } // namespace Math::Nr
