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
#ifndef _CORE_STRING_EXPRESSION_HH
#define _CORE_STRING_EXPRESSION_HH

#include <vector>
#include <Core/Utility.hh>
#include <Core/Configuration.hh>
#include <Core/Hash.hh>

namespace Core {

    /**
     * String parameter with variable parts
     *
     * A string expression consits of constant and variable tokens.
     * E.g.: "prefix-$(var).ext" where "var" is a variable
     *
     * @see StringExpressionParser for details about creating StringExpression object
     */

    class StringExpression {
    private:
	friend class StringExpressionParser;

	class Token {
	protected:
	    bool set_;
	    std::string value_;

	public:
	    Token() : set_(false) {}
	    Token(const std::string& value) : set_(true), value_(value) {}

	    void operator=(const std::string& v) { set_ = true; value_ = v; }
	    bool set() const { return set_; }
	    void clear() { set_ = false; value_ = ""; }
	    const std::string& value() const { verify(set_); return value_; }
	};

	typedef std::vector<Token> Tokens;
	typedef std::vector<u32> TokenReferences;
	typedef Core::StringHashMap<TokenReferences> Variables;

	Tokens tokens_;
	Variables variables_;

	void setValue(const TokenReferences &references, const std::string &value);

	void clear(const TokenReferences &references);

	void pushBackToken(const std::string &token);

	void pushBackVariableToken(const std::string &token);

    public:
	StringExpression() {}

	/** isConstant
	 * @return is true if the string expression does not contain any variables
	 */
	bool isConstant() const { return variables_.empty(); }

	bool hasVariable(const std::string &name) const {
	    return variables_.find(name) != variables_.end();
	}

	/** setVariable
	 * sets the value of a variable in the string expression which name is @name
	 * @return is false if variable does not exist
	 */
	bool setVariable(const std::string& name, const std::string& value);

	/** setVariable
	 * set the values of all variables in the string expression by querying the configuration @c
	 * @return is false if variable does not exist
	 */
	bool setVariables(const Core::Configuration &c);

	/** clear clears the value the variables @name
	 * @return is false if variable does not exist in @c
	 */
	bool clear(const std::string& name);

	/** clear clears the value of all variables
	 */
	void clear();

	/** value return the resolved paramerer in v
	 * @return is true if there are no unset variables in the parameter
	 */
	bool value(std::string &v) const;
    };


    /** Builder for StringExpression */

    class StringExpressionParser {
    private:
	StringExpression &toBuild_;
	std::string openTag_;
	std::string closeTag_;

    private:
	void clear();

    public:
	StringExpressionParser(StringExpression& toBuild,
			const std::string &openTag = "$(",
			const std::string &closeTag = ")") :
	    toBuild_(toBuild), openTag_(openTag), closeTag_(closeTag) {}

	bool accept(const std::string &stringExpression);
    };

    /** helper function to parse and create a StringExpression */
    StringExpression makeStringExpression(const std::string &stringExpression,
					  const std::string &openTag = "$(",
					  const std::string &closeTag = ")");
}

#endif // _FLOW_PARAMETER_HH
