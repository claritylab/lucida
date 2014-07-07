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

#include "Cluster.hh"

using namespace Cart;


void Cluster::write(std::ostream & out) const {
    out << "cluster " << node->id() << std::endl;
    out << std::right;
    u32 index = 1;
    for (ConstExampleRefList::const_iterator it = exampleRefs->begin();
	 it != exampleRefs->end(); ++it, ++index) {
	out << std::setw(3) << std::right << index << ". ";
	(*it)->write(out);
	out << std::endl;
    }
}

void Cluster::writeXml(Core::XmlWriter & xml) const {
    xml << Core::XmlOpen("cluster")
	+ Core::XmlAttribute("id", node->id());
    for (ConstExampleRefList::const_iterator it = exampleRefs->begin();
	 it != exampleRefs->end(); ++it)
	(*it)->writeXml(xml);
    xml << Core::XmlClose("cluster");
}

// ============================================================================
const Core::ParameterString ClusterList::paramClusterFilename(
    "cluster-file",
    "name of cluster file");

void ClusterList::write(std::ostream & out) const {
    out << "cluster-list:" << std::endl;
    map_->write(out);
    out << std::endl;
    for (ClusterList::const_iterator it = clusterRefs_.begin();
	 it != clusterRefs_.end(); ++it)
	(*it)->write(out);
}

void ClusterList::writeXml(Core::XmlWriter & xml) const {
    xml << Core::XmlOpen("cluster-list");
    map_->writeXml(xml);
    for (ClusterList::const_iterator it = clusterRefs_.begin();
	 it != clusterRefs_.end(); ++it)
	(*it)->writeXml(xml);
    xml << Core::XmlClose("cluster-list");
}

void ClusterList::writeToFile() const {
    std::string filename(paramClusterFilename(config));
    if (!filename.empty()) {
	log() << "write clusters to \"" << filename << "\"";
	Core::XmlOutputStream xml(new Core::CompressedOutputStream(filename));
	xml.generateFormattingHints(true);
	xml.setIndentation(4);
	xml.setMargin(78);
	writeXml(xml);
    } else {
	warning("cannot store clusters, because no filename is given");
    }
}
// ============================================================================
