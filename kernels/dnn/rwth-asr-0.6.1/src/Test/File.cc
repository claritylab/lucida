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
#include <Test/File.hh>
#include <cstdlib>
#include <cstdio>

namespace Test {

void Directory::create()
{
    char *t = std::getenv("TMPDIR");
    std::string tmpdir;
    if (t)
	tmpdir = t;
    else
	tmpdir = "/tmp";
    const size_t len = tmpdir.length() + 8;
    char *dir_template = new char[len];
    snprintf(dir_template, len, "%s/XXXXXX", tmpdir.c_str());
    path_ = ::mkdtemp(dir_template);
    delete[] dir_template;
}

void Directory::remove()
{
    Core::removeDirectory(path_);
}

} // namespace Test
