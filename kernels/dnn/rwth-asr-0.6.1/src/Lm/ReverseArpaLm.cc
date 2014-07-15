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
#include <ext/hash_map>
#include <ext/hash_set>
#include <string>
#include <iostream>
#include <fstream>
#include <assert.h>
#include <limits>
#include <string.h>
#include <cstdlib>
#include <cmath>
#include <vector>

// #define STANDALONE

#ifndef STANDALONE
#include <Core/CompressedStream.hh>
#endif

namespace Lm {

static const bool skipSentenceBegin = false;

template <class T>
struct MyHash
{
    inline T operator()(T v)
    {
	v = (v ^ 0xc761c23c) ^ (v >> 19);
	v = (v + 0xfd7046c5) + (v << 3);
	return v;
    }
};

struct StringHash {
    inline size_t operator()(const std::string& str) const {
	return __gnu_cxx::hash<const char*>()(str.data());
    }
};

class StringIndex
{
public:
    int size() const {
	return strings_.size();
    }
private:
    friend class IndexedString;
    typedef __gnu_cxx::hash_map<std::string, int, StringHash> Index;
    Index index_;
    std::vector<std::string> strings_;
    std::string emptyString_;
};

class IndexedString
{
public:
    IndexedString() : index_(-1) {}

    IndexedString(const std::string& text, StringIndex& index) {
	assert(text.size() == 0 || (text[0] != ' ' && text[text.size() - 1] != ' '));
	StringIndex::Index::iterator it = index.index_.find(text);
	if (it != index.index_.end())
	    index_ = it->second;
	else{
	    index_ = index.strings_.size();
	    index.strings_.push_back(text);
	    index.index_.insert(std::make_pair(text, index_));
	}
    }

    std::string get(const StringIndex& index) const {
	if(index_ == -1)
	    return index.emptyString_;
	else
	    return index.strings_[index_];
    }

    const int index() const {
	return index_;
    }

    bool operator==(const IndexedString& rhs) const {
	return index_ == rhs.index_;
    }

private:
    int index_;
};

class NGrams
{
public:
    NGrams(int order) : order_(order) {
	assert(order_ != 0);
    }

    int find(const IndexedString* data) const {
	unsigned int hash = 0;
	for(int o = 0; o < order_; ++o)
	    hash = MyHash<unsigned int>() (data[o].index() + hash);
	std::pair<Hash::const_iterator, Hash::const_iterator> found = index_.equal_range(hash);
	for (; found.first != found.second; ++found.first)
	    if (memcmp(data_.data() + (found.first->second * order_), data, sizeof(IndexedString) * order_) == 0)
		return found.first->second;  // Match
	return -1;
    }

    int insert(const IndexedString* data) {
	unsigned int hash = 0;
	for(int o = 0; o < order_; ++o)
	    hash = MyHash<unsigned int>() (data[o].index() + hash);
	int ret = data_.size() / order_;
	for(int o = 0; o < order_; ++o)
	    data_.push_back(data[o]);
	index_.insert(std::make_pair(hash, ret));
	return ret;
    }

    int size() const {
	return data_.size() / order_;
    }

    const IndexedString* get(int index) const {
	return data_.data() + index * order_;
    }

    void setProb(int index, float prob, float backoff) {
	if (index >= (int)probs_.size())
	    probs_.resize(index + 1, std::make_pair(0.0f, 0.0f));
	assert(probs_[index] == std::make_pair(0.0f, 0.0f) || probs_[index] == std::make_pair(prob, backoff));
	probs_[index] = std::make_pair(prob, backoff);
    }

    std::pair<float, float> prob(int index) const {
	if (index >= (int)probs_.size())
	    return std::make_pair(0.0f, 0.0f);
	else
	    return probs_[index];
    }

    int order_;
private:
    std::vector<IndexedString> data_;
    typedef __gnu_cxx::hash_multimap<unsigned int, int> Hash;
    std::vector<std::pair<float, float> > probs_;
    Hash index_;
};

// Returns p(lastword|history...) for ngram = [ history1, history2, ..., lastword ]
float score(const IndexedString* ngram, int len, const std::vector<NGrams>& ngrams, const StringIndex& stringIndex)
{
    assert(len);
    if (len > (int)ngrams.size())
    {
	ngram += len - ngrams.size();
	len = ngrams.size();
    }

    float score = 0;

    while (len > 0)
    {
	int index = ngrams[len - 1].find(ngram);
	if (index != -1) {
	    return score + ngrams[len - 1].prob(index).first;
	}else{
	    if (len >= 2)
	    {
		// try finding a backoff weight for the history
		index = ngrams[len - 2].find(ngram);
		if (index != -1)
		    score += ngrams[len - 2].prob(index).second;
	    }
	    len -= 1;
	    if (len == 0)
	    {
		std::cerr << "ERROR: Missing unigram for " << ngram->get(stringIndex) << std::endl;
		exit(9);
	    }
	    ++ngram;
	}
    }
    return score;
}

float sentenceScore(const IndexedString* sentence, int len, const std::vector<NGrams>& ngrams, const StringIndex& index, bool print = false)
{
    float ret = 0;
    for (int pos = 1; pos <= len; ++pos)
    {
	float s = score(sentence, pos, ngrams, index);
	ret += s;
	if (print)
	    std::cout << "word '" << sentence[pos - 1].get(index) << "' score " << s << std::endl;
    }
    return ret;
}

void reverseArpaLm(const std::string& inName, const std::string& outName, int testSamples, std::string testFile) {
#ifdef STANDALONE
    std::ifstream infile(inName.c_str());
#else
    Core::CompressedInputStream infile(inName.c_str());
#endif

    StringIndex stringIndex;

    std::string line;

    while (line.size() == 0 && infile.good())
	std::getline(infile, line);
    assert(line.find("\n") == std::string::npos);
    if(line != "\\data\\")
    {
	std::cerr << "bad first line: \"" << line << "\"" << std::endl;
	exit(6);
    }

    // skip until ngram counts
    while(line.size() && line.substr(0, 5) != "ngram")
    {
	std::getline(infile, line);
	assert(infile.good());
    }

    std::vector<int> ngramCounts;
    std::vector<NGrams> ngrams;

    // get ngram counts

    int totalCount = 0;

    while(line.substr(0, 5) == "ngram")
    {
	std::size_t bound = line.find('=');
	assert(bound != std::string::npos);

	std::string orderStr(line.substr(6, bound - 6));
	int order = atoi(orderStr.c_str());
	std::string countStr(line.substr(bound + 1));
	int count = atoi(countStr.c_str());
	totalCount += count;
	ngramCounts.push_back(count);
	ngrams.push_back(NGrams(order));

	std::getline(infile, line);
	assert(infile.good());
    }

    std::cout << "reading " << totalCount << " n-grams" << std::endl;

    float sentprob = 0.0;

    IndexedString sentenceBegin("<s>", stringIndex), sentenceEnd("</s>", stringIndex);

    // read actual ngrams
    for(int n = 1; n <= (int)ngramCounts.size(); ++n)
    {
	std::cout << "reading order " << n << std::endl;
	int backed = 0;
	NGrams& this_ngrams(ngrams[n - 1]);
	// skip until ngram counts
	while(line.find("-grams") == std::string::npos)
	{
	    std::getline(infile, line);
	    assert(infile.good());
	}
	int count = ngramCounts[n - 1];
	std::vector<IndexedString> indexedLine(n);

	for (int ng = 0; ng < count; ++ng)
	{
	    std::getline(infile, line);
	    assert(infile.good());
	    size_t pos = line.find(' ');
	    size_t tabPos = line.find('\t');
	    if (tabPos != std::string::npos && (pos == std::string::npos || tabPos < pos))
		pos = tabPos;
	    assert(pos != std::string::npos);
	    line[pos] = 0;
	    float prob = atof(line.c_str());
	    float backoff = 0.0;
	    int currentOrder = 0;
	    // Read ngram
	    while (true)
	    {
		++pos;
		size_t start = pos;
		pos = line.find(' ', start);
		tabPos = line.find('\t', start);
		if (tabPos != std::string::npos && (pos == std::string::npos || tabPos < pos))
		    pos = tabPos;
		if (pos == std::string::npos)
		{
		    if (currentOrder < n)
		    {
			indexedLine[currentOrder] = IndexedString(line.data() + start, stringIndex);
			++currentOrder;
		    }else{
			backoff = atof(line.data() + start);
		    }
		    assert(currentOrder == n);
		    break;
		}else{
		    line[pos] = 0;
		    indexedLine[currentOrder] = IndexedString(line.data() + start, stringIndex);
		    ++currentOrder;
		}
	    }

	    if (n == 1 && indexedLine[0] == sentenceBegin && skipSentenceBegin)
	    {
		sentprob = prob;
		prob = 0.0;
	    }

	    int index = this_ngrams.insert(&indexedLine[0]);
	    this_ngrams.setProb(index, prob, backoff);

	    for (int backoffOrder = n - 1; backoffOrder > 0; --backoffOrder)
	    {
		assert(ngrams[backoffOrder - 1].order_ == backoffOrder);
		// Create missing backoff ngrams for reversed LM
		IndexedString* l_ngram = &indexedLine[0]; // shortened ngram

		bool added = false;

		if (ngrams[backoffOrder - 1].find(l_ngram) == -1) {
		    float s = score(l_ngram, backoffOrder, ngrams, stringIndex);
		    int index2 = ngrams[backoffOrder - 1].insert(l_ngram);
		    ngrams[backoffOrder - 1].setProb(index2, s, 0);
		    added = true;
		    backed += 1;
		}

		IndexedString* r_ngram = &indexedLine[1]; // shortened ngram with offset one
		if (ngrams[backoffOrder - 1].find(r_ngram) == -1) {
		    float s = score(r_ngram, backoffOrder, ngrams, stringIndex);
		    int index2 = ngrams[backoffOrder - 1].insert(r_ngram);
		    ngrams[backoffOrder - 1].setProb(index2, s, 0);
		    added = true;
		    backed += 1;
		}

		// add all missing backoff ngrams for forward lm
		IndexedString* h_ngram = &indexedLine[n - backoffOrder]; // shortened history
		if (ngrams[backoffOrder - 1].find(h_ngram) == -1) {
		    float s = score(h_ngram, backoffOrder, ngrams, stringIndex);
		    int index2 = ngrams[backoffOrder - 1].insert(h_ngram);
		    ngrams[backoffOrder - 1].setProb(index2, s, 0);
		    added = true;
		    backed += 1;
		}
		if (!added)
		    break;
	    }
	}
	std::cout << "got " << this_ngrams.size() << " ngrams for this order, backed " << backed << std::endl;
    }

    while (infile.good() and line.substr(0, 5) != "\\end\\") {
	std::getline(infile, line);
    }

    if (line.size() == 0)
    {
	std::cerr << "invalid ARPA file" << std::endl;
	exit(5);
    }

    std::vector<NGrams> newNGrams; // Eventually used for validation

    infile.close();

    int order = ngrams.size();

    float offset = 0.0;

    std::cout << "writing " << outName << std::endl;

    // Write new reversed ARPA model
#ifdef STANDALONE
    std::ofstream outfile(outName.c_str());
#else
    Core::CompressedOutputStream outfile(outName.c_str());
#endif

    outfile << "\\data\\\n";

    for (int n = 1; n <= order; ++n) // unigrams, bigrams, trigrams
	outfile << "ngram " << n << "=" << ngrams[n - 1].size() << "\n";

    for (int n = 1; n <= order; ++n) // unigrams, bigrams, trigrams
    {
	if (testSamples || testFile.size())
	    newNGrams.push_back(NGrams(n));

	std::cout << "reversing order " << n << std::endl;
	outfile << "\\" << n << "-grams:\n";
	int ngramCount = ngrams[n - 1].size();
	for (int ngram = 0; ngram < ngramCount; ++ngram)
	{
	    const IndexedString* words = ngrams[n - 1].get(ngram);
	    std::pair<float, float> prob = ngrams[n - 1].prob(ngram);
	    float revprob = prob.first;
//         if (prob.second != inf) // only backoff weights from not newly created ngrams
	    revprob += prob.second;

	    // sum all missing terms in decreasing ngram order
	    for (int backoffOrder = n - 1; backoffOrder > 0; --backoffOrder)
	    {
		const IndexedString* l_ngram = words;
		int backoffIndex = ngrams[backoffOrder - 1].find(l_ngram);
		assert(backoffIndex != -1);
		revprob += ngrams[backoffOrder - 1].prob(backoffIndex).first;

		const IndexedString* r_ngram = words + 1;
		backoffIndex = ngrams[backoffOrder - 1].find(r_ngram);
		assert(backoffIndex != -1);
		revprob -= ngrams[backoffOrder - 1].prob(backoffIndex).first;
	    }

	    float backoff = 0.0;

	    if (skipSentenceBegin && n != order && words[n - 1] == sentenceEnd)
	    {
		if (n == 1)
		{
		    offset = revprob; // remember <s> weight
		    revprob = sentprob; // apply <s> weight from forward model
		    backoff = offset;
		}else if (n == 2) {
		    revprob += offset;
		}
	    }

	    outfile << revprob << "\t";
	    for (int w = n - 1; w >= 0; --w) {
		if (words[w] == sentenceBegin)
		    outfile << sentenceEnd.get(stringIndex);
		else if(words[w] == sentenceEnd)
		    outfile << sentenceBegin.get(stringIndex);
		else
		    outfile << words[w].get(stringIndex);
		if (w)
		    outfile << " ";
	    }

	    if (testSamples || testFile.size())
	    {
		std::vector<IndexedString> revWords(n);
		for (int w = n - 1; w >= 0; --w) {
		    if (words[w] == sentenceBegin)
			revWords[n - w - 1] = sentenceEnd;
		    else if(words[w] == sentenceEnd)
			revWords[n - w - 1] = sentenceBegin;
		    else
			revWords[n - w - 1] = words[w];
		}
		int index = newNGrams.back().insert(&revWords[0]);
		newNGrams.back().setProb(index, revprob, backoff); // prob.second != inf ? backoff : -100000.0);
	    }

	    if (n != order) {
		// not highest order, print a backoff weight
		outfile << " ";
		outfile << backoff;
	    }
	    outfile << "\n";
	}
    }
    outfile << "\\end\\\n";

    if (testSamples || testFile.size())
    {
	std::vector<std::vector<IndexedString> > testLines;

	if (testFile.size())
	{
	    std::ifstream inTestFile(testFile.c_str());
	    while (inTestFile.good())
	    {
		std::string line;
		std::getline(inTestFile, line);
		if (line.size())
		{
		    testLines.push_back(std::vector<IndexedString>());
		    size_t pos = 0;
		    while (true)
		    {
			size_t start = pos;
			pos = line.find(' ', start);

			if (pos != std::string::npos)
			    line[pos] = 0;

			testLines.back().push_back(IndexedString(line.data() + start, stringIndex));

			if (pos == std::string::npos)
			    break;

			++pos;
		    }
		}
	    }

	    testSamples += testLines.size();
	}
	bool failed = false;
	std::cout << "testing " << testSamples << " word sequences" << std::endl;
	for (int sample = 0; sample < testSamples; ++sample) {
	    int length;

	    if (sample < (int)testLines.size())
		length = testLines[sample].size() + 2;
	    else
		length = random() % (2 * order) + 2;

	    std::vector<IndexedString> ngram(length);
	    std::vector<IndexedString> reversedNgram(length);

	    for (int n = 0; n < length; ++n)
	    {
		IndexedString word;

		if (sample < (int)testLines.size() && n > 0 && n - 1 < (int)testLines[sample].size())
		    word = testLines[sample][n - 1];
		else
		    word = *ngrams[0].get((rand() % ngrams[0].size()));

		// Due to the offsetting of the sentence probability, it is not properly reversed in our local reversed model. thus ignore sentence begin and end while testing.
		if (skipSentenceBegin) {
		    while (word == sentenceBegin || word == sentenceEnd)
			word = *ngrams[0].get((rand() % ngrams[0].size()));
		}else {
		    if (n == 0)
			word = sentenceBegin;
		    else if(n == length - 1)
			word = sentenceEnd;
		}

		IndexedString reversedWord = word;
		if (word == sentenceBegin)
		    reversedWord = sentenceEnd;
		else if (word == sentenceEnd)
		    reversedWord = sentenceBegin;

		ngram[n] = word;
		reversedNgram[length - n - 1] = reversedWord;
	    }

	    float forwardScore = sentenceScore(&ngram[0], length, ngrams, stringIndex, sample < (int)testLines.size());
	    float backwardScore = sentenceScore(&reversedNgram[0], length, newNGrams, stringIndex, sample < (int)testLines.size());

	    bool mismatch = std::abs(forwardScore - backwardScore) > 0.001;
	    if (mismatch || sample < (int)testLines.size())
	    {
		if (mismatch)
		    std::cout << "ERROR: Mismatch in forward/backward score for ";

		std::cout << "ngram '";

		for (int o = 0; o < length; ++o)
		{
		    if(o)
			std::cout << " ";
		    std::cout << ngram[o].get(stringIndex);
		}

		std::cout << "' forward score " << forwardScore << " backward score " << backwardScore << " (cross-check " << sentenceScore(&reversedNgram[0], length, ngrams, stringIndex) << ") " << std::endl;
		if (mismatch)
		    failed = true;
	    }
	}
	if (failed)
	    exit(11);

	std::cout << "success" << std::endl;
    }
}

}

#ifdef STANDALONE

int main(int argc, char** argv) {
    if(argc != 3 && argc != 4)
    {
	std::cerr << "usage: " << argv[0] << " arpa.in arpa.out [test_samples] / [test_file]" << std::endl;
	std::cout << "only either test_samples or test_file may be given. test_file must be a \
		      list of sentences (one per line), with words separated by spaces. Sentence \
		      start and end markers are inserted automatically.";

	exit(5);
    }

    std::string inName(argv[1]);
    std::string outName(argv[2]);

    int testSamples = 0;
    std::string testFile;

    if (argc == 4)
    {
	if (std::ifstream(argv[3]))
	{
	    testFile = argv[3];
	    std::cout << "will validate on file " << testFile << std::endl;
	}else{
	    testSamples = atoi(argv[3]);
	    std::cout << "will validate with " << testSamples << " random test ngrams" << std::endl;
	}
    }

    Lm::reverseArpaLm(inName, outName, testSamples, testFile);
    return 0;
}
#endif
