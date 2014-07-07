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
#include <Test/Registry.hh>
#include <cppunit/extensions/TestFactoryRegistry.h>

using namespace Test;

TestSuiteRegistry* TestSuiteRegistry::instance_ = 0;

TestSuiteRegistry& TestSuiteRegistry::instance()
{
    if (!instance_) {
	instance_ = new TestSuiteRegistry();
	CppUnit::TestFactoryRegistry::getRegistry().registerFactory(instance_);
    }
    return *instance_;
}

/**
 * add a test case to the given test suite.
 */
bool TestSuiteRegistry::addTest(const std::string &module,
				const std::string &suiteName,
				CppUnit::Test *test)
{
    SuiteMap &suites = modules_[module];
    if (suites.find(suiteName) == suites.end()) {
	suites.insert(SuiteMap::value_type(suiteName,
					   new CppUnit::TestSuite(suiteName)));
    }
    suites[suiteName]->addTest(test);
    return true;
}

/**
 * generate a CppUnit test case including all registered test cases.
 */
CppUnit::Test* TestSuiteRegistry::makeTest()
{
    CppUnit::TestSuite *allTests = new CppUnit::TestSuite("all");
    for (ModuleMap::const_iterator m = modules_.begin(); m != modules_.end(); ++m) {
	for (SuiteMap::const_iterator i = m->second.begin(); i != m->second.end(); ++i) {
	    allTests->addTest(i->second);
	}
    }
    return allTests;
}

CppUnit::Test* TestSuiteRegistry::makeTest(const std::string &module)
{
    if (module.empty())
	return makeTest();
    CppUnit::TestSuite *allTests = new CppUnit::TestSuite(module);
    ModuleMap::const_iterator m = modules_.find(module);
    if (m != modules_.end()) {
	for (SuiteMap::const_iterator i = m->second.begin(); i != m->second.end(); ++i) {
	    allTests->addTest(i->second);
	}
    }
    return allTests;
}
