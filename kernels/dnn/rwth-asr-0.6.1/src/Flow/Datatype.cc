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
// $Id: Datatype.cc 5439 2005-11-09 11:05:06Z bisani $

#include "Datatype.hh"

using namespace Flow;


bool Datatype::readGatheredData(
    Core::BinaryInputStream &i, std::vector<DataPtr<Data> > &data) const
{
    u32 n;
    i >> n;
    data.resize(n);
    for (std::vector<DataPtr<Data> >::iterator j = data.begin(); j != data.end(); ++j) {
	if (!(*j)) *j = DataPtr<Data>(newData());
	require((*j)->datatype() == this);
	if (!(*j)->read(i)) return false;
    }
    return true;
}

bool Datatype::writeGatheredData(
    Core::BinaryOutputStream &o, const std::vector<DataPtr<Data> > &data) const
{
    u32 n = data.size();
    o << n;
    for (std::vector<DataPtr<Data> >::const_iterator j = data.begin(); j != data.end(); ++j) {
	require((*j)->datatype() == this);
	if (!(*j)->write(o)) return false;
    }
    return true;
}

bool Datatype::readData(Core::BinaryInputStream &i, DataPtr<Data> &data) const
{
    DataPtr<Data> d(newData());
    require(d->datatype() == this);
    if (d->read(i)) {
	data = d;
	return true;
    }
    return false;
}

bool Datatype::writeData(Core::BinaryOutputStream &o, const DataPtr<Data> &data) const
{
    require(data->datatype() == this);
    return data->write(o);
}
