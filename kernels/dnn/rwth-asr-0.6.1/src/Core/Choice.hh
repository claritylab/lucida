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
#ifndef _CORE_CHOICE_HH
#define _CORE_CHOICE_HH

#include <set>
#include <string>
#include <vector>

#include "Types.hh"


namespace Core {

    /**
     * Choice class
     *
     * A choice is the symbolic mapping between enumerations and strings.
     * Choices are used to specify enumerative parameters.
     **/

    class Choice {
    public:
	typedef s32 Value;
	typedef std::pair<std::string, Value> IdentifierValuePair;
	typedef std::vector<IdentifierValuePair> IdentifierValuePairList;

	static const Value IllegalValue;
	static const char* const IllegalIdentifier;
	static const char* endMark();
    private:
	class Item {
	private:
	    const std::string ident_;
	    Value value_;
	public:
	    Item(const char * ident, const Value value) : ident_(ident), value_(value) {}
	    const std::string & ident() const { return ident_; }
	    Value value() const { return value_; }
	    bool operator < (const Item& item) const { return ident_.compare(item.ident_) < 0; }
	    bool operator == (const Item& item) const { return (value_ == item.value_) && (ident_ == item.ident_); }
	};

	struct comp_by_value {
	    bool operator () (const Item& item1, const Item& item2) const { return (item1.value() < item2.value()); }
	};

    private:
	const std::string illegalIdentifier;
	std::set<Item> items_by_ident;
	std::set<Item, comp_by_value> items_by_value;

    public:
	Choice();

	/**
	 * Constructs the choice.
	 * The parameters are specified as a list of pairs of strings and
	 * enumeration (integer) values. varargs are used to allow for an
	 * arbitrary number of different choices. The last identifier must
	 * be 0 to mark the end of the list.
	 **/
	Choice(const char* ident, const Value value, ...);

	/**
	 * Constructs the choice.
	 **/
	Choice(const IdentifierValuePairList &);

	/**
	 * Constructs the choice.
	 * The vector of strings is mapped to a consecutive list of numbers
	 * starting with 0, i.e. the identifier is the string and the value
	 * equals the index.
	 **/
	Choice(const std::vector<std::string> &);

	void addChoice(const char*, Value);

	/** Add identifier for smalles unused value. */
	Value addChoice(const char*);

	/**
	 * Returns the number of different choices.
	 **/
	u32 nChoices() const { return items_by_ident.size(); }

	/**
	 * Get a list of possible values (may contain duplicates)
	 **/
	void getValues(std::vector<Value> &) const;

	/**
	 * Get a list of possible identifiers (may contain duplicates)
	 **/
	void getIdentifiers(std::vector<std::string> &) const;

	/**
	 * Returns the enumeration value of a choice or Choice::Illegal
	 * if ident is not a valid identifier.
	 * @param ident choice identifier
	 **/
	Value operator[] (const std::string& ident) const;

	/**
	 * Returns the identifier of a choice or an empty string
	 * if ident is not a valid identifier.
	 * @param value enumeration value
	 **/
	const std::string & operator[] (const Value& value) const;


	/**
	 * Returns true, iff both choices define
	 * exactly the same mapping.
	 * @param choice instance of Choice
	 **/
	bool operator==(const Choice & choice) const;

	/**
	 * Print a comma separated list of all identifiers.
	 **/
	void printIdentifiers(std::ostream&) const;

	/**
	 * Iterator
	 **/
	typedef std::set<Item>::const_iterator const_iterator;
	const_iterator begin() const { return items_by_ident.begin(); }
	const_iterator end() const { return items_by_ident.end(); }
    };

    /** Predefined Choice for boolean values.
     * (e.g. "true", "false, "yes", "no" ...). */
    extern const Choice boolChoice;

    /** Predefined Choice for "inifinite" numbers.
     * (e.g. "infinite", "-infinity", "unlimited" ...) */
    extern const Choice infinityChoice;

} // namespace Core

#endif // _CORE_CHOICE_HH
