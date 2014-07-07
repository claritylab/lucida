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
#ifndef _CORE_OBJECT_CACHE_HH
#define _CORE_OBJECT_CACHE_HH

#include "Hash.hh"
#include <list>
#include <unistd.h>
#include "Parameter.hh"
#include "Component.hh"
#include "BinaryStream.hh"
#include "XmlStream.hh"
#include "Directory.hh"
#include "ReferenceCounting.hh"

namespace Core {

    /**
     * ObjectCacheItem: class handling the data_ object (read, write, dirty)
     */

    template<class Data> class ObjectCacheItem
    {
    private:
	Data *data_;
	bool dirty_;
    private:
	ObjectCacheItem(const ObjectCacheItem<Data> &v) {}
	ObjectCacheItem<Data>& operator=(const ObjectCacheItem<Data> &v) { return *this; }
    public:
	ObjectCacheItem(Data *data) : data_(data), dirty_(true) { ensure(data); }
	~ObjectCacheItem() {  delete data_; }

	const Data& readAccess() const { return *data_; }
	Data& writeAccess() { dirty_ = true; return *data_; }

	bool dirty() const { return dirty_; }

	void read(BinaryInputStream &is) { data_->read(is); dirty_ = false; }
	void write(BinaryOutputStream &os) { data_->write(os); dirty_ = false; }
	void dump(XmlOutputStream &os, const std::string &name) const;
    };

    template<class Data>
    void ObjectCacheItem<Data>::dump(XmlOutputStream &os, const std::string &name) const {
	os << XmlOpen("dump-cached-object") + Core::XmlAttribute("name", name);
	data_->dump(os);
	os << XmlClose("dump-cached-object");
    }

    /**
     * MruObjectCacheList: list of ObjectCacheItems implementing the
     *                      most recently used strategy
     *
     * ObjectCacheItems are stored in a hash map by their keys.
     * Order of ObjectCacheItems is implemented by a linked list of keys.
     *
     * Each ObjectCacheItem points to its corresponding element in the linked list.
     * If an element is accessed the corresponding element is moved (erase, push_front)
     * to the beginning of the linked list.
     * last() returns last key in the linked list.
     */
    template<class _Key,
	     class _Data,
	     class _HashFunction,
	     class _EqualKey>
    class MruObjectCacheList;

    template<class _Key,
	     class _Data,
	     class _HashFunction,
	     class _EqualKey>
    class MruObjectCacheList
    {
    public:
	typedef MruObjectCacheList<_Key, _Data, _HashFunction, _EqualKey> Self;
	typedef _Key Key;
	typedef _Data Data;

	typedef std::list<Key> MruList;

	/** ObjectCacheItem extended by an index in the MRU list */
	class Item : public ObjectCacheItem<Data> {
	    friend class MruObjectCacheList<_Key, _Data, _HashFunction, _EqualKey>;
	    typename MruList::iterator mruListItem_;
	public:
	    Item(Data *data) : ObjectCacheItem<Data>(data) {}
	};

	typedef Core::hash_map<Key, Item*, _HashFunction, _EqualKey> HashMap;

	typedef typename HashMap::value_type ValueType;
	typedef typename HashMap::iterator Iterator;
    private:
	HashMap table_;
	MruList mruList_;
    public:
	MruObjectCacheList() {}
	~MruObjectCacheList() { clear(); }

	Iterator access(const Key &key);
	Iterator insert(const Key &key, Data* data);
	Iterator find(const Key &key) { return table_.find(key); }

	void erase(Iterator item);
	void clear();

	Iterator last() {
	    Iterator result = find(mruList_.back()); ensure(result != end()); return result;
	}

	Iterator begin() { return table_.begin(); }
	Iterator end() { return table_.end(); }

	u32 size() const { return table_.size(); }
    };

    template<class _Key, class _Data, class _HashFunction, class _EqualKey>
    typename MruObjectCacheList<_Key, _Data, _HashFunction, _EqualKey>::Iterator
    MruObjectCacheList<_Key, _Data, _HashFunction, _EqualKey>::access(const Key &key)
    {
	Iterator result = find(key);
	if (result != end()) {
	    mruList_.erase(result->second->mruListItem_);
	    mruList_.push_front(key);
	    result->second->mruListItem_ = mruList_.begin();
	}
	return result;
    }

    template<class _Key, class _Data, class _HashFunction, class _EqualKey>
    typename MruObjectCacheList<_Key, _Data, _HashFunction, _EqualKey>::Iterator
    MruObjectCacheList<_Key, _Data, _HashFunction, _EqualKey>::insert(const Key &key, Data* data)
    {
	std::pair<Iterator, bool> result = table_.insert(std::make_pair(key, new Item(data)));
	verify(result.second);

	mruList_.push_front(key);
	result.first->second->mruListItem_ = mruList_.begin();
	return result.first;
    }

    template<class _Key, class _Data, class _HashFunction, class _EqualKey>
    void MruObjectCacheList<_Key, _Data, _HashFunction, _EqualKey>::erase(Iterator item)
    {
	mruList_.erase(item->second->mruListItem_);
	delete item->second;
	table_.erase(item);
    }


    template<class _Key, class _Data, class _HashFunction, class _EqualKey>
    void MruObjectCacheList<_Key, _Data, _HashFunction, _EqualKey>::clear()
    {
	for(typename HashMap::iterator i = begin(); i != end(); ++ i)
	    delete i->second;

	mruList_.clear();
	table_.clear();
    }

    /**
     * RandomObjectCacheList: list of ObjectCacheItems implementing the random  strategy
     *
     * ObjectCacheItems are stored in a hash map by their keys.
     * A deque is used to enable random access to keys and fast removal of keys.
     *
     * Each ObjectCacheItem points to its corresponding element in the deque.
     * Erasing at the end of a deque is much faster then erasing somewhere
     * in the middle of the deque:
     *   -Erasing an element swaps the element and the last element in the deque.
     *   -The corresponding ObjectCacheItem of the last element is updated to the
     *   -new position of its deque element. The new last element and the ObjectCacheItem
     *   -are erased.
     * last() chooses a random element of the deque..
     */
    template<class _Key,
	     class _Data,
	     class _HashFunction,
	     class _EqualKey>
    class RandomObjectCacheList;

    template<class _Key,
	     class _Data,
	     class _HashFunction,
	     class _EqualKey>
    class RandomObjectCacheList
    {
    public:
	typedef RandomObjectCacheList<_Key, _Data, _HashFunction, _EqualKey> Self;
	typedef _Key Key;
	typedef _Data Data;

	typedef std::deque<Key> Keys;


	class Item : public ObjectCacheItem<Data> {
	    friend class RandomObjectCacheList<_Key, _Data, _HashFunction, _EqualKey>;
	    u32 keyIndex_;
	public:

	    Item(Data *data) : ObjectCacheItem<Data>(data) {}
	};


	typedef Core::hash_map<Key, Item*, _HashFunction, _EqualKey> HashMap;

	typedef typename HashMap::value_type ValueType;
	typedef typename HashMap::iterator Iterator;
    private:
	HashMap table_;
	Keys keys_;
    private:
	void copyBack(u32 destination) {
	    if (destination != size() - 1) {
		keys_[destination] = keys_.back();
		Iterator item = find(keys_.back());
		verify_(item != end());
		item->second->keyIndex_ = destination;
	    }
	}
    public:
	RandomObjectCacheList() {}
	~RandomObjectCacheList() { clear(); }

	Iterator access(const Key &key) { return find(key); }
	Iterator insert(const Key &key, Data *data);
	Iterator find(const Key &key) { return table_.find(key); }

	void erase(Iterator item);
	void clear();

	Iterator last() {
	    size_t randomIndex = size_t(f64(size()) * f64(rand()) / f64(RAND_MAX + 1.0));
	    Iterator result = find(keys_[randomIndex]); ensure(result != end()); return result;
	}

	Iterator begin() { return table_.begin(); }
	Iterator end() { return table_.end(); }

	u32 size() const { return table_.size(); }
    };


    template<class _Key, class _Data, class _HashFunction, class _EqualKey>
    typename RandomObjectCacheList<_Key, _Data, _HashFunction, _EqualKey>::Iterator
    RandomObjectCacheList<_Key, _Data, _HashFunction, _EqualKey>::insert(const Key &key, Data *data)
    {
	std::pair<Iterator, bool> result = table_.insert(std::make_pair(key, new Item(data)));
	verify(result.second);

	keys_.push_back(key);
	result.first->second->keyIndex_ = size() - 1;
	return result.first;
    }

    template<class _Key, class _Data, class _HashFunction, class _EqualKey>
    void RandomObjectCacheList<_Key, _Data, _HashFunction, _EqualKey>::erase(Iterator item)
    {
	copyBack(item->second->keyIndex_);

	keys_.pop_back();
	delete item->second;
	table_.erase(item);
    }

    template<class _Key, class _Data, class _HashFunction, class _EqualKey>
    void RandomObjectCacheList<_Key, _Data, _HashFunction, _EqualKey>::clear()
    {
	for(typename HashMap::iterator i = begin(); i != end(); ++ i)
	    delete i->second;

	keys_.clear();
	table_.clear();
    }

    const ParameterInt parameterObjectCacheSize
    ("size", "max number of objects in the cache memory", 1, 1);
    const ParameterString parameterObjectCacheDirectory
    ("directory", "directory of cached objects");
    const ParameterBool parameterObjectCacheDump
    ("dump", "on/off dumping of objects", false);

    const u8 reuseObjectCacheMode = 0;
    const u8 appendObjectCacheMode = 1;
    const u8 createObjectCacheMode = 2;

    const std::string objectCacheDumpExtension(".dump");


    /** ObjectCache: implements the caching of larger objects.
     *
     * Datas are created by the ObjectCache.
     * Datas can be accessed by their keys. Access is devided into readAccess
     * and writeAccess to minimize the number of file operations.
     * If more then maxSize_ object are referenced the last object in the CacheList
     * is written back to its file if necesseary and it is removed from the memory.
     * The last object is given by the cache strategy implemented by the CacheList. The
     * files are stored in one directory. The file names are the keys of the Datas.
     * Datas can be explicitly saved (writeBack) and explicitly removed (erase)
     * from the memory.
     * Store references to Datas with care! References are valid until the next access to
     * the cache.
     *
     */
    template<class List> class ObjectCache : public virtual Component
    {
    private:
	typedef typename List::Key Key;
	typedef typename List::Data Data;
	typedef typename List::Iterator ListIterator;
    private:
	List list_;
	u32 maxSize_;
	std::string directoryName_;
	u8 mode_;
	bool dump_;
    private:
	std::string createFilename(const Key &key) const;
	std::string fileExtension() const { return name().empty() ? "" : "." + name(); }
	bool openInputStream(const Key &key, BinaryInputStream &is);

	/** Saves the object stored by @param item and removes the @param item from the list_.
	 *  Note: iterators on the list_ gets invalid after a call to this funciton.
	 */
	void writeBack(ListIterator item);
	ListIterator access(const Key &key);

	bool parameterName(std::string &n);
    public:
	/** ObjectCache
	 * @param mode controls handling of files:
	 *        reuseObjectCacheMode does not allow creation of new objects
	 *        appendObjectCacheMode reuses existing objects and allows creation of new objects
	 *        createObjectCacheMode deletes exisiting objects and create new ones
	 */
	ObjectCache(const Configuration &c,
		    u8 mode = createObjectCacheMode);
	~ObjectCache() { writeBack(); }

	/** @return is a constant pointer to the object of the key if
	 * the object is in the cache or is stored in the cache directory;
	 * otherwise return is zero pointer.
	 */
	const Data* findForReadAccess(const Key &key) {
	    ListIterator item;
	    return (item = access(key)) != list_.end() ? &(item->second->readAccess()) : 0;
	}
	/** @return is a pointer to the object of the key if
	 * the object is in the cache or is stored in the cache directory;
	 * otherwise return is zero pointer.
	 */
	Data* findForWriteAccess(const Key &key) {
	    ListIterator item;
	    return (item = access(key)) != list_.end() ? &(item->second->writeAccess()) : 0;
	}

	/** Inserts a new object into the cache.
	 *  @return is false if the @param key already exists.
	 */
	bool insert(const Key &key, Data *data);

	/** saves the object of the key and removes it from the memory */
	void writeBack(const Key &key);
	/** saves all objects and removes them from the memory*/
	void writeBack();

	/** clears the memory and and the directory */
	void clear();

	/** return a 'std::set' of all the keys, both in memory and on disk. */
	std::set<Key> keys()
	{
	    std::set<Key> result;

	    if (!name().empty()) {
		DirectoryFileIterator::FileExtensionFilter filter(fileExtension());
		for (DirectoryFileIterator file(directoryName_, &filter); file; ++ file)
		    result.insert(file.path().substr(0, file.path().size() - fileExtension().size()));
	    }

	    for(typename List::Iterator i = list_.begin(); i != list_.end(); ++ i)
		result.insert(i->first);

	    return result;
	}

	/** getDirectory
	 */
	std::string getDirectory() const { return directoryName_; }
	/** setDirectory
	 * @param directoryName is the directory storing the objects
	 */
	void setDirectory(const std::string &directoryName);
	/** setMaxSize
	 * @param maxSize is max number of objects in the memory
	 */
	void setMaxSize(u32 maxSize);

	/** size
	 * @return is the number of objects in the memory
	 */
	u32 size() { return list_.size(); }

	/** For each object a *.dump file is created in XML format */
	void setDump(bool shouldDump) { dump_ = shouldDump; }

	/** supports parametrization by name
	 * syntax of parameter name (as one would do in a configuration file:
	 * cacheName.parameter-name = "something"
	 */
	bool setParameter(const std::string &name, const std::string &value);
    };

    template<class List>
    ObjectCache<List>::ObjectCache(const Configuration &c,
				   u8 mode) :
	Component(c),
	maxSize_(0),
	mode_(mode),
	dump_(false)
    {
	setMaxSize(parameterObjectCacheSize(c));
	setDirectory(parameterObjectCacheDirectory(c));
	setDump(parameterObjectCacheDump(c));
    }

    template<class List>
    std::string ObjectCache<List>::createFilename(const Key &key) const
    {
	std::ostringstream stringKey;
	stringKey << key;

	return joinPaths(directoryName_, stringKey.str() + fileExtension());
    };

    template<class List>
    void ObjectCache<List>::writeBack(ListIterator item)
    {
	if (item->second->dirty()) {
	    std::string filename = createFilename(item->first);

	    std::string directory = directoryName(filename);
	    createDirectory(directory);
	    if (!isDirectory(directory)) {
		error("Could not create directory '%s'.", directory.c_str());
		return;
	    }

	    BinaryOutputStream os(filename);
	    if (!os.isOpen() || !os.good()) {
		error("Could not create file '%s'.", filename.c_str());
		return;
	    }
	    item->second->write(os);
	    if (!os.good()) {
		error("Could not save object into file '%s'.", filename.c_str());
		return;
	    }

	    if (dump_) {
		XmlOutputStream xmlOs(filename + objectCacheDumpExtension);
		xmlOs.setIndentation(4);
		item->second->dump(xmlOs, item->first);
	    }
	}
	list_.erase(item);
    }

    template<class List>
    bool ObjectCache<List>::openInputStream(const Key &key, BinaryInputStream &is)
    {
	std::string filename = createFilename(key);
	is.clear();
	is.open(filename);
	return (is.isOpen() && !isDirectory(filename));
    }

    template<class List>
    typename ObjectCache<List>::ListIterator ObjectCache<List>::access(const Key &key)
    {
	ListIterator item = list_.access(key);
	if (item == list_.end()) {
	    BinaryInputStream is;
	    if (openInputStream(key, is)) {
		while(list_.size() >= maxSize_)
		    writeBack(list_.last());

		item = list_.insert(key, new Data);
		item->second->read(is);
		if (!is.good())
		    error("Could not read object.");
	    }
	}
	return item;
    }

    template<class List>
    bool ObjectCache<List>::insert(const Key &key, Data* data)
    {
	if (access(key) == list_.end()) {
	    while(list_.size() >= maxSize_)
		writeBack(list_.last());

	    list_.insert(key, data);
	    return true;
	}
	return false;
    }

    template<class List>
    bool ObjectCache<List>::parameterName(std::string &n)
    {
	if (n.find(name() + ".") == 0) {
	    n = n.substr(name().size() + 1);
	    return true;
	}
	return false;
    }

    template<class List>
    void ObjectCache<List>::writeBack(const Key &key)
    {
	ListIterator item = list_.find(key);
	if (item != list_.end())
	    writeBack(item);
    }

    template<class List>
    void ObjectCache<List>::writeBack()
    {
	while(list_.size() > 0)
	    writeBack(list_.begin());
    }

    template<class List>
    void ObjectCache<List>::clear()
    {
	list_.clear();
	if (!name().empty()) {
	    DirectoryFileIterator::FileExtensionFilter filter(fileExtension());
	    for (DirectoryFileIterator file(directoryName_, &filter); file; ++ file)
		unlink(joinPaths(file.base(), file.path()).c_str());
	}
    }

    template<class List>
    void ObjectCache<List>::setDirectory(const std::string &directoryName)
    {
	writeBack();
	list_.clear();

	directoryName_ = directoryName;

	if (mode_ == createObjectCacheMode)
	    clear();
    }

    template<class List>
    void ObjectCache<List>::setMaxSize(u32 maxSize)
    {
	maxSize_ = maxSize;
	while(list_.size() > maxSize_)
	    writeBack(list_.last());
    }

    template<class List>
    bool ObjectCache<List>::setParameter(const std::string &name, const std::string &value)
    {
	std::string n(name);
	if (!parameterName(n))
	    return false;

	if (parameterObjectCacheSize.match(n))
	    setMaxSize(parameterObjectCacheSize(value));
	else if (parameterObjectCacheDirectory.match(n))
	    setDirectory(parameterObjectCacheDirectory(value));
	else if (parameterObjectCacheDump.match(n))
	    setDump(parameterObjectCacheDump(value));
	else
	    return false;

	return true;
    }
}

#endif // _CORE_OBJECT_CACHE_HH
