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
 * Test cases for Test::Lexicon
 */

#include <Test/UnitTest.hh>
#include <Test/Lexicon.hh>

class TestLexicon : public Test::Fixture
{
public:
    void setUp();
protected:
    Test::Lexicon lexicon_;
};

void TestLexicon::setUp()
{
    lexicon_.addPhoneme("si", false);
    lexicon_.addPhoneme("a");
    lexicon_.addPhoneme("b");
    lexicon_.addPhoneme("c");
    lexicon_.addLemma("A", "a");
    lexicon_.addLemma("[SILENCE]", "si", "silence");
    lexicon_.addLemma("AC", "a  c");
}

TEST_F(Test, TestLexicon, AddPhoneme)
{
    EXPECT_NE(lexicon_.phonemeInventory()->phoneme("a"), 0);
    EXPECT_FALSE(lexicon_.addPhoneme("a"));
}

TEST_F(Test, TestLexicon, SpecialLemma)
{
    EXPECT_NE(lexicon_.specialLemma("silence"), 0);
}

TEST_F(Test, TestLexicon, Lemma)
{
    const Bliss::Lemma *l = lexicon_.lemma("AC");
    EXPECT_NE(l, 0);
    EXPECT_EQ(std::string(l->preferredOrthographicForm()), std::string("AC"));
    Bliss::Lemma::LemmaPronunciationRange lpr = l->pronunciations();
    EXPECT_NE(lpr.first, lpr.second);
    const Bliss::Pronunciation *pron = lpr.first->pronunciation();
    EXPECT_EQ(pron->length(), u32(2));
    EXPECT_EQ((*pron)[0], Bliss::Phoneme::Id(lexicon_.phonemeInventory()->phoneme("a")->id()));
    EXPECT_EQ((*pron)[1], Bliss::Phoneme::Id(lexicon_.phonemeInventory()->phoneme("c")->id()));
    ++lpr.first;
    EXPECT_EQ(lpr.first, lpr.second);
}
