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
#include "CudaDataStructure.hh"

using namespace Math;

bool CudaDataStructure::initialized = false;

bool CudaDataStructure::_hasGpu = false;

cublasHandle_t CudaDataStructure::cublasHandle;

curandGenerator_t CudaDataStructure::randomNumberGenerator;

u32 CudaDataStructure::multiPrecisionBunchSize = 8;

void CudaDataStructure::initialize(){
    if (initialized)
	return;
    initialized = true;
    // check whether MODULE_CUDA is active and a GPU is available
    int nGpus = 0;
    bool hasCuda = false;
    int success = Cuda::getNumberOfGpus(nGpus, hasCuda);
    if (hasCuda){
	if (success != 0){ // no GPU available, or some error occured
	    std::ostringstream ss;
	    ss << "Using binary with GPU support, but no GPU available. Error code is: " << success
	       << " (" << Cuda::getErrorString(success) << ")";
	    warning(ss.str());
	}

	_hasGpu = nGpus > 0;
	// initialize cuBLAS and cuRAND
	if (_hasGpu){
	    std::ostringstream ss; ss << "using 1 of " << nGpus << " GPUs";
	    log(ss.str());
	    success = Cuda::createCublasHandle(cublasHandle);
	    if (success != 0){
		std::ostringstream serr;
		serr << "Failed to initialize cuBLAS library: Error code is: " << success
		     << " (" << Cuda::getErrorString(success) << ")";
		criticalError(serr.str());
	    }
	    success = Cuda::createRandomNumberGenerator(randomNumberGenerator, CURAND_RNG_PSEUDO_DEFAULT);
	    if (success != 0){
		std::ostringstream serr;
		serr << "Failed to initialize cuRAND random number generator library: Error code is: " << success
		     << " (" << Cuda::getErrorString(success) << ")";
		criticalError(serr.str());
	    }
	    success = Cuda::setSeed(randomNumberGenerator, (unsigned long long) rand());
	    if (success != 0){
		std::ostringstream serr;
		serr << "Failed to set seed for cuRAND random number generator: Error code is: " << success
		     << " (" << Cuda::getErrorString(success) << ")";
		criticalError(serr.str());
	    }
	}
	// this should never occur (when no GPU is available, a non-zero error code is returned)
	if (!_hasGpu && success == 0) {
	    std::string msg = "Using binary with GPU support, but no GPU available.";
	    warning(msg);
	}
    }
}

bool CudaDataStructure::hasGpu(){
    if (!initialized)
	initialize();
    return _hasGpu;
}

void CudaDataStructure::setMultiprecisionBunchSize(u32 val) {
    verify_ge(val, 1);
    multiPrecisionBunchSize = val;
}


CudaDataStructure::CudaDataStructure() :
    gpuMode_(hasGpu())
  { }

CudaDataStructure::CudaDataStructure(const CudaDataStructure &x ) :
    gpuMode_(x.gpuMode_)
  { }

void CudaDataStructure::log(const std::string &msg){
    if (Core::Application::us())
	Core::Application::us()->log() << msg;
    else
	std::cerr << msg << std::endl;
}

void CudaDataStructure::warning(const std::string &msg){
    if (Core::Application::us())
	Core::Application::us()->warning() << msg;
    else
	std::cerr << msg << std::endl;
}


void CudaDataStructure::error(const std::string &msg){
    if (Core::Application::us())
	Core::Application::us()->error() << msg;
    else
	std::cerr << msg << std::endl;
}

void CudaDataStructure::criticalError(const std::string &msg){
    if (Core::Application::us())
	Core::Application::us()->criticalError() << msg;
    else
	std::cerr << msg << std::endl;
}
