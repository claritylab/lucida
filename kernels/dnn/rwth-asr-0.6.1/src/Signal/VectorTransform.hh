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
#ifndef _SIGNAL_VECTOR_TRANSFORM_HH
#define _SIGNAL_VECTOR_TRANSFORM_HH

#include <Math/AnalyticFunction.hh>
#include <Flow/Node.hh>

namespace Signal {

    /** For each ith element x in vector X performs x = op(x, f(i)).
     *  Operation "op" can be an arbitrary binary function.
     *  f(x) is an arbitrary analytic function.
     */
    class ContinuousVectorTransformNode : public Flow::SleeveNode {
	typedef Flow::SleeveNode Precursor;
    private:
	static const Core::ParameterString paramF;
	static const Core::ParameterString paramOperation;
    private:
	typedef f32 Data;
    private:
	f64 sampleRate_;
	size_t inputSize_;

	std::string fDeclaration_;
	Math::UnaryAnalyticFunctionRef f_;

	std::string operationDeclaration_;
	Math::BinaryAnalyticFunctionRef operation_;

	bool needInit_;
    private:
	void setSampleRate(f64 sampleRate) { sampleRate_ = sampleRate; needInit_ = true; }

	void setF(const std::string &declaration) {
	    if (fDeclaration_ != declaration) { fDeclaration_ = declaration; needInit_ = true; }
	}
	bool createF();

	void setOperation(const std::string &declaration) {
	    if (operationDeclaration_ != declaration) { operationDeclaration_ = declaration; needInit_ = true; }
	}
	bool createOperation();

	void init(size_t inputSize);
    private:
	void apply(std::vector<Data> &in);
    public:
	static std::string filterName() {
	    return std::string("signal-vector-") + Core::Type<Data>::name + "-continuous-transform";
	}
	ContinuousVectorTransformNode(const Core::Configuration &c);
	virtual ~ContinuousVectorTransformNode();

	virtual bool configure();
	virtual bool setParameter(const std::string &name, const std::string &value);
	virtual bool work(Flow::PortId p);
    };
} // namespace Signal

#endif // _SIGNAL_VECTOR_TRANSFORM_HH
