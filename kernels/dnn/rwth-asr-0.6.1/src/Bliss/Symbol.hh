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
// $Id: Symbol.hh 9621 2014-05-13 17:35:55Z golik $

#ifndef _BLISS_SYMBOL_HH
#define _BLISS_SYMBOL_HH

#include <Core/Hash.hh>
#include <Core/Obstack.hh>
#include <Core/XmlStream.hh>
#include <cstring>
#include <iostream>

namespace Bliss {

    /**
     * Symbols are character strings which are collected in a
     * SymbolSet.  Copying and equality test is very efficient.  Two
     * Symbols from different SymbolSets are un-equal, even if their
     * strings are the same (This is a feature, not a bug!).  A
     * special void symbol is created by the default constructor, any
     * other symbol can only be created by a SymbolSet or by copying.
     *
     * Implementation: A Symbol is just a char pointer into the
     * SymbolSet.  Since a SymbolSet contains each character string at
     * most once, Symbols can be compared and hashed very efficiently
     * by looking at the pointers only.
     */

    class Symbol {
    public:
	typedef char Char;
	typedef std::string String;
    private:
	const Char *s;
    protected:
	friend class SymbolSet;
	explicit Symbol(const Char *_s) : s(_s) {}
    public:
	Symbol() : s(0) {}
	Symbol(const Symbol &_s) : s(_s.s) {}

	bool operator==(const Symbol &rhs) const { return s == rhs.s; }
	bool operator!=(const Symbol &rhs) const { return s != rhs.s; }
	size_t length() const { return strlen(s); }

	/** Test if symbol is not void. */
	operator bool () const { return s != 0; }
	/** Test if symbol is void. */
	bool operator!() const { return s == 0; }

	/** C string (zero-terminated character pointer) */
	const Char* str() const { return s; }

	/** C++ string */
	operator String () const { return String(s); }

	static Symbol cast(void *p) {
	    return Symbol(reinterpret_cast<char*>(p));
	}

	friend std::ostream &operator<< (std::ostream &os, const Bliss::Symbol &s) {
	    if (s)
		os.write(s.str(), s.length());
	    else
		os.write("(null)", 6);
	    return os;
	}
	friend Core::XmlWriter &operator<< (Core::XmlWriter &xw, const Bliss::Symbol &s) {
	    std::ostream &os(xw);
	    if (s)
		os.write(s.str(), s.length());
	    else
		os.write("(null)", 6);
	    return xw;
	}

	struct Hash {
	    size_t operator() (Symbol s) const {
		return size_t(s.s);
	    }
	};
	struct Equality {
	    bool operator() (Symbol s1, Symbol s2) const {
		return s1.s == s2.s;
	    }
	};
    };

    /**
     * A pool of Symbols.  A SymbolSet owns the memory of all its
     * symbols.  CAVEAT: When the SymbolSet is deleted, all derived
     * Symbols become invalid.
     */
    class SymbolSet {
    private:
	Core::Obstack<Symbol::Char> strings_;
	typedef Core::hash_set<const Symbol::Char*, Core::StringHash, Core::StringEquality> Map;
	Map map_;
    public:
	SymbolSet();
	/** Convert char pointer to Symbol.
	 * Symbol is added if not already present. */
	Symbol operator[] (const Symbol::Char*);
	/** Convert string to Symbol.
	 * Symbol is added if not already present. */
	Symbol operator[] (const Symbol::String&);

	/** return void symbol if not present */
	Symbol get(const Symbol::Char*) const;
	Symbol get(const Symbol::String&) const;

	bool contains(const Symbol::Char*) const;
	bool contains(const Symbol::String&) const;
    };

    template <typename T_Value>
    class SymbolHashMap :
	public Core::hash_map<Symbol, T_Value, Symbol::Hash, Symbol::Equality>
    {};

    class Token {
    public:
	typedef s32 Id;
	static const Id invalidId = -1;
    private:
	Id id_;
	Symbol symbol_;
    protected:
	Token(Id _id, Bliss::Symbol _symbol) : id_(_id), symbol_(_symbol) {}
	Token(Id _id) : id_(_id) {}
	Token(Bliss::Symbol _symbol) : id_(invalidId), symbol_(_symbol) {}
	Token() : id_(invalidId) {}
	friend class TokenInventory;
	void setId(Id id) { id_ = id; }
	void setSymbol(Symbol s) { symbol_ = s; }
    public:
	virtual ~Token() {}

	/** A unique string identifier for the token. */
	Symbol symbol() const { return symbol_; }

	/** The numerical ID of the token. */
	Id id() const { return id_; }
    };

    class TokenInventory {
	typedef std::vector<Token*> List;
	List list_;
	typedef Core::hash_map<const Symbol::Char*, Token*, Core::StringHash, Core::StringEquality> Map;
	Map map_;
    public:
	~TokenInventory() {
	    for (List::const_iterator tt = list_.begin() ; tt != list_.end() ; ++tt)
		delete *tt;
	}

	void insert(Token *token) {
	    token->setId(list_.size());
	    list_.push_back(token);
	}
	void link(Symbol symbol, Token *token) {
	    require_(token == list_[token->id()]);
	    map_[symbol.str()] = token;
	}
	void add(Token *token) {
	    insert(token);
	    link(token->symbol(), token);
	}

	Token *operator[] (Token::Id id) const {
	    require(0 <= id && id < Token::Id(list_.size()));
	    Token * result = list_[id];
	    ensure_(result->id() == id);
	    return result;
	}
	Token *operator[] (const std::string &sym) const {
	    Map::const_iterator i = map_.find(sym.c_str());
	    return (i != map_.end()) ? i->second : 0;
	}
	Token *operator[] (Symbol sym) const {
	    Map::const_iterator i = map_.find(sym.str());
	    return (i != map_.end()) ? i->second : 0;
	}

	u32 size() const {
	    return list_.size();
	}

	typedef Token *const *Iterator;
	Iterator begin() const { return &(*list_.begin()); }

	// Note: Eliminating the "-1"..."+1" construct causes Sprint to fail in debug mode.
	Iterator end()   const { return (&(*(list_.end()-1)) +1);   }
    };

    template <typename T>
    class TokenMap {
    public:
	typedef T Value;
	typedef typename std::vector<Value>::iterator iterator;
    private:
	std::vector<Value> store_;
    public:
	TokenMap(const TokenInventory &ti) :
	    store_(ti.size() + 1) {}

	void fill(const Value &v) {
	    std::fill(store_.begin(), store_.end(), v);
	}

	iterator begin() {
	    return store_.begin();
	}

	iterator end() {
	    return store_.end();
	}

	void operator= (const TokenMap<T> &map) {
	    require_(store_.size() == map.store_.size());
	    store_ = map.store_;
	}

	Value &operator[](const Token *t) {
	    require_(t);
	    verify_(t->id() < store_.size());
	    return store_[t->id()];
	}

	const Value &operator[](const Token *t) const {
	    require_(t);
	    verify_(t->id() < store_.size());
	    return store_[t->id()];
	}
    };

    template <class Symbol> class SymbolSequenceSet;

    template <class S = Symbol>
    class SymbolSequence {
    public:
	typedef S Symbol;
	typedef const Symbol *Iterator;
	typedef size_t Size;
    private:
	const Symbol *begin_, *end_;
    protected:
	friend class SymbolSequenceSet<Symbol>;
    public: // FIXME
	SymbolSequence(
	    const Symbol *_begin,
	    const Symbol *_end):
	    begin_(_begin), end_(_end) {}

    public:
	SymbolSequence() : begin_(0), end_(0) {}
	SymbolSequence(const SymbolSequence &o) : begin_(o.begin_), end_(o.end_) {}

	bool valid() const {
	    return begin_ != 0;
	}

	Size size() const {
	    return end_ - begin_;
	}

	Size length() const { return size(); }

	bool isEpsilon() const {
	    return length() == 0;
	}

	const Symbol &operator[](Size i) const {
	    require_(valid());
	    require_(0 <= i && i < size());
	    return begin_[i];
	}

	const Symbol &front() const { require_(begin_ < end_); return *begin_; }
	const Iterator begin() const { return begin_; }
	const Iterator end()   const { return end_;   }
    };

    template <class S = Symbol>
    class SymbolSequenceSet {
    public:
	typedef S Symbol;
	typedef typename Symbol::String String;
	typedef SymbolSequence<Symbol> Sequence;
    private:
	SymbolSet &symbolSet_;
	Core::Obstack<Symbol> sequences_;
    public:
	SymbolSequenceSet(SymbolSet &ss) : symbolSet_(ss) {}

	Sequence add(const std::vector<String> &seq) {
	    sequences_.start();
	    for (std::vector<std::string>::const_iterator i = seq.begin(); i != seq.end(); ++i)
		sequences_.grow(symbolSet_[*i]);
	    Sequence result(sequences_.currentBegin(), sequences_.currentEnd());
	    sequences_.finish();
	    return result;
	}

	Sequence add(const String &sym) {
	    Symbol *s = sequences_.add(symbolSet_[sym]);
	    return Sequence(s, s+1);
	}

	Sequence add(Symbol sym) {
	    Symbol *s = sequences_.add(sym);
	    return Sequence(s, s+1);
	}
    };


} // namespace Bliss

#endif //_BLISS_SYMBOL_HH
