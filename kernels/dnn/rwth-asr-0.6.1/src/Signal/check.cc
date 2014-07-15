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
#include <Signal/Module.hh>
#include <Flow/Registry.hh>


class TestApplication : public Core::Application {
public:
  std::string getUsage() const { return "short program to test signal network\n"; }

  int main(const std::vector<std::string> &arguments) {
    INIT_MODULE(Signal)
    Flow::Registry::instance().dumpFilters(std::cout);
    return 0;
  }
};

APPLICATION(TestApplication)
