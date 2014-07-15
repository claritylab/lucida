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
#include "Dump.hh"

#include <Core/Directory.hh>

using namespace Flow;

const Core::ParameterString DumpNode::paramFilename("file", "file name");
const Core::ParameterBool DumpNode::paramUnbuffered("unbuffered", "false");

DumpNode::DumpNode(const Core::Configuration &c) :
    Core::Component(c),
    SleeveNode(c),
    unbuffered_(paramUnbuffered(c))
{
    open(paramFilename(c));
}

DumpNode::~DumpNode() {
    if (isOpen_)
		close();
}

void DumpNode::open(const std::string &filename) {
    if (!filename.empty()) {
		close();
		//strip of the dirname part and create directory
		if(!Core::createDirectory(Core::directoryName(filename)))
			warning("Could not create directory for dump file '%s'.", filename.c_str());
		f_.open(filename);
		if ((isOpen_ = f_.good()))
			f_ << Core::XmlOpen(filterName());
		else
			error("Could not open dump file '%s'.", filename.c_str());
	}
}


void DumpNode::close() {
    if (isOpen_) {
		f_ << Core::XmlClose(filterName());
		f_.close();
    }
    isOpen_=false;
}


bool DumpNode::setParameter(const std::string &name, const std::string &value) {
    if (paramFilename.match(name))
		open(value);
    else if (paramUnbuffered.match(name))
		unbuffered_ = paramUnbuffered(value);
    else
		return false;
    return true;
}


bool DumpNode::work(PortId output) {
    DataPtr<Data> d;
    getData(0, d);

    if (isOpen_ && d) {
		d->dump(f_);
		if (unbuffered_)
			f_.flush();
    }
    return putData(0, d.get());
}
