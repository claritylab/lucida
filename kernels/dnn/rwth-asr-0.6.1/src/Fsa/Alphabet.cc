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
#include "Alphabet.hh"

namespace Fsa
{

std::string Alphabet::specialSymbol(LabelId index) const
{
	switch (index) {
	case Epsilon:
		return "*EPS*";
	case Any:
		return "*ANY*";
	case Failure:
		return "*FAIL*";
	case Else:
		return "*ELSE*";
	default:
		break;
	}
	return std::string();
}

LabelId Alphabet::specialIndex(const std::string &symbol) const
{
	if (symbol == "*EPS*")
		return Epsilon;
	else if (symbol == "*ANY*")
		return Any;
	else if (symbol == "*FAIL*")
		return Failure;
	else if (symbol == "*ELSE*")
		return Else;
	return InvalidLabelId;
}

LabelId Alphabet::index(const std::string &symbol) const
{
	// Important to inform user, since compose() and other
	// functions that use mapOutput() and friends give false
	// results for alphabet that dont implement this function.
	// Consider using defect().
	std::cerr
			<< "critical error: this alphabet does not support symbol-to-index mapping."
			<< std::endl;
	return InvalidLabelId;
}

bool Alphabet::isDisambiguator(LabelId index) const
{
	// Default implemetation: There are no disambiguators.
	return false;
}

AlphabetTag Alphabet::tag(LabelId index) const
{
	// Default implemetation: There are no tags.
	return 0;
}

bool Alphabet::write(Core::BinaryOutputStream &o) const
{
	for (const_iterator i = begin(); i != end(); ++i) {
		if (!(o << LabelId(i)))
			return false;
		if (!(o << *i))
			return false;
		if (!(o << u8(isDisambiguator(LabelId(i)) ? 1 : 0)))
			return false;
	}
	o << InvalidLabelId;
	return true;
}

void Alphabet::writeXml(Core::XmlWriter &o) const
{
	for (const_iterator i = begin(); i != end(); ++i) {
		o << Core::XmlOpen("symbol") + Core::XmlAttribute("index", LabelId(i));
		if (isDisambiguator(LabelId(i)))
			o << Core::XmlEmpty("disambiguator");
		o << *i << Core::XmlClose("symbol") << "\n";
	}
}

StaticAlphabet::StaticAlphabet()
{
	clear();
}

void StaticAlphabet::clear()
{
	symbols_.erase(symbols_.begin(), symbols_.end());
	indices_.erase(indices_.begin(), indices_.end());
	indices_["*EPS*"] = Epsilon;
	indices_["*ANY*"] = Any;
	indices_["*FAIL*"] = Failure;
	indices_["*ELSE*"] = Else;
}

LabelId StaticAlphabet::addSymbol(const std::string &symbol, AlphabetTag tag)
{
	if (symbol.empty())
		return InvalidLabelId;
	LabelId index = this->index(symbol);
	if (index != InvalidLabelId)
		return index;
	index = symbols_.size();
	symbols_.push_back(Symbol(symbol, tag));
	indices_[symbol] = index;
	return index;
}

bool StaticAlphabet::addIndexedSymbol(const std::string &symbol, LabelId index,
		AlphabetTag tag)
{
	if (symbol.empty())
		return false;
	if (!this->symbol(index).empty())
		return false;
	if (this->index(symbol) != InvalidLabelId)
		return false;
	symbols_.grow(index);
	symbols_[index].name_ = symbol;
	symbols_[index].tag_ = tag;
	indices_[symbol] = index;
	return true;
}

Alphabet::const_iterator StaticAlphabet::begin() const
{
	LabelId id = 0;
	while ((size_t(id) < symbols_.size()) && (symbols_[id].name_.empty()))
		++id;
	return const_iterator(ConstAlphabetRef(this), id);
}

LabelId StaticAlphabet::next(LabelId id) const
{
	++id;
	while ((size_t(id) < symbols_.size()) && (symbols_[id].name_.empty()))
		++id;
	if (size_t(id) >= symbols_.size())
		return symbols_.size();
	return id;
}

std::string StaticAlphabet::symbol(LabelId index) const
{
	switch (index) {
	case Epsilon:
		return "*EPS*";
	case Any:
		return "*ANY*";
	case Failure:
		return "*FAIL*";
	case Else:
		return "*ELSE*";
	default:
		if ((index == InvalidLabelId) || (size_t(index) >= symbols_.size()))
			return std::string();
		return symbols_[index].name_;
	}
}

LabelId StaticAlphabet::index(const std::string &symbol) const
{
	Core::hash_map<std::string, LabelId, Core::StringHash>::const_iterator i =
			indices_.find(symbol);
	if (i != indices_.end())
		return i->second;
	return InvalidLabelId;
}

bool StaticAlphabet::isDisambiguator(LabelId index) const
{
	if ((index == InvalidLabelId) || (size_t(index) >= symbols_.size()))
		return false;
	return symbols_[index].tag_ == alphabetTagDisambiguator;
}
AlphabetTag StaticAlphabet::tag(LabelId index) const
{
	if ((index == InvalidLabelId) || (size_t(index) >= symbols_.size()))
		return 0;
	return symbols_[index].tag_;
}
bool StaticAlphabet::read(Core::BinaryInputStream &i)
{
	clear();
	LabelId index;
	if (!(i >> index))
		return false;
	while (index != InvalidLabelId) {
		std::string symbol;
		if (!(i >> symbol))
			return false;
		u8 tag;
		if (!(i >> tag))
			return false;
		switch (index) {
		case Epsilon:
		case Any:
		case Failure:
			break;
		default:
			//Core::stripWhitespace(symbol);
			if (symbol.empty())
				std::cerr
						<< "empty symbol string is not support in alphabet.\n"
						<< std::endl;
			if (!addIndexedSymbol(symbol, index, tag)) {
				std::cerr << "duplicate symbol '" << symbol
						<< "' in alphabet.\n" << std::endl;
				return false;
			}
		}
		if (!(i >> index))
			return false;
	}
	symbols_.minimize();
	return true;
}

size_t StaticAlphabet::getMemoryUsed() const
{
	return sizeof(LabelId) + symbols_.getMemoryUsed();
}

Core::Ref<StaticAlphabet> staticCopy(ConstAlphabetRef aa)
{
	StaticAlphabet *result = new StaticAlphabet;
	for (Alphabet::const_iterator ss = aa->begin(); ss != aa->end(); ++ss) {
		result->addIndexedSymbol(*ss, ss, aa->tag(ss));
	}
	return ref(result);
}

} // namespace Fsa
