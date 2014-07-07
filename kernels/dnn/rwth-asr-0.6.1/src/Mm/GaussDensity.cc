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
#include "GaussDensity.hh"

using namespace Mm;

void GaussDensity::map(
    const std::vector<MeanIndex>& meanMap,
    const std::vector<CovarianceIndex> &covarianceMap)
{
    setMeanIndex(meanMap[meanIndex()]);
    setCovarianceIndex(covarianceMap[covarianceIndex()]);
}

bool Mean::write(std::ostream& o) const
{
	o << size();
	for (unsigned int i = 0; i < size(); i++)
		o << " " << this->at(i);
	o << std::endl;
	return o.good();
}
bool Mean::read(std::istream& i)
{
	clear();
	MeanType melem;
	ComponentIndex dim;
	i >> dim;
	while (0<dim) {
		i >> melem;
		push_back(melem);
		dim--;
	}
	return i.good();
}

bool DiagonalCovariance::write(std::ostream& o) const
{
	o << dimension();

	for (unsigned int i=0; i<dimension(); ++i) {
		o << " " << diagonal().at(i) << " " << weights().at(i);
	}
	o << std::endl;
	return o.good();
}
bool DiagonalCovariance::read(std::istream& i)
{
	ComponentIndex dim;
	i >> dim;
	VarianceType velem;
	std::vector<VarianceType> v;
	f64 welem;
	std::vector<f64> w;
	while (0<dim) {
		i >> velem >> welem;
		v.push_back(velem*welem);
		w.push_back(1.0);
		dim--;
	}
	(*this) = v;
	setFeatureWeights(w);
	return i.good();
}
