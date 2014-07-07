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
#ifndef _CORE_MAP_PARSER_HH
#define _CORE_MAP_PARSER_HH

#include <Core/StringUtilities.hh>
#include "XmlBuilder.hh"

namespace Core {

    /** XmlMapElement implements XmlBuilderElement for an arbitrary XmlSchemaParser
     *  which parses a map.
     *
     *  Structure of map-element
     *    <"map-name">  <"map-item" key="xxx" data="xxx" ... /> ... </"map-name">
     */
    template<class Map>
    class XmlMapElement :
	public XmlBuilderElement<Map,
				 XmlRegularElement,
				 CreateByContext>
    {
    public:
	typedef XmlBuilderElement<Map,
				  XmlRegularElement,
				  CreateByContext> Predecessor;
	typedef XmlMapElement<Map> Self;
	typedef Map* (XmlContext::*CreationHandler)(const XmlAttributes atts);
    private:
	XmlEmptyElement *mapEntryElement_;
	const char *keyAttributeName_;
	const char *dataAttributeName_;
    protected:
	virtual void newEntry(const XmlAttributes atts);
    public:
	XmlMapElement(XmlContext *context, CreationHandler newMap,
		      const char *name = "map",
		      const char *itemName = "map-item",
		      const char *keyAttributeName = "key",
		      const char *dataAttributeName = "data");
	virtual ~XmlMapElement();
    };

    template<class Map>
    XmlMapElement<Map>::XmlMapElement(XmlContext *context, CreationHandler newMap,
				      const char *name, const char *itemName,
				      const char *keyAttributeName, const char *dataAttributeName) :
	Predecessor(name, context, newMap),
	mapEntryElement_(0),
	keyAttributeName_(keyAttributeName),
	dataAttributeName_(dataAttributeName)
    {
	mapEntryElement_ = new XmlEmptyElementRelay(
	    itemName, this, XmlEmptyElementRelay::startHandler(&Self::newEntry));

	Predecessor::addTransition(Predecessor::initial, Predecessor::initial, mapEntryElement_);
	Predecessor::addFinalState(Predecessor::initial);
    }

    template<class Map>
    XmlMapElement<Map>::~XmlMapElement()
    {
	delete mapEntryElement_;
    }

    template<class Map>
    void XmlMapElement<Map>::newEntry(const XmlAttributes atts)
    {
	const char *keyValue = atts[keyAttributeName_];
	if (keyValue == 0) {
	    this->parser()->error("Map entry must have an attribute \"%s\"", keyAttributeName_);
	    return;
	}

	std::stringstream keyStream(keyValue);
	typename Map::key_type key;
	keyStream >> key;

	const char *dataValue = atts[dataAttributeName_];
	if (dataValue == 0) {
	    this->parser()->error("Map entry must have an attribute \"%s\"", dataAttributeName_);
	    return;
	}

	std::stringstream dataStream(dataValue);
	typename Map::data_type data;
	dataStream >> data;

	(*Predecessor::product_)[key] = data;
    }


    /** XML document containing one single map. */
    template<class Map>
    class XmlMapDocument : public XmlSchemaParser {
	typedef XmlSchemaParser Predecessor;
	typedef XmlMapDocument Self;
    private:
	XmlMapElement<Map> *mapElement_;
	Map &map_;
	Map* pseudoCreateMap(const XmlAttributes atts) { return &map_; }
    public:
	XmlMapDocument(const Configuration &c, Map &map,
		       const char *name, const char *itemName,
		       const char *keyAttributeName, const char *dataAttributeName);
	virtual ~XmlMapDocument();
    };

    template<class Map>
    XmlMapDocument<Map>::XmlMapDocument(const Configuration &c, Map &map,
					const char *name, const char *itemName,
					const char *keyAttributeName, const char *dataAttributeName) :
	Predecessor(c),
	mapElement_(0),
	map_(map)
    {
	mapElement_ = new XmlMapElement<Map>(
	    this, XmlMapElement<Map>::creationHandler(&Self::pseudoCreateMap),
	    name, itemName, keyAttributeName, dataAttributeName);
	setRoot(mapElement_);
    }

    template<class Map>
    XmlMapDocument<Map>::~XmlMapDocument()
    {
	delete mapElement_;
    }

} //namespace Core

#endif // _CORE_MAP_PARSER_HH
