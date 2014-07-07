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
#include <iostream>

#include "Link.hh"
#include "Node.hh"
#include "Registry.hh"

using namespace Flow;

/*****************************************************************************/
Link::Link()
/*****************************************************************************/
{
    from_node_ = 0;
    from_port_ = IllegalPortId;
    to_node_ = 0;
    to_port_ = IllegalPortId;
    is_fast_ = false;
    buffer_ = 0;
    datatype_ = 0;
    fast_data_ = sentinelEmpty();
}

/*****************************************************************************/
Link::~Link()
/*****************************************************************************/
{
    clear();
}

/*****************************************************************************/
void Link::clear()
/*****************************************************************************/
{
    queue_.clear();

    if (!isEmpty(fast_data_)) {
	if (fast_data_->decrement())
	    fast_data_->free();
	fast_data_ = sentinelEmpty();
    }
}

/*****************************************************************************/
std::ostream& Flow::operator<< (std::ostream &o, const Link &l)
/*****************************************************************************/
{
    bool dumped = false;
    if (l.from_node_) {
	o << "node: '" << l.from_node_->name() << "' port: " << l.from_port_;
	dumped = true;
    }
    if (l.to_node_) {
	if (dumped) o << " ";
	o << "node: '" << l.to_node_->name() << "' port: " << l.to_port_;
	dumped = true;
    }
    if (l.buffer_) {
	if (dumped) o << " ";
	o << "buffer: " << l.buffer_;
	dumped = true;
    }
    if (l.is_fast_) {
	if (dumped) o << " ";
	o << "fast";
	dumped = true;
    }
    return o;
}

/*****************************************************************************/
void Link::configure()
/*****************************************************************************/
{
    if (getFromNode())
	is_fast_ = !getFromNode()->isThreaded();
}

/*****************************************************************************/
void Link::setDatatype(const std::string &dt)
/*****************************************************************************/
{
    if (dt.empty()) {
	datatype_ = 0;
    } else {
	datatype_ = Flow::Registry::instance().getDatatype(dt);
	require(datatype_); // node advertised unknown data type
    }
}

/*****************************************************************************/
void Link::setAttributes(Core::Ref<const Attributes> a)
/*****************************************************************************/
{
    ensure(a);

    attributes_ = a;
    if (attributes_)
	setDatatype(attributes_->get("datatype"));
}
