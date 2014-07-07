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
#include "MixtureSetBuilder.hh"

using namespace Mm;

// MixtureSetBuilder
////////////////////

MixtureSetBuilder::MixtureSetBuilder(MixtureIndex nMixtures,
				     size_t nDensitiesPerMixture) :
    nMixtures_(nMixtures),
    nDensitiesPerMixture_(nDensitiesPerMixture)
{}


void MixtureSetBuilder::build(MixtureSet &toBuild, ComponentIndex dimension)
{
    toBuild.clear();
    toBuild.setDimension(dimension);

    for(MixtureIndex mix = 0; mix < nMixtures_; ++ mix) {
	Mixture *mixture = new Mixture;
	for(size_t dns = 0; dns < nDensitiesPerMixture_; ++ dns) {
	    if (densityIndex(mix, dns) >= toBuild.nDensities()) {
		if (meanIndex(mix, dns) >= toBuild.nMeans())
		    toBuild.addMean(meanIndex(mix, dns), new Mean(dimension));
		if (covarianceIndex(mix, dns) >= toBuild.nCovariances())
		    toBuild.addCovariance(covarianceIndex(mix, dns), new DiagonalCovariance(dimension));

		toBuild.addDensity(densityIndex(mix, dns), new GaussDensity(meanIndex(mix, dns),
									    covarianceIndex(mix, dns)));
	    }
	    mixture->addDensity(densityIndex(mix, dns));
	}
	toBuild.addMixture(mix, mixture);
    }
}
