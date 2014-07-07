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
 * Examples for Sprint Test Cases using TEST()
 *
 * The test cases themself are not very useful and do not fully verify
 * the functionality of the tested methods.
 */

#include <Core/StringUtilities.hh>
#include <Test/UnitTest.hh>

namespace Core {

using std::string;

TEST(Core, StringUtilities, StripWhiteSpace) {
  string a = "  abc";
  stripWhitespace(a);
  EXPECT_EQ(a, string("abc"));

  string b = "abc   ";
  stripWhitespace(b);
  EXPECT_EQ(b, string("abc"));

  string c = "   abc   ";
  stripWhitespace(c);
  EXPECT_EQ(c, string("abc"));

  string d = " \t  abc  \n";
  stripWhitespace(d);
  EXPECT_EQ(d, string("abc"));
}

TEST(Core, StringUtilities, Split) {
  string s = "abc def ghi jklmn";
  std::vector<string> v = split(s, " ");
  EXPECT_EQ(v.size(), size_t(4));
  EXPECT_EQ(v[0], string("abc"));
  EXPECT_EQ(v[1], string("def"));
  EXPECT_EQ(v[2], string("ghi"));
  EXPECT_EQ(v[3], string("jklmn"));
}

TEST(Core, StringUtilities, StartsWith) {
	EXPECT_TRUE(startsWith(string("abc"),string("a")));
	EXPECT_TRUE(startsWith(string("abc"),string("ab")));
	EXPECT_TRUE(startsWith(string("abc"),string("abc")));
	EXPECT_FALSE(startsWith(string("abc"),string("abcd")));
	EXPECT_FALSE(startsWith(string("abc"),string("x")));
	EXPECT_FALSE(startsWith(string("abc"),string("abx")));
	EXPECT_FALSE(startsWith(string("abc"),string("axc")));
	EXPECT_FALSE(startsWith(string("abc"),string("xbc")));
}

TEST(Core, StringUtilities, Form) {
  string a = form("%d", 1);
  EXPECT_EQ(a, string("1"));
  string b = form("%0.2f", .23);
  EXPECT_EQ(b, string("0.23"));
  string c = form("%d %.4f", 10, 1.2345);
  EXPECT_EQ(c, string("10 1.2345"));
}

}
