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
/**
 * Test functions for Core::ThreadPool
 */

#include <unistd.h>
#include <Core/ThreadPool.hh>
#include <Test/UnitTest.hh>

using namespace Core;

class TestTask {
public:
    explicit TestTask(int v = 0) { value = v; }
    int value;
    void set(int v) { value = v; }
};

class TestMapper {
public:
    TestMapper* clone() const { return new TestMapper(); }
    void map(const TestTask &task) {
	sum += task.value;
    }
    void reset() {
	sum = 0;
    }
    int sum;
};

class TestPtrMapper {
public:
    TestPtrMapper* clone() const { return new TestPtrMapper(); }
    void map(const TestTask *task) {
	sum += task->value;
	delete task;
    }
    void reset() {
	sum = 0;
    }
    int sum;
};

class TestPtrModifyMapper {
public:
    TestPtrModifyMapper* clone() const { return new TestPtrModifyMapper(); }
    void map(TestTask *task) {
	task->set(1);
	sum += task->value;
    }
    void reset() {
	sum = 0;
    }
    int sum;
};

class TestReducer {
public:
    TestReducer() { sum = 0; }
    void reduce(TestMapper *mapper) {
	sum += mapper->sum;
    }
    void reduce(TestPtrMapper *mapper) {
	sum += mapper->sum;
    }
    void reduce(TestPtrModifyMapper *mapper) {
	sum += mapper->sum;
    }
    int sum;
};


TEST(Core, ThreadPool, Simple)
{
    ThreadPool<TestTask, TestMapper, TestReducer> pool;
    TestMapper mapper;
    pool.init(10, mapper);
    const int num_task = 100;
    for (int t = 0; t < num_task; ++t) {
	pool.submit(TestTask(t));
    }
    TestReducer reducer;
    pool.combine(&reducer);
    EXPECT_EQ(reducer.sum, num_task*(num_task - 1)/2);
}

TEST(Core, ThreadPool, Pointer)
{
    ThreadPool<TestTask*, TestPtrMapper, TestReducer> pool;
    TestPtrMapper mapper;
    pool.init(10, mapper);
    const int num_task = 100;
    for (int t = 0; t < num_task; ++t) {
	pool.submit(new TestTask(t));
    }
    TestReducer reducer;
    pool.combine(&reducer);
    EXPECT_EQ(reducer.sum, num_task*(num_task - 1)/2);
}

TEST(Core, ThreadPool, Modify)
{
    ThreadPool<TestTask*, TestPtrModifyMapper, TestReducer> pool;
    TestPtrModifyMapper mapper;
    pool.init(10, mapper);
    const int num_task = 100;
    for (int t = 0; t < num_task; ++t) {
	pool.submit(new TestTask(t));
    }
    TestReducer reducer;
    pool.combine(&reducer);
    EXPECT_EQ(reducer.sum, num_task);
}

TEST(Core, ThreadPool, Reset)
{
    ThreadPool<TestTask, TestMapper, TestReducer> pool;
    TestMapper mapper;
    pool.init(10, mapper);
    int num_task = 100;
    for (int t = 0; t < num_task; ++t) {
	pool.submit(TestTask(t));
    }
    pool.wait();
    pool.reset();
    num_task = 10;
    for (int t = 0; t < num_task; ++t) {
	pool.submit(TestTask(t));
    }
    TestReducer reducer;
    pool.combine(&reducer);
    EXPECT_EQ(reducer.sum, num_task*(num_task - 1)/2);
}
