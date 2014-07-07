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
#include <Modules.hh>
#include <Core/Application.hh>
#include "StateTree.hh"

/**
 * Test suite for the Search module of SPRINT.
 *
 */
class TestApplication : public virtual Core::Application
{
public:
  /**
   * Standard constructor for setting title.
   */
  TestApplication ( ) : Core::Application ( ) { setTitle ( "check" ); }//end TestApplication

  std::string getUsage() const { return "short program to test speech mbr toolkit\n"; }//end getUsage

  int main ( const std::vector<std::string> &arguments )
  {
      /*
      std::cout << "reading " << arguments[0] << std::endl;
      OpenFst::VectorFst *f = OpenFst::VectorFst::Read(arguments[0]);
      u32 sid = atof(arguments[1].c_str());
      std::cout << "state " << sid << std::endl;
      OpenFst::ArcIterator ai(*f, sid);
      for (; !ai.Done(); ai.Next()) {
	  const OpenFst::Arc &a = ai.Value();
	  std::cout << "arc " << a.ilabel << " " << a.olabel << " " << a.nextstate << " " << a.weight << std::endl;
      }
      */
      /*
      FstLib::EncodeTable<FstLib::StdArc> t(FstLib::kEncodeLabels);
      std::ifstream strm(arguments[0].c_str(), std::ifstream::in | std::ifstream::binary);
      t.Read(strm, "output");
      std::cout << "size: " << t.Size() << std::endl;
      for (u32 l = 1; l <= t.Size()+1; ++l) {
	  const FstLib::EncodeTable<FstLib::StdArc>::Tuple *tuple = t.Decode(l);
	  std::cout << l << " -> " << tuple->ilabel << " " << tuple->olabel << " " << tuple->weight.Value() << std::endl;
      }
      */
      /*
      std::cout << "sizeof(Exit):      " << sizeof(Search::StateTree::Exit) << std::endl;
      std::cout << "sizeof(Bliss::LemmaPronunciation *): " << sizeof(Bliss::LemmaPronunciation *) << std::endl;
      std::cout << "sizeof(StateId) : " << sizeof(Search::StateTree::StateId) << std::endl;
      std::cout << "sizeof(StateDesc): " << sizeof(Search::StateTree::StateDesc) << std::endl;
      std::cout << "sizeof(Mm::MixtureIndex): " << sizeof(Mm::MixtureIndex) << std::endl;
      std::cout << "sizeof(State):     " << sizeof(Search::StateTree::State) << std::endl;
      */

      return EXIT_SUCCESS;
  }//end main

};

APPLICATION(TestApplication)
