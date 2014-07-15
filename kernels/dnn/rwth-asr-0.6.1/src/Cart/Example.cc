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
#include <Core/CompressedStream.hh>

#include "Example.hh"
#include "Parser.hh"

using namespace Cart;


// ============================================================================
/**
 * \todo the hash function should be checked/improved some time
 */
size_t FloatBox::HashFcn::operator()(const FloatBox & val) const {
    size_t hash = 0;
    for (FloatBox::const_vector_iterator it = val.begin();
	 it != val.end(); ++it)
	hash = size_t(*it) ^ (hash << 4 | hash >> (sizeof(size_t) * 8 - 4));
    return hash;
}

FloatBox & FloatBox::operator+=(const FloatBox & values) {
    require(size_ == values.size());
    const_vector_iterator itSrc = values.begin();
    for (vector_iterator itTrg = begin_; itTrg != end_; ++itTrg, ++itSrc)
	*itTrg += *itSrc;
    return *this;
}

bool FloatBox::operator==(const FloatBox & values) const {
    if (size_ != values.size())
	return false;
    const_vector_iterator it1, it2;
    for (it1 = begin_, it2 = values.begin();
	 ((it1 != end_) && (*it1 == *it2)); ++it1, ++it2);
    return (it1 == end_);
}

bool FloatBox::operator<(const FloatBox & values) const {
    if (size_ != values.size())
	return (size_ < values.size());
    const_vector_iterator it1, it2;
    for (it1 = begin_, it2 = values.begin();
	 ((it1 != end_) && (*it1 == *it2)); ++it1, ++it2);
    return ((it1 != end_) && (*it1 < *it2));
}

void FloatBox::writeValues(std::ostream & os) const {
    os << std::setiosflags(std::ios::scientific) << std::setprecision(12);
    for (const_row_vector_iterator it = begin_row();
	 it != end_row(); ++it) {
	for (const_row_iterator itt = it.begin();
	     itt != it.end(); ++itt)
	    os << *itt << " ";
	os << "\n";
    }
    os << std::resetiosflags(std::ios::scientific);
}

void FloatBox::write(std::ostream & os) const {
    os << rows_ << " " << columns_ << "\n";
    writeValues(os);
}

void FloatBox::writeXml(Core::XmlWriter & xml) const {
    xml << Core::XmlOpen("matrix-f64")
	+ Core::XmlAttribute("nRows", rows_)
	+ Core::XmlAttribute("nColumns", columns_);
    writeValues(xml);
    xml << Core::XmlClose(std::string("matrix-f64"));
}
// ============================================================================


// ============================================================================
void Example::write(std::ostream & os) const {
    os << "example" << std::endl
       << "observation(s) : " << nObs << std::endl;
    properties->write(os);
    os << "values:" << std::endl;
    values->write(os);
}

void Example::writeXml(Core::XmlWriter & os) const {
    os << Core::XmlOpen("example")
	+ Core::XmlAttribute("nObservations", nObs);
    properties->writeXml(os);
    values->writeXml(os);
    os << Core::XmlClose("example");
}
// ============================================================================


// ============================================================================
const Core::ParameterString ExampleList::paramExampleFilename(
    "example-file",
    "name of example file");
const Core::ParameterStringVector ExampleList::paramExampleFilenamesToMerge(
    "merge-example-files",
    "name of example files to merge");

bool ExampleList::loadFromString(const std::string & str) {
    XmlExampleListParser parser(config);
    return parser.parseString(str, this);
}

bool ExampleList::loadFromStream(std::istream & i) {
    XmlExampleListParser parser(config);
    return parser.parseStream(i, this);
}

bool ExampleList::loadFromFile(std::string filename) {
    if (filename.empty()) {
	filename = paramExampleFilename(config);
	verify(!filename.empty());
    }
    log() << "load example list from \"" << filename << "\"";
    XmlExampleListParser parser(config);
    return parser.parseFile(filename, this);
}

bool ExampleList::mergeFromFiles(std::vector<std::string> filenames) {
    if (filenames.empty()) {
	filenames = paramExampleFilenamesToMerge(config);
	verify(!filenames.empty());
    }
    Cart::XmlExampleListMerger merger(config, this);
    for (std::vector<std::string>::const_iterator itFilename = filenames.begin();
	 itFilename != filenames.end(); ++itFilename) {
	log() << "merge example list from \"" << *itFilename << "\"";
	if (!merger.parseFile(*itFilename))
	    return false;
    }
    return true;
}

void ExampleList::write(std::ostream & out) const {
    out << "example-list:" << std::endl;
    map_->write(out);
    out << std::endl;
    out << std::right;
    u32 index = 1;
    for (const_iterator it = exampleRefs_.begin();
	 it != exampleRefs_.end(); ++it) if (*it) {
	    out << std::setw(3) << std::right << index << ". ";
	    (*it)->write(out);
	    out << std::endl;
	    ++index;
	}
}

void ExampleList::writeXml(Core::XmlWriter & xml) const {
    xml << Core::XmlOpen("example-list");
    map_->writeXml(xml);
    for (const_iterator it = exampleRefs_.begin();
	 it != exampleRefs_.end(); ++it)
	if (*it) (*it)->writeXml(xml);
    xml << Core::XmlClose("example-list");
}

void ExampleList::writeToFile() const {
    std::string filename = paramExampleFilename(config);
    if (!filename.empty()) {
	log() << "write example list to \"" << filename << "\"";
	Core::XmlOutputStream xml(new Core::CompressedOutputStream(filename));
	xml.generateFormattingHints(true);
	xml.setIndentation(4);
	xml.setMargin(78);
	writeXml(xml);
    } else {
	warning("cannot store examples, because no filename is given");
    }
}

// ============================================================================
