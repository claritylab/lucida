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
#ifndef _FSA_ALPHABET_HH
#define _FSA_ALPHABET_HH

#include <string>
#include <Core/BinaryStream.hh>
#include <Core/Hash.hh>
#include <Core/ReferenceCounting.hh>
#include <Core/XmlStream.hh>
#include "Types.hh"
#include <Core/Vector.hh>

namespace Fsa
{

/**
 * The abstract definition of an alphabet. An alphabet serves the purpose
 * of mapping strings (the external representation of an alphabet) to
 * label ids (used internally for efficiency reasons). As all objects,
 * alphabets are reference counted and despite in the case you create
 * an alphabet on your own, you should only handle alphabets by using
 * the ConstAlphabetRef type.
 *
 * As alphabets are virtually infinte, this interface class defines
 * a lazy interface. Use the Alphabet::const_iterator to traverse an
 * alphabet if you need to.
 *
 * The empty string is not a valid symbol's name.
 **/
class Alphabet : public Core::ReferenceCounted
{
public:
	/**
	 * Iterator that should be used to traverse an alphabet. If you derive
	 * from Alphabet to create your own implementation of an on-demand
	 * alphabet you should NOT modify this iterator. Instead overload
	 * appropriate virtual methods at your Alphabet class. You should not
	 * assume any ordering on the label ids or strings you get.
	 **/
	class const_iterator
	{
	protected:
		Core::Ref<const Alphabet> alphabet_;
		LabelId label_;
	public:
		explicit const_iterator(Core::Ref<const Alphabet> a, LabelId i) :
			alphabet_(a), label_(i)
		{
		}
		const_iterator& operator++()
		{
			label_ = alphabet_->next(label_);
			return *this;
		}
		bool operator==(const const_iterator &i) const
		{
			return (label_ == i.label_);
		}
		bool operator!=(const const_iterator &i) const
		{
			return (label_ != i.label_);
		}

		/**
		 * Retrieve the string representation of the current symbol.
		 */
		std::string operator*()
		{
			return alphabet_->symbol(label_);
		}

		/**
		 * Retrieve the label id of the current symbol.
		 */
		operator LabelId()
		{
			return label_;
		}
		operator LabelId() const
		{
			return label_;
		}
	};
public:
	virtual ~Alphabet()
	{
	}

	/**
	 * Retrieve a string representation of a special label id.
	 * Special labels are default to all alphabets. This methods is NOT
	 * virtual as you are not allowed to change it.
	 * @param index the label id of the special symbol
	 * @return the string representation of the special symbol
	 **/
	std::string specialSymbol(LabelId index) const;

	/**
	 * Map a string representation of a special symbol to its label id.
	 * Special labels are default to all alphabets. This methods is NOT
	 * virtual as you are not allowed to change it.
	 * @param symbol the string representation of the special symbol
	 * @return the label id of the special symbol
	 **/
	LabelId specialIndex(const std::string &symbol) const;

	/**
	 * Returns an iterator pointing to the first symbol. Note, that if
	 * the alphabet contains a hole in the space of label ids at the beginning
	 * the iterator must be winded to the first existing symbol.
	 * @return a const iterator pointing to the first symbol or end()
	 **/
	virtual const_iterator begin() const
	{
		return const_iterator(Core::Ref<const Alphabet>(this), 0);
	}

	/**
	 * Returns an iterator pointing behind the last symbol. This
	 * usually does not need to be overloaded except for the case
	 * you need a different internal representation of the end
	 * iterator. Currently, InvalidLabelId is being used as end
	 * iterator.
	 * @return a const iterator pointing to the first symbol or end()
	 **/
	virtual const_iterator end() const {
		return const_iterator(Core::Ref<const Alphabet>(this), InvalidLabelId);
	}

	/**
	 * Returns the label id of the next symbol in sequence. Users can savely
	 * assume that holes in the space of label ids are being skipped
	 * automatically.
	 * @param id the current symbol
	 * @return the label id of the next symbol
	 **/
	virtual LabelId next(LabelId id) const {return ++id;}

	/**
	 * Returns the string representation of a symbol.
	 * @param index the symbol's label id
	 * @return the string representation of the symbol or an empty string
	 *    if there is no symbol with that label id
	 **/
	virtual std::string symbol(LabelId index) const = 0;

	/**
	 * Given a string representation of a symbol returns the label id.
	 * @param symbol the string representation of a symbol
	 * @return the label id of the symbol or InvalidLabelId if the string
	 *    represents no valid symbol
	 **/
	virtual LabelId index(const std::string &symbol) const;

	/**
	 * Determine wether a symbol is a disambiguator (see
	 * determinize for more details on the use of disambiguators).
	 * @param index the symbol's label id
	 * @return true if the symbol is a disambiguator.
	 **/
	virtual bool isDisambiguator(LabelId index) const;

	/**
	 * return symbol tag
	 * @param index the symbol's label id
	 * @return symbol's tag
	 **/
	virtual AlphabetTag tag(LabelId index) const;

	/**
	 * Write the alphabet to BinaryOutputStream.
	 * @param o the BinaryOutputStream
	 * @return true if the operation was successfull for all symbols,
	 *    false otherwise
	 **/
	virtual bool write(Core::BinaryOutputStream &o) const;

	/**
	 * Write the alphabet to an XmlWriter.
	 * @param o the XmlWriter
	 * @return true if the operation was successfull for all symbols,
	 *    false otherwise
	 **/
	virtual void writeXml(Core::XmlWriter &o) const;

	/**
	 * Return the amount of memory used by this object.
	 * @return the amount of memory in bytes
	 **/
	virtual size_t getMemoryUsed() const {return 0;}
};

/**
 * @typedef ConstAlphabetRef
 * This abstract smart pointer should be used to handle alphabets in your
 * program.
 **/
typedef Core::Ref<const Alphabet> ConstAlphabetRef;

/**
 * The implementation of a statically allocated alphabet.  All
 * methods are overloaded appropriately. Static alphabets are
 * used automatically when reading automata from disk.
 **/
class StaticAlphabet : public Alphabet
{
private:

	struct Symbol
	{
		std::string name_;
		//bool isDisambiguator_;
		// this is due to the binary format which only holds 8 bits for a disambiguator
		AlphabetTag tag_;
		Symbol() {}
		//Symbol(const std::string &name, bool isDisambiguator = false) :
		//  name_(name), tag_(isDisambiguator ? disambiguatorTag : 0) {}
		Symbol(const std::string &name, AlphabetTag tag = 0) :
			name_(name), tag_(tag) {}
	};
	Core::Vector<Symbol> symbols_;
	Core::hash_map<std::string, LabelId, Core::StringHash> indices_;
public:
	StaticAlphabet();

	/**
	 * Clears the whole alphabet. Resets it to a state right after
	 * initialization.
	 **/
	void clear();

	/**
	 * Adds a symbol to the alphabet.
	 * @param symbol the symbol
	 * @param isDisambiguator mark this symbol as a disambiguator (see
	 *   determinize for more information on disambiguation symbols)
	 * @return the index of the newly added symbol or InvalidLabelId
	 *   if the string was not a valid symbol
	 **/
	//LabelId addSymbol(const std::string &symbol, bool isDisambiguator = false);
	LabelId addSymbol(const std::string &symbol, AlphabetTag tag = 0);

	/**
	 * Adds a symbol to the alphabet together with an already known,
	 * associated index.
	 * @param symbol the symbol
	 * @param index the associated index
	 * @param isDisambiguator mark this symbol as a disambiguator (see
	 *   determinize for more information on disambiguation symbols)
	 * @return false if the string was not a valid symbol or the index
	 *   was already assigned to a different symbol
	 **/
	//bool addIndexedSymbol(const std::string &symbol, LabelId index, bool isDisambiguator = false);
	bool addIndexedSymbol(const std::string &symbol, LabelId index, AlphabetTag tag = 0);

	virtual const_iterator begin() const;
	virtual const_iterator end() const {return const_iterator(ConstAlphabetRef(this), symbols_.size());}
	virtual LabelId next(LabelId id) const;
	virtual std::string symbol(LabelId index) const;
	virtual LabelId index(const std::string &symbol) const;
	virtual bool isDisambiguator(LabelId index) const;
	virtual AlphabetTag tag(LabelId index) const;
	virtual bool read(Core::BinaryInputStream &i);
	virtual size_t getMemoryUsed() const;
};

/**
 * Create a deep copy of an alphabet. As alphabets are usually
 * reference counted to reduce memory and runtime requirements
 * this call *really* copies an alphabet.
 **/
Core::Ref<StaticAlphabet> staticCopy(ConstAlphabetRef);

} // namespace Fsa

#endif // _FSA_ALPHABET_HH
