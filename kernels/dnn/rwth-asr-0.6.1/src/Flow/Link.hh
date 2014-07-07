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
#ifndef _FLOW_LINK_HH
#define _FLOW_LINK_HH

//#include <ostream.h>
#include <ostream>

#include <Core/Assertions.hh>

#include "Attributes.hh"
#include "Queue.hh"
#include "Types.hh"


namespace Flow {

    class AbstractNode;
    class Link {
    private:
	// connection
	AbstractNode *from_node_;
	PortId from_port_;
	AbstractNode *to_node_;
	PortId to_port_;

	// static information
	bool is_fast_;
	std::string from_node_name_, from_port_name_, to_node_name_, to_port_name_;

	// dynamic data
	u32 buffer_;
	Queue queue_;
	const Datatype *datatype_;
	Core::Ref<const Attributes> attributes_;
	Data* fast_data_;

	/** Represents the status of fast_data_.
	 *  fast_data_ can be either "empty" or occupied by a data or also by a
	 *  non-data object. Link does not differentiate data and non-data objects.
	 */
	static inline Data* sentinelEmpty() {
	    static Data sentinelEmpty_(1);
	    return &sentinelEmpty_;
	}
	static inline bool isEmpty(Data *t) {
	    return (t == sentinelEmpty());
	}
    public:
	Link();
	~Link();

	void clear();
	inline bool isDataAvailable() {
	    if (is_fast_) return (!isEmpty(fast_data_) || (!queue_.isEmpty()));
	    return (!queue_.isEmptyAtomar());
	}
	template<class T> inline bool getData(DataPtr<T> &d) {
	    if (is_fast_) {
		if (!isEmpty(fast_data_)) {
		    d.take(fast_data_);
		    fast_data_ = sentinelEmpty();
		} else {
		    // Link is fast and empty: it tipically happens if the "from" node
		    // did not produce a data in the last call to its work method.
		    verify(!queue_.isEmpty());
		    queue_.get(d);
		}
	    } else queue_.getBlocking(d);

	    return d;
	}

	/** Send data packet to down-stream Node.
	 * @return true on success (Note: Won't fail anyway.)
	 */
	inline bool putData(Data *d) {
	    /* node tried to send a packet which is not of advertised type */
	    require(d);
	    require(Data::isSentinel(d) || !datatype() || d->datatype() == datatype());

	    if (is_fast_) {
		if (queue_.isEmpty()) {
		    if (isEmpty(fast_data_)) {
			d->increment();
			fast_data_ = d;
			return true;
		    } else {
			fast_data_->decrement();
			queue_.put(fast_data_);
			fast_data_ = sentinelEmpty();
		    }
		}
		queue_.put(d);
		return true;
	    }
	    queue_.putAtomar(d);
	    return true;
	}

	/** Datatype as advertised by source node. */
	const Datatype *datatype() const { return datatype_; }
	void setDatatype(const std::string &dt);

	void setAttributes(Core::Ref<const Attributes> a);
	Core::Ref<const Attributes> attributes() const { return attributes_; }
	void eraseAttributes() { attributes_.reset(); }
	bool areAttributesAvailable() const { return (bool)attributes_; }
	void setNodeNames(std::string from_n, std::string from_p,
		std::string to_n, std::string to_p) {
		 from_node_name_ = from_n;
		 from_port_name_ = from_p;
		 to_node_name_ = to_n;
		 to_port_name_ = to_p;
	}
	void getNodeNames(std::string * const from_n, std::string *from_p,
		std::string *to_n, std::string *to_p) const {
		*from_n = from_node_name_;
		*from_p = from_port_name_;
		*to_n = to_node_name_;
		*to_p = to_port_name_;
	}

	void configure();
	inline bool isFast() { return is_fast_; }
	inline void setFromNode(AbstractNode *n) { from_node_ = n; }
	inline void setFromPort(PortId id) { from_port_ = id; }
	inline void setToNode(AbstractNode *n) { to_node_ = n; }
	inline void setToPort(PortId id) { to_port_ = id; }
	inline void setBuffer(u32 size) { buffer_ = size; }
	inline u32 getBuffer() { return buffer_; }

	inline AbstractNode* getFromNode() const { return from_node_; }
	inline PortId getFromPort() const { return from_port_; }
	inline AbstractNode* getToNode() const { return to_node_; }
	inline PortId getToPort() const { return to_port_; }

	friend std::ostream& operator << (std::ostream &o, const Link &l);
	friend std::ostream& operator << (std::ostream &o, const Link *l) { return o << *l; }
    };

} // namespace Flow

#endif // _FLOW_LINK_HH
