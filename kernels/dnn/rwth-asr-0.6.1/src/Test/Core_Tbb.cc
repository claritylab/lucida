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
 * Verifies usability of Intel Threading Building Blocks
 */

#include <vector>
#include <ext/numeric>
#include <tbb/blocked_range.h>
#include <tbb/parallel_for.h>
#include <tbb/parallel_reduce.h>
#include <Test/UnitTest.hh>

template<class T>
class Sum
{
private:
    const std::vector<T> &data_;
    T sum_;
public:
    Sum(const std::vector<T> &data) : data_(data), sum_(0) {}
    Sum(const Sum &o, tbb::split) : data_(o.data_), sum_(0) {}
    T result() const { return sum_; }
    void operator()(const tbb::blocked_range<size_t> &range) {
	size_t end = range.end();
	T s = sum_;
	for (size_t i = range.begin(); i != end; ++i)
	    s += data_[i];
	sum_ = s;
    }
    void join(const Sum &other) {
	sum_ += other.sum_;
    }
};

template<class T>
class Plus
{
private:
    T* const data_;
    T increment_;
public:
    Plus(T* const data, T inc) : data_(data), increment_(inc) {}
    void operator()(tbb::blocked_range<size_t> &range) const {
	size_t end = range.end();
	for (size_t i = range.begin(); i != end; ++i)
	    data_[i] += increment_;
    }
};

TEST(Core, Tbb, For)
{
    const size_t nElements = 100000;
    std::vector<s32> data(nElements);
    __gnu_cxx::iota(data.begin(), data.end(), 0);
    Plus<s32> minusTwo(&data[0], -2);
    tbb::parallel_for(tbb::blocked_range<size_t>(0, nElements), minusTwo);
    EXPECT_EQ(-2, data[0]);
    EXPECT_EQ(s32(nElements - 3), data.back());
}

TEST(Core, Tbb, Reduce)
{
    const size_t nElements = 100000;
    std::vector<u32> data(nElements);
    __gnu_cxx::iota(data.begin(), data.end(), 0);
    Sum<u32> intSum(data);
    tbb::parallel_reduce(tbb::blocked_range<size_t>(0, nElements), intSum);
    EXPECT_EQ(u32(nElements*(nElements - 1)/2), intSum.result());
}
