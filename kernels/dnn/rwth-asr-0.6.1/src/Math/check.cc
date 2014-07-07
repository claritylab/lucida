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
#include <Core/Application.hh>
#include <Math/Matrix.hh>
#include <Math/Nr/MatrixTools.hh>
#include <Math/Lapack/MatrixTools.hh>

class TestApplication :
    public Core::Application
{
public:
    virtual std::string getUsage() const {
	return "short program to test Math features\n";
    }

    TestApplication() : Core::Application() {
	setTitle("check");
    }

    int main(const std::vector<std::string> &arguments) {
	Math::Matrix<double> m(4, 4);
	m[0][0] = 1; m[0][1] = 4; m[0][2] = 1; m[0][3] = 8;
	m[1][0] = 2; m[1][1] = 3; m[1][2] = 7; m[1][3] = 3;
	m[2][0] = 3; m[2][1] = 7; m[2][2] = 3; m[2][3] = 5;
	m[3][0] = 4; m[3][1] = 1; m[3][2] = 2; m[3][3] = 7;

	Math::Matrix<double> nrInv(m);
	Math::Matrix<double> lapackInv(m);

	Math::Nr::invert(nrInv);
	Math::Lapack::invert(lapackInv);

	log("Original matrix: ") << m;
	log("Nr computed inverse: ") << nrInv;
	log("Lapack computed inverse: ") << lapackInv;
	log("Difference: ") << nrInv - lapackInv;

	nrInv = m;
	lapackInv = m;

	Math::Nr::pseudoInvert(nrInv);
	Math::Lapack::pseudoInvert(lapackInv);

	log("Nr computed pseudo inverse: ") << nrInv;
	log("Lapack computed pseudo inverse: ") << lapackInv;
	log("Difference: ") << nrInv - lapackInv;

	log("Nr computed determinant: ") << Math::Nr::determinant(m);
	log("Lapack computed determinant: ") << Math::Lapack::determinant(m);
	log("Difference: ") << Math::Nr::determinant(m) - Math::Lapack::determinant(m);

	log("Nr computed log-determinant: ") << Math::Nr::logDeterminant(m);
	log("Lapack computed log-determinant: ") << Math::Lapack::logDeterminant(m);
	log("Difference: ") << Math::Nr::logDeterminant(m) - Math::Lapack::logDeterminant(m);

	return 0;
    }

};

APPLICATION(TestApplication)
