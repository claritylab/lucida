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
#include <iostream>
#include <string>
#include <vector>
#include <cppunit/TestFailure.h>
#include <cppunit/TestResult.h>
#include <cppunit/TestResultCollector.h>
#include <cppunit/TestRunner.h>
#include <cppunit/TextTestProgressListener.h>
#include <cppunit/TextOutputter.h>
#include <Core/Application.hh>
#include <Test/UnitTest.hh>
#include <Test/Registry.hh>

class ProgressListener: public CppUnit::TestListener
{
public:
    ProgressListener() : allTestsPassed_(true) {}
    virtual ~ProgressListener() {}

    virtual void startSuite(CppUnit::Test *suite)
    {
	std::string name = suite->getName();
	if (name.empty()) name = "unnamed";
	std::cerr
	    << name
	    << " (" << suite->getChildTestCount() << ")"
	    << std::endl;
    }

    virtual void startTest(CppUnit::Test *test)
    {
	std::cerr
	    << "    "
	    << (test->getName().empty() ? "n/a" : test->getName())
	    << std::endl;
	curTestFailure_ = false;
    }

    virtual void addFailure(const CppUnit::TestFailure &failure)
    {
	curTestFailure_ = true;
	allTestsPassed_ = false;
    }

    virtual void endTest(CppUnit::Test *test)
    {
	std::cerr
	    << "        => "
	    << (curTestFailure_ ? "FAILED" : "OK")
	    << std::endl;
    }

    bool allTestsPassed() const { return allTestsPassed_; }

private:
    ProgressListener(const ProgressListener &copy);
    void operator =(const ProgressListener &copy);
    bool curTestFailure_, allTestsPassed_;
};


class UnitTester : public Core::Application
{
public:
    UnitTester() {
	setDefaultLoadConfigurationFile(false);
	setDefaultOutputXmlHeader(false);
	setTitle("unit-tester");
    }

    std::string getUsage() const {
	return "Sprint unit test\n";
    }

    int main(const std::vector<std::string> &arguments);

protected:
    CppUnit::Test* findTest(CppUnit::Test *root, const std::string &name);
    static const Core::ParameterString paramModule;
};

APPLICATION(UnitTester)

const Core::ParameterString UnitTester::paramModule(
    "module", "select test cases for one module only", "");

CppUnit::Test* UnitTester::findTest(CppUnit::Test *root, const std::string &name)
{
    std::deque<CppUnit::Test*> to_visit;
    to_visit.push_back(root);
    while (!to_visit.empty()) {
	CppUnit::Test *t = to_visit.front();
	to_visit.pop_front();
	if (t->getName() == name)
	    return t;
	for (int i = 0; i < t->getChildTestCount(); ++i) {
	    to_visit.push_back(t->getChildTestAt(i));
	}
    }
    return 0;
}


int UnitTester::main(const std::vector<std::string> &arguments)
{
    for (std::vector<std::string>::const_iterator t = arguments.begin(); t != arguments.end(); ++t) {
	std::cout << "arg '" << *t << "'" << std::endl;
    }
    CppUnit::TestResult controller;
    CppUnit::TestResultCollector result;
    controller.addListener(&result);
    ProgressListener progressListener;
    controller.addListener(&progressListener);
    CppUnit::TestRunner runner;
    Test::TestSuiteRegistry &registry = Test::TestSuiteRegistry::instance();
    CppUnit::Test *root = registry.makeTest(paramModule(config));
    if (!arguments.empty()) {
	for (std::vector<std::string>::const_iterator t = arguments.begin(); t != arguments.end(); ++t) {
	    CppUnit::Test *test = findTest(root, *t);
	    if (!test)
		criticalError("Test '%s' not found", t->c_str());
	    else
		runner.addTest(test);
	}
    } else {
	// run all known tests
	runner.addTest(root);
    }

    runner.run(controller);
    CppUnit::TextOutputter output(&result, std::cout);
    output.write();
    return !progressListener.allTestsPassed();
}
