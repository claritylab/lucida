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
#ifndef _TEST_REGISTRY_HH
#define _TEST_REGISTRY_HH

#include <map>
#include <string>
#include <cppunit/Test.h>
#include <cppunit/TestCaller.h>
#include <cppunit/TestSuite.h>
#include <cppunit/extensions/TestFactory.h>

namespace Test {

/**
 * Registry for all test cases.
 */
class TestSuiteRegistry: public CppUnit::TestFactory
{
public:
    typedef void (*TestMethod)();

    /**
     * return the only TestSuiteRegistry instance (singleton)
     */
    static TestSuiteRegistry& instance();

    /**
     * add a test case to the given test suite.
     */
    bool addTest(const std::string &module, const std::string &suiteName, CppUnit::Test *test);

    /**
     * generate a CppUnit test case including all registered test cases.
     */
    CppUnit::Test* makeTest();

    /**
     * generate a CppUnit test case including all registered test cases
     * for the given module.
     */
    CppUnit::Test* makeTest(const std::string &module);

    virtual ~TestSuiteRegistry() {}
protected:
    typedef std::map<std::string, CppUnit::TestSuite*> SuiteMap;
    typedef std::map<std::string, SuiteMap> ModuleMap;
    ModuleMap modules_;

private:
    TestSuiteRegistry() {}
    static TestSuiteRegistry *instance_;
};


/**
 * adds a test case to the registry.
 * to be used as static object.
 */
template<class T>
class RegisterTest
{
public:
    RegisterTest(const std::string &module, const std::string &suiteName,
		 const std::string &testName, void(T::*m)()) :
	    registry_(TestSuiteRegistry::instance())
    {
	registry_.addTest(module, suiteName, new CppUnit::TestCaller<T>(testName, m));
    }
protected:
    RegisterTest() : registry_(TestSuiteRegistry::instance()) {}
    TestSuiteRegistry &registry_;
};

/**
 * adds a test fixture to the registry.
 * to be used as static object.
 */
template<class T>
class RegisterTestCase: public RegisterTest<T>
{
public:
    RegisterTestCase(const std::string &module, const std::string &suiteName, const std::string &testName)
    {
	this->registry_.addTest(module, suiteName, new T(testName));
    }
};

}  // namespace Test

#endif  // _TEST_REGISTRY_HH
