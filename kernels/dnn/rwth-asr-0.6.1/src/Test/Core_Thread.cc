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
 * Test cases for Core::Thread, Core::Mutex
 */

#include <unistd.h>
#include <vector>
#include <Core/Thread.hh>
#include <Test/UnitTest.hh>

using namespace Core;

class Shared {
private:
    mutable Mutex lock_;
    int a_;
public:
    Shared() : a_(0) {}
    void inc() {
	lock_.lock();
	++a_;
	lock_.release();
    }

    int get() const {
	int r;
	lock_.lock();
	r = a_;
	lock_.release();
	return r;
    }
};

class TestThread : public Thread {
public:
    TestThread(Shared *data) : data_(data) {}
protected:
    virtual void run() {
	timeval now;
	gettimeofday(&now, 0);
	::usleep(now.tv_usec);
	data_->inc();
    }
    Shared *data_;
};

TEST(Core, Thread, Simple) {
    const int num_thread = 10;
    std::vector<TestThread*> threads;
    Shared data;
    for (int t = 0; t < num_thread; ++t)
	threads.push_back(new TestThread(&data));
    for (int t = 0; t < num_thread; ++t)
	threads[t]->start();
    for (int t = 0; t < num_thread; ++t)
	threads[t]->wait();
    for (int t = 0; t < num_thread; ++t)
	delete threads[t];
	EXPECT_EQ(data.get(), num_thread);
}
