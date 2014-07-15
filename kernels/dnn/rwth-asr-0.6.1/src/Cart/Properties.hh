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
#ifndef _CART_PROPERTIES_HH
#define _CART_PROPERTIES_HH

#include <Core/Assertions.hh>
#include <Core/Choice.hh>
#include <Core/Component.hh>
#include <Core/ReferenceCounting.hh>
#include <Core/Types.hh>
#include <Core/XmlStream.hh>

#include "Conditions.hh"


namespace Cart {

    /**
       The property map fullfills two tasks.
       First it provides a mapping between key/value string and key/value index.
       Second it defines the set of possible properties, i.e. key/value-pairs.

       <properties-definition>
	   <key> central </key> <value-map> <value id="-1"> # </value> <value id="0"> a </value> </value-map>
	   <!--
	   <key> x       </key> <value-range> 0.0 - 1.0 </value-range>
	   -->
	   ...
       </properties-definition>
    */
    class PropertyMap :
		public Core::ReferenceCounted {
    public:
		typedef Core::Choice::Value Index;
		typedef std::string String;
		typedef std::vector<std::string> StringList;
		typedef std::vector<StringList> ListOfStringList;
		typedef std::pair<std::string, Index> IndexedString;
		typedef std::vector<IndexedString> IndexedStringList;
		typedef std::vector<IndexedStringList> ListOfIndexedStringList;

    protected:
		typedef Core::Choice Choice;
		typedef std::vector<Choice *> ChoicePtrList;

    public:
		const Index       undefinedIndex;
		const std::string undefinedString;

    protected:
		Choice * keys_;
		ChoicePtrList * values_;

    protected:
		void reset();

    public:
		PropertyMap() :
			undefinedIndex(Core::Choice::IllegalValue),
			undefinedString(Core::Choice::IllegalIdentifier),
			keys_(0),
			values_(0) {}

		PropertyMap(
			const StringList & keys,
			const ListOfIndexedStringList & values) :
			undefinedIndex(Core::Choice::IllegalValue),
			undefinedString(Core::Choice::IllegalIdentifier) ,
			keys_(0),
			values_(0) {
			set(keys, values);
		}

		virtual ~PropertyMap() {
			reset();
		}

		virtual void set(const StringList & keys,
						 const ListOfIndexedStringList & values);

		// true, iff keys_ (and values_) are not set
		bool empty() const {
			return keys_ == 0;
		}

		size_t size() const {
			require_(keys_);
			return keys_->nChoices();
		}

		Index key(const std::string & key) const {
			require_(keys_);
			return (*keys_)[key];
		}

		const std::string & key(const Index keyIndex) const {
			require_(keys_);
			return (*keys_)[keyIndex];
		}

		bool exists(const std::string & key) const {
			require_(keys_);
			return (*keys_)[key] != Core::Choice::IllegalValue;
		}

		bool exists(const Index keyIndex) const {
			require_(keys_);
			return (0 <= keyIndex) && (keyIndex < Index(size()));
		}

		const Choice & operator[](const std::string & key) const {
			require_(keys_);
			Index keyIndex = (*keys_)[key];
			require(exists(keyIndex));
			return *((*values_)[keyIndex]);
		}

		const Choice & operator[](const Index keyIndex) const {
			require_(keys_);
			require(exists(keyIndex));
			return *((*values_)[keyIndex]);
		}

		bool isDefined(const std::string & s) const {
			return s.c_str() != Core::Choice::IllegalIdentifier;
		}

		bool isDefined(const Index i) const {
			return i != Core::Choice::IllegalValue;
		}

		bool operator==(const PropertyMap & map) const;

		void write(std::ostream & out) const;

		void writeXml(Core::XmlWriter & xml) const;
    };

    typedef Core::Ref<const PropertyMap> PropertyMapRef;



    // ============================================================================
    /**
       Find and log differences in two PropertyMaps
       ATTENTION: diff makes a symbolic comparison,
       it doesn't care for different indices for different symbols;
       in terms of max: this is a feature not a bug as properties are
       stored in their symbolic forms
    */

    class PropertyMapDiff :
		public Core::Component {
    private:
		/*
		  1 = property unique to left map
		  2 = property unique to right map
		  4 = property not unique but different values
		*/
		const PropertyMap & map1_;
		const PropertyMap & map2_;
		u16 differences_;

    private:
		bool equal(const std::string & key);
		bool equalSymbolic(const std::string & key);

		void logKey(
			Core::XmlWriter & xml,
			const std::string & name,
			const PropertyMap & map,
			std::string key);

		void logUniq(
			Core::XmlWriter & xml,
			std::string key,
			u16 diff);

		void logDiff(
			Core::XmlWriter & xml,
			std::string key);

    public:
		PropertyMapDiff(
			const Core::Configuration & config,
			const PropertyMap & map1,
			const PropertyMap & map2,
			bool compareSymbolic);

		u16 differences() { return differences_; }
		bool hasDifferences() { return (differences_ != 0); }
    };



    // ============================================================================
    /**
       Abstract set of properties, i.e. key/value pairs.

       For a given key index the value index is returned
       or the invalid value if either the key is
       not known or the value for that key is not set.

       <properties>
	   <key> history[0] </key>
	   <key> central    </key> <value> a </value>
	   ...
       </properties>
    */
    class Properties {
    public:
		typedef PropertyMap::Index Index;

    protected:
		const PropertyMapRef map_;

    public:
		Properties(PropertyMapRef map) :
			map_(map) {}
		virtual ~Properties() {}

		const PropertyMap & map() const { return *map_; }

		PropertyMapRef getMap() const { return map_; }

		virtual Index operator[](const Index keyIndex) const = 0;

		virtual const std::string & operator[](const std::string & key) const;

		/*
		  prototype for extension towards support of continous
		  properties

		  virtual f64 getNumericalProperty(const Index keyIndex) = 0;

		  f64 getNumericalProperty(const std::string & key) {
		  Index keyIndex = map_->key(name);
		  return getNumericalProperty(keyIndex);
		  }
		*/

		bool operator==(const Properties & props) const;

		void write(std::ostream & out) const;

		void writeXml(Core::XmlWriter & xml) const;

		/*
		  hash function
		*/
		struct HashFcn {
			size_t operator()(const Properties & props) const;
		};

		/*
		  pointer
		*/
		struct PtrEqual {
			bool operator()(const Properties * const props1, const Properties * const props2) const {
				return *props1 == *props2;
			}
		};

		struct PtrHashFcn {
			HashFcn hasher;
			size_t operator()(const Properties * const props) const {
				return hasher(*props);
			}
		};
    };



    // ============================================================================
    /**
       This class does explicitly store the properties.
    */
    class StoredProperties :
		public Properties {
    public:
		typedef std::vector<std::string> StringList;

    private:
		std::vector<Index> valueIndexes_;

    public:
		StoredProperties(const Properties & props);

		StoredProperties(
			PropertyMapRef map,
			const StringList & keys,
			const StringList & values);

		const std::string & operator[](const std::string & key) const {
			return Properties::operator[](key);
		}

		Index operator[](const Index keyIndex) const {
			if (map_->exists(keyIndex))
				return valueIndexes_[keyIndex];
			else
				return map_->undefinedIndex;
		}
    };

} // namespace Cart

inline std::ostream & operator<<(std::ostream & out, const Cart::PropertyMap & p) {
    p.write(out);
    return out;
}

inline std::ostream & operator<<(std::ostream & out, const Cart::Properties & p) {
    p.write(out);
    return out;
}

#endif // _CART_PROPERTIES_HH
