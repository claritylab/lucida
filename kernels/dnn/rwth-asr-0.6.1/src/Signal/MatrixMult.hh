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
// $Id: MatrixMult.hh 8748 2012-08-09 09:47:30Z rybach $

#ifndef _SIGNAL_MATRIX_MULT_HH
#define _SIGNAL_MATRIX_MULT_HH

#include <fstream>
#include <Math/Matrix.hh>
#include <Core/Types.hh>
#include <Core/MatrixParser.hh>
#include <Core/Hash.hh>
#include <Flow/Data.hh>
#include <Flow/Node.hh>
#include <Flow/Timestamp.hh>
#include <Flow/Types.hh>
#include <Flow/Vector.hh>
#include <Flow/StringExpressionNode.hh>
#include <Math/Module.hh>
#include "Node.hh"

namespace Signal {

    const Core::ParameterString  paramMatrixMultiplicationFileName(
	"file", "name of matrix file to load");
    const Core::ParameterBool  paramMatrixMultiplicationUseCache(
	"use-cache", "uses a cache for different matrices.", false);
    const Core::ParameterBool paramIgnoreTimestamps(
	"ignore-timestamps", "ignore timestamps, i.e. update matrix for every frame", false);
    const Core::ParameterInt paramMultipleDataProcessing(
	"multi-data", "multiple data processing, i.e. more than one vector at a time, this parameter can only be set using a config file", 1);


    /**
     * Matrix multiplication filter.
     * Each input vector is multiplied with a constant matrix.  The
     * result is emitted on output port.  The dimension of the input
     * vector must match the number of columns of the matrix.  The
     * output vector dimension will be equal to the number of row of the
     * matrix.
     * Parameters:
     *  -@c file is a path name to load the matrix from.
     *   Since MatrixMultiplicationNode derives from Flow::StringExpressionNode, @c file can
     *   contain placeholders for input ports. Example: file="$input(warping-factor).matrix".
     *   In this case, node has got an extra input port called warping-factor. The placeholder
     *   $input(warping-factor) will be replaced by the values which arrives on the input port.
     *   For more details @see Flow::StringExpressionNode.
     *  -$c use-cache activates/deactivates a cache for the matrices.
     *  -$c multi-data <number of expected parallel vectors> activates/deactivates
     *   processing of more than one vector per timestamp
     *   In this case input ports have to be numerical names, i.e. 0 , 1 ,2 ...
     *   The result of the multiplication of input vector 0 is then available at output port 0.
     *   Not that all vectors will multiplied with the very same matrix.
     *
     * Inputs: one or more
     * Outputs: one or more
     */
    template <class T>
    class MatrixMultiplicationNode : public Flow::StringExpressionNode {
    private:
	typedef Math::RefernceCountedMatrix<T> Matrix;
	typedef Core::Ref<Matrix> MatrixRef;
	typedef Core::StringHashMap<MatrixRef> MatrixCache;
    private:
	MatrixCache *matrixCache_;
	MatrixRef currentMatrix_;
	std::string currentMatrixFile_;
	bool ignoreTimestamps_;
	s32 multiData_;
    private:
	bool updateMatrix(const std::string &filename);
	MatrixRef loadMatrix(const std::string &filename);
	void activateCache(bool activate);
    public:
	static std::string filterName() {
	    return std::string("signal-matrix-multiplication-") + Core::Type<T>::name;
	}
	MatrixMultiplicationNode(const Core::Configuration &c);
	virtual ~MatrixMultiplicationNode();

	Flow::PortId getInput(const std::string &name);
	Flow::PortId getOutput(const std::string &name);
	virtual bool configure();
	virtual bool setParameter(const std::string &name, const std::string &value);
	virtual bool work(Flow::PortId p);
	virtual bool putDataAllPorts(Flow::Data *d);
    };

    template<class T>
    MatrixMultiplicationNode<T>::MatrixMultiplicationNode(const Core::Configuration &c) :
	Component(c), Node(c), StringExpressionNode(c, paramMultipleDataProcessing(c)), matrixCache_(0),
	multiData_(paramMultipleDataProcessing(c))
    {
	Flow::StringExpressionNode::setTemplate(paramMatrixMultiplicationFileName(c));
	activateCache(paramMatrixMultiplicationUseCache(c));
	ignoreTimestamps_ = paramIgnoreTimestamps(c);
    }

    template<class T>
    MatrixMultiplicationNode<T>::~MatrixMultiplicationNode()
    {
	delete matrixCache_;
    }

    template<class T>
    Flow::PortId MatrixMultiplicationNode<T>::getInput(const std::string &name) {
	if(name.empty()){
	    return addInput(0);
	}
	s32 result = 0;
	// convention:
	// either only one input port or multiple ports
	// with numberical port names, i.e. matrix-mult:0
	if(multiData_ > 1){
	    char* endptr;
	    result = (s32)strtol(name.c_str(), &endptr, 10);
	    if(*endptr != '\0' || result < 0){
		return StringExpressionNode::getInput(name);
	    }
	} else{
	    return StringExpressionNode::getInput(name);
	}
	return addInput(result);
    }

    template<class T>
    Flow::PortId MatrixMultiplicationNode<T>::getOutput(const std::string &name)
    {
	s32 result = 0;
	// convention:
	// either only one output port or multiple ports
	// with numberical port names, i.e. matrix-mult:0
	if(multiData_ > (s32)1){
	    char* endptr;
	    result = (s32)strtol(name.c_str(), &endptr, 10);
	    if(*endptr != '\0' || result < 0){
		return Flow::IllegalPortId;
	    }
	}
	return addOutput(result);
    }

    template<class T>
    bool MatrixMultiplicationNode<T>::configure()
    {
	Core::Ref<Flow::Attributes> attributes(new Flow::Attributes());
	Core::Ref<Flow::Attributes> inputAttributes(new Flow::Attributes());
	for(s32 i = 0; i < multiData_; ++i){
	    getInputAttributes((Flow::PortId)i, *inputAttributes);
	    if (!configureDatatype(inputAttributes, Flow::Vector<f32>::type()))
		return false;
	    if (!StringExpressionNode::configure(*inputAttributes))
		return false;
	    attributes->merge(*inputAttributes);
	}

	attributes->set("datatype", Flow::Vector<f32>::type()->name());
	bool retrval = true;
	for(s32 i = 0; i < multiData_; ++i){
	    retrval = putOutputAttributes((Flow::PortId)i, attributes) && retrval;
	}
	return retrval;
    }

    template<class T>
    bool MatrixMultiplicationNode<T>::setParameter(const std::string &name, const std::string &value)
    {
	if (paramMatrixMultiplicationFileName.match(name))
	    Flow::StringExpressionNode::setTemplate(paramMatrixMultiplicationFileName(value));
	else if (paramMatrixMultiplicationUseCache.match(name))
	    activateCache(paramMatrixMultiplicationUseCache(value));
	else if (paramIgnoreTimestamps.match(name))
	    ignoreTimestamps_ = paramIgnoreTimestamps(value);
	else if (paramMultipleDataProcessing.match(name))
	    error("Parameter multi-data can only be set using a config file");
	else
	    return false;
	return true;
    }

    template<class T>
    bool MatrixMultiplicationNode<T>::work(Flow::PortId p)
    {
	std::vector<Flow::DataPtr<Flow::Vector<T> > > in;
	for(s32 i = 0; i < multiData_; ++i){
	    if(!inputConnected((Flow::PortId)i)){
		continue;
	    }
	    Flow::DataPtr<Flow::Vector<T> > inData;
	    if(!getData((Flow::PortId)i, inData)){
		return putDataAllPorts(inData.get());
	    }
	    in.push_back(inData);
	}

	bool doUpdate = false;
	u32 portNumber = 0;
	bool retrval = true;

	if (ignoreTimestamps_)
	    doUpdate = Flow::StringExpressionNode::update();
	else
	    doUpdate = Flow::StringExpressionNode::update(*(in[0]));
	if (doUpdate)
	    updateMatrix(StringExpressionNode::value());
	if (!currentMatrix_) {
	    u32 portNumber = 0;
	    bool retrval = true;
	    error() << "no matrix for multiplication";
	    for(s32 i = 0; i < multiData_; ++i){
		if(nOutputLinks((Flow::PortId)i)){
		    Flow::Vector<T> *out = new Flow::Vector<T>(*(in[portNumber]));
		    out->setTimestamp(*(in[portNumber]));
		    retrval = putData((Flow::PortId)portNumber, out) && retrval;
		    ++portNumber;
		}
	    }
	    return retrval;
	}

	for(s32 i = 0; i < multiData_; ++i){
	    if(inputConnected((Flow::PortId)i)){
		if (in[portNumber]->size() != currentMatrix_->nColumns()) {
		    error() << "vector/matrix dimension mismatch: vector(at port " << portNumber << " ): " << in[portNumber]->size()
			    << ", matrix " << currentMatrix_->nColumns() << " columns";
		    return putDataAllPorts(Flow::Data::eos());
		} // end inner if
		++portNumber;
	    } // end connected if
	}

	portNumber = 0;
	for(s32 i = 0; i < multiData_; ++i){
	    if(nOutputLinks((Flow::PortId)i)){
		Flow::Vector<T> *out = new Flow::Vector<T>((*currentMatrix_) * (*(in[portNumber])));
		out->setTimestamp(*(in[portNumber]));
		retrval = putData((Flow::PortId)portNumber, out) && retrval;
		++portNumber;
	    }
	}

	return retrval;
    }

    template <class T>
    bool MatrixMultiplicationNode<T>::putDataAllPorts(Flow::Data *d)
    {
	bool retrval = true;
	bool portConnected = false;
	for(s32 i = 0; i < multiData_ ; ++i){
	    if(nOutputLinks((Flow::PortId)i)){
		portConnected = true;
		retrval = putData((Flow::PortId)i, d) && retrval;
	    }
	}
	return (portConnected ? retrval : false);
    }

    template <typename T>
    bool MatrixMultiplicationNode<T>::updateMatrix(const std::string &filename)
    {
	if (filename.empty()) {
	    error() << "update matrix filename is empty.";
	}
	if (filename == currentMatrixFile_)
	    return (bool)currentMatrix_;
	if (matrixCache_) {
	    typename MatrixCache::const_iterator m = matrixCache_->find(filename);
	    if (m != matrixCache_->end())
		currentMatrix_ = m->second;
	    else {
		currentMatrix_ = loadMatrix(filename);
		currentMatrixFile_ = filename;
		if (currentMatrix_) matrixCache_->insert(std::make_pair(filename, currentMatrix_));
	    }
	} else {
	    currentMatrix_ = loadMatrix(filename);
	    currentMatrixFile_ = filename;
	}
	return (bool)currentMatrix_;
    }

    template <typename T>
    void MatrixMultiplicationNode<T>::activateCache(bool activate)
    {
	if (activate && !matrixCache_) {
	    matrixCache_ = new MatrixCache();
	}
	if (!activate && matrixCache_) {
	    delete matrixCache_; matrixCache_ = 0;
	}
    }

    template <typename T>
    typename MatrixMultiplicationNode<T>:: MatrixRef
    MatrixMultiplicationNode<T>::loadMatrix(const std::string &filename)
    {
	MatrixRef result;
	if (filename.empty()) {
	    error() << "Matrix filename is empty.";
	} else {
	    result = Core::ref(new Matrix());
	    if (!Math::Module::instance().formats().read(
		    filename, *result)) {
		error("Failed to read matrix from file '%s'.", filename.c_str());
		result.reset();
	    }
	}
	return result;
    }

} // namespace Signal

#endif // _SIGNAL_MATRIX_MULT_HH
