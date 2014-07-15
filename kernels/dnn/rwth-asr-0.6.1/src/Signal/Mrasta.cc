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
#include "Mrasta.hh"

using namespace Signal;

// filter widths: 8 .. 130 ms (see original paper)
const float MrastaFiltering::sigma_[maxFilter] = { 0.8, 1.2, 1.8, 2.7, 4.0, 6.0, 9.0, 13.0 };

inline float MrastaFiltering::sigma(int index) const {
    if ((index < 0) || (index > (int)maxFilter)) {
	return (Value) 0;
    } else {
	return sigma_[index];
    }
}

MrastaFiltering::MrastaFiltering() :
    needInit_(true),
    nFeatures_(0),
    nFrames_(0),
    nDerivatives_(0),
    nFilters_(0)
{}

MrastaFiltering::~MrastaFiltering() {}

/** Initialize MrastaFiltering.
 *  Function will be called internally before MRASTA filtering.
 *  @return true if initialization was successful
 */
bool MrastaFiltering::init() {
    // init/check all the member values
    require(nFrames_ > 0);
    require(nFeatures_ > 0);
    require(nFilters_ > 0);
    require(nFilters_ < maxFilter);

    // Initialize the Gaussian filters G1 and G2 as matrices
    initGaussianFilters(nFilters_, nFrames_);

    // allocate memory for a single input band
    inputBuffer_.resize(nFrames_);
    fill(inputBuffer_.begin(), inputBuffer_.end(), 0);

    // allocate memory for the filtered band
    outputBuffer_.resize(2 * nFilters_);
    fill(outputBuffer_.begin(), outputBuffer_.end(), 0);

    needInit_ = false;
    return true;
}


/** Initialize the Gaussian filters.
 *
 *  u = (cMax - 1) / 2 => 50
 *  c = 0..cMax
 *  o[i] = {0.8, 1.2, 1.8, 2.7, 4.0, 6.0, 9.0, 13.0}
 *
 *  G0 = exp( -(c-u)^2 / (2 o[i]^2 ) )
 *  G1 = -(c-u) / o[i]^2 * G0
 *  G2 = ( (c-u)^2 / s[i]^4 - 1 / o[i]^2 ) * G0
 */
void MrastaFiltering::initGaussianFilters(size_t nFilter, size_t nFrames) {
    // if necessary, resize the filter matrices
    resizeFilters(nFilter, nFrames);

    // fill the filter with the corresponding Gaussian values
    for (int i = 0; i < (int) nFilter; i++) {
	for (int j = 0; j < (int) nFrames; j++) {
	    // Gaussian filter (G0)
	    getG0()[i][j] = exp (
			    -(Value) ( (j - (int) (nFrames / 2) ) * (j - (int) (nFrames / 2) )
					    / (2.0 * sigma(i) * sigma(i)) )
	    );
	    // first (time) derivative of the Gaussian filter (G1)
	    getG1()[i][j] = (Value) -( (j - (int) (nFrames / 2) )
			    / (sigma(i) * sigma(i)) )
			    * getG0()[i][j];

	    // second derivative of the Gaussian filter (G2)
	    getG2()[i][j] = ( (Value) (j - (int) (nFrames / 2) ) * (j - (int) (nFrames / 2) )
			    / (sigma(i) * sigma(i) * sigma(i) * sigma(i))
			    - 1.0 / (sigma(i) * sigma(i) ) )
			    * getG0()[i][j];
	}
	// divide each Gaussian filter (G1, G2) by absolute maximum (over frames)
	normalizeFilterResponse(getG1(), i);
	normalizeFilterResponse(getG2(), i);
    }
}

inline void MrastaFiltering::resizeFilters(size_t nFilter, size_t nFrames) {
    if (filters_.size() != 3)
	filters_.resize(3);
    for (Math::Vector<Math::Matrix<Value> >::iterator it = filters_.begin(); it != filters_.end(); it++)
	if ( (it->nRows() != nFilter) || (it->nColumns() != nFrames) )
	    it->resize(nFilter, nFrames);
}

inline void MrastaFiltering::normalizeFilterResponse(Math::Matrix<Value>& G, int filter_num) {
    Math::Vector<Value> vector = G.row(filter_num);
    // max(|v|) = max(-min(v), max(v))
    Value maxAbsVal = std::max( - (Value) *std::min_element(vector.begin(), vector.end() ),
					  *std::max_element(vector.begin(), vector.end()));
    // normalize vector and write back into the matrix
    std::transform(vector.begin(), vector.end(), vector.begin(),
		    std::bind2nd(std::multiplies<Value>(), (Value) 1 / maxAbsVal));
    G.setRow(filter_num, vector);
}

void MrastaFiltering::getBand(size_t band, std::vector<Value> &in, std::vector<Value> &out) {
    //Copy each frame of a band.
    for (Flow::Vector<Value>::iterator it = out.begin(), itv = (in.begin() + band);
	 it != out.end();
	 it++, itv += nFeatures_)
	    *it = *itv;
}

void MrastaFiltering::setBand(size_t band, std::vector<Value> &in, std::vector<Value> &out) {
    //Copy value to the end of the output vector.
    for (Flow::Vector<Value>::iterator it = in.begin(), itv = (out.begin() + band);
	 it != in.end();
	 it++, itv += nFeatures_)
	    *itv = *it;
}

void MrastaFiltering::filterEnergyBand(Math::Vector<Value> &in, Math::Vector<Value> &response) {
    // get the filter response
    for (size_t filter = 0; filter < nFilters_; filter++) {
	response[2*filter  ] = in * getG1().row(filter);
	response[2*filter+1] = in * getG2().row(filter);
    }
}

void MrastaFiltering::appendDerivatives(std::vector<Value> &in) {
    // Pointer to start of the values for the derivative calculation
    Math::Vector<Value>::iterator itBand;
    // Pointer to the start of the derivatives
    Math::Vector<Value>::iterator itDeriv;

    // add first derivative
    if (nDerivatives_ > 0) {
	// set the iterators
	itBand  = in.begin();
	itDeriv = in.begin() + (int) nFeatures_ * 2 * nFilters_;

	// for each Gaussian filter [(G1+G2) * nFilter()]
	for (int filter = 0; filter < 2 * (int) nFilters_; filter++) {
	    // first derivative (no derivatives for first/last band)
	    for (int i = 1; i < (int) nFeatures_-1; i++, itDeriv++, itBand++)
		 *itDeriv = *(itBand + 2) - *itBand;

	    // set the iterator to start of next band (no derivatives for first last band)
	    itBand += 2;
	}
    }

    // add second derivative
    if (nDerivatives_ > 1) {
	// reset the iterators (itDeriv should be correct)
	itBand  = in.begin();
	itDeriv = in.begin() + (int) 2 * nFilters_ * (nFeatures_ + (nFeatures_ -2) );

	// for each Gaussian filter [(G1+G2) * nFilter()]
	for (int filter = 0; filter < 2 * (int) nFilters_; filter++) {
	    // second derivative (no derivatives for first/last band)
	    for (int i = 1; i < (int) nFeatures_ -1; i++, itDeriv++, itBand++)
		 *itDeriv = *(itBand + 2) / 2 - *(itBand + 1)  + *itBand / 2;

	    // set the iterator to start of next band (no derivatives for first last band)
	    itBand += 2;
	}
    }
}


/** apply the Gaussian filtering to the values
 *
 *  @param in input feature vector of size nFrames(101) * nBands(19)
 *  @param out output feature vector of size nFrames(101) * 2 * nFilter(6) + derivatives
 *
 *  @return false if initialization fails, true on success
 * */
bool MrastaFiltering::apply(std::vector<Value> &in, std::vector<Value> &out) {
    // init is requested, initialization failed?
    if (needInit_ && !init())
	return false;

    // get the Gaussian filter response for each band
    for (size_t band = 0; band < nFeatures_; band++) {
	getBand(band, in, inputBuffer_);
	filterEnergyBand(inputBuffer_, outputBuffer_);
	setBand(band, outputBuffer_, out);
    }

    // apply derivatives for the Gaussian filter response
    appendDerivatives(out);
    return true;
}


//============================================================================

const Core::ParameterInt MrastaFilteringNode::paramContextLength(
    "context-length", "number of input CRBE frames (sliding window size)", 101);
const Core::ParameterInt MrastaFilteringNode::paramDerivatives(
    "derivative", "number of derivatives", 1);
const Core::ParameterInt MrastaFilteringNode::paramGaussFilters(
    "gauss-filter", "number of Gaussian filters", 6);

MrastaFilteringNode::MrastaFilteringNode(const Core::Configuration &c) :
    Core::Component(c), Precursor(c),
    contextLength_(0),
    nDerivatives_(0),
    nGaussFilters_(0),
    needInit_(true)
{ }

MrastaFilteringNode::~MrastaFilteringNode() {}

bool MrastaFilteringNode::setParameter(const std::string &name, const std::string &value) {
    if (paramContextLength.match(name))
	setContextLength(paramContextLength(value));
    else if (paramDerivatives.match(name))
	setDerivative(paramDerivatives(value));
    else if (paramGaussFilters.match(name))
	setGaussFilter(paramGaussFilters(value));
    else
	return false;

    return true;
}

bool MrastaFilteringNode::configure() {
    Core::Ref<Flow::Attributes> attributes(new Flow::Attributes());
    getInputAttributes(0, *attributes);

    if (!configureDatatype(attributes, Flow::Vector<Value>::type()))
	return false;

    attributes->set("datatype", Flow::Vector<Value>::type()->name());
    return putOutputAttributes(0, attributes);
}

bool MrastaFilteringNode::work(Flow::PortId p) {
    Flow::DataPtr<Flow::Vector<Value> > ptrFeatures;

    if (getData(0, ptrFeatures)) {
	// init with the length of the concat'd input vectors
	if (needInit_) init(ptrFeatures.get()->size());

	// generate features
	Flow::Vector<Value> *out = new Flow::Vector<Value>(getOutputDimension());
	apply(*(ptrFeatures.get()), *out);

	out->setTimestamp(*ptrFeatures);
	return putData(0, out);
    }
    return putData(0, ptrFeatures.get());
};

/** Initialize all objects of the node and the node itself.
 *  @param length dimension of input feature vector = nFrames * nElements
 */
void MrastaFilteringNode::init(size_t length) {
    // initialize MrastaFiltering (length = nFrames * nElements)
    MrastaFiltering::init((size_t) (length / contextLength_), contextLength_, nGaussFilters_, nDerivatives_);

    // check the output dimension
    if (getOutputDimension() <= 0)
	criticalError() << "bad output dimension: " << getOutputDimension();

    // all inits done
    respondToDelayedErrors();
    needInit_ = false;
}
