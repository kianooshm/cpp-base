#ifndef CPP_BASE_DATA_STRUCT_LRU_MAP_H_
#define CPP_BASE_DATA_STRUCT_LRU_MAP_H_

#include <algorithm>
#include <hash_map>
#include <limits>
#include <list>
#include <utility>
#include <vector>

#include "cpp-base/hash/hash.h"
#include "cpp-base/macros.h"
#include "cpp-base/util/map_util.h"

namespace cpp_base {

// This class provides a bounded-size map which guarantees to never exceed
// the given capacity; the least-recently used entry is removed to ensure this.
// Lookup/insertion/deletion are all done in O(1) like a regular hash map;
// it's hash-map-based so there is no API to iterate over keys in sorted order.
// This class is *not* thread safe; concurrency must be provided externally.
template <class KeyType, class ValueType>
class LruMap {
  private:
    typedef std::list<std::pair<KeyType, ValueType>> ListType;
    typedef hash_map<KeyType, typename ListType::iterator> MapType;
    ListType list_;
    MapType map_;
    int64 capacity_;

  public:
    LruMap() : capacity_(std::numeric_limits<int64>::max()) {}
    explicit LruMap(int64 capacity) : capacity_(capacity) {}
    ~LruMap() {}

    // Use this setter only at initialization time. Do not use when the map is full.
    void SetCapacity(int64 capacity) { capacity_ = capacity; }

    bool Empty() const { return map_.empty(); }
    size_t Size() const { return map_.size(); }
    bool Contains(const KeyType& key) const { return ContainsKey(map_, key); }

    void Clear() {
        list_.clear();
        map_.clear();
    }

    // Returns false iff map is empty.
    bool OldestKey(KeyType* key) const {
        if (list_.empty())
            return false;
        *key = list_.back().first;
        return true;
    }

    // Returns false iff map is empty.
    bool OldestValue(ValueType* value) const {
        if (list_.empty())
            return false;
        *value = list_.back().second;
        return true;
    }

    // Insert the given key and value. If the key existed, it is reinserted at
    // list head. Returns whether the key already existed.
    bool Put(const KeyType& key,
             const ValueType& value,
             ValueType* evicted_value = nullptr) {
        typename MapType::iterator map_it = map_.find(key);
        if (map_it == map_.end()) {
            CHECK_LE(map_.size(), capacity_);
            if (map_.size() == capacity_)
                CHECK(EvictOldest(evicted_value));
            list_.push_front(std::pair<KeyType, ValueType>(key, value));
            map_[key] = list_.begin();
            CHECK_LE(map_.size(), capacity_);
            return false;
        } else {
            // Remove the existing entry from list_, insert a new one at the
            // head of list_, and put an iterator to it in map_.
            typename ListType::iterator list_it = map_it->second;
            CHECK(list_it->first == key);
            list_it->second = value;
            if (list_it != list_.begin()) {
                list_.splice(list_.begin(), list_, list_it);
                map_it->second = list_.begin();
            }
            CHECK_LE(map_.size(), capacity_);
            return true;
        }
    }

    // Gets the value mapped for the given key. Does not reinsert the key at
    // list head. 'value' cannot be nullptr.
    bool GetWithoutTouch(const KeyType& key, ValueType* value) const {
        typename MapType::const_iterator map_it = map_.find(key);
        if (map_it == map_.end())
            return false;
        typename ListType::const_iterator list_it = map_it->second;
        CHECK(list_it->first == key);
        *value = list_it->second;
        return true;
    }

    // Returns whether the map contains 'key'. If so, it is re-inserted at list
    // head and optionally the mapped value is copied to 'value', if a non-null
    // pointer is given. If the key does not exist in the map, it is *not*
    // inserted.
    bool GetWithTouch(const KeyType& key, ValueType* value /*can be nullptr*/) {
        typename MapType::iterator map_it = map_.find(key);
        if (map_it == map_.end()) {
            return false;
        }
        // Find the list entry, move to the head, and update the list iterator
        // in the map.
        typename ListType::iterator list_it = map_it->second;
        CHECK(list_it->first == key);
        if (value != nullptr)
            *value = list_it->second;
        if (list_it != list_.begin()) {
            list_.splice(list_.begin(), list_, list_it);
            map_it->second = list_.begin();
        }
        return true;
    }

    bool Touch(const KeyType& key) { return GetWithTouch(key, nullptr); }

    // Removes the given key. Returns whether it existed. The mapped value is
    // filled in 'value', if non-null.
    bool Erase(const KeyType& key, ValueType* value = nullptr) {
        typename MapType::iterator map_it = map_.find(key);
        if (map_it == map_.end())
            return false;
        typename ListType::iterator list_it = map_it->second;
        CHECK(list_it->first == key);
        if (value != nullptr)
            *value = list_it->second;
        list_.erase(list_it);
        map_.erase(map_it);
        return true;
    }

    // Removes the oldest entry. Returns whether one existed. The mapped value
    // is filled in 'value', if non-null.
    bool EvictOldest(ValueType* value /*can be nullptr*/) {
        if (list_.empty())
            return false;
        KeyType key = list_.back().first;
        if (value != nullptr)
            *value = list_.back().second;
        list_.pop_back();
        int ret = map_.erase(key);
        CHECK_EQ(ret, 1);  // Must have existed
        return true;
    }

    // Returns the full contents of this map with the oldest item being at
    // index 0. This is an expensive call and should normally not be used
    // except for testing.
    std::vector<std::pair<KeyType, ValueType>> RawList() const {
        std::vector<std::pair<KeyType, ValueType>> res(map_.size());
        size_t i = map_.size();
        for (const auto& it : list_) {
            res[--i] = it;
        }
        return res;
    }

    // Since LruMap does not (yet) expose STL-like begin() and end(), it can't be used with
    // helper functions like STLDeleteValues(), so the following function is exposed directly.
    // Call only if the map's values are of 'raw pointer' type.
    void DeleteValues() {
        for (auto& p : list_)
            delete p.second;
    }

    // An intensive check for internal consistency. Do not call frequently.
    void CheckInternalCorrectness() const {
        if (list_.empty()) {
            CHECK(map_.empty());
            return;
        }
        for (const auto& p : list_) {
            typename MapType::const_iterator map_it = map_.find(p.first);
            CHECK(map_it != map_.end());
            // CHECK(p.first == list_it->first);
            // CHECK(p.second == list_it->second);
            CHECK_EQ(memcmp(&p, &(*map_it->second), sizeof(p)), 0);
        }
        CHECK_EQ(list_.size(), map_.size());  // This is an expensive call!
    }

#if 0
    void SaveToFile(FileOutputStream* out) {
        out->WriteGenericOrDie(this->unknown_key_);
        out->WriteGenericOrDie(this->unknown_value_);
        out->WriteGenericOrDie(static_cast<long>(map_.size()));
        for (ListIterType it = list_.begin(); it != list_.end(); ++it) {
            const std::pair<KeyType, ValueType>& p = *it;
            out->WriteGenericOrDie(p.first);
            out->WriteGenericOrDie(p.second);
        }
    }

    void LoadFromFile(FileInputStream* in) {
        CHECK(list_.empty() && map_.empty());
        KeyType k;
        in->ReadGenericOrDie(&k); CHECK(k == this->unknown_key_);
        ValueType v;
        in->ReadGenericOrDie(&v); CHECK(v == this->unknown_value_);
        long n;
        in->ReadGenericOrDie(&n);
        for (; n > 0; --n) {
            std::pair<KeyType, ValueType> p;
            in->ReadGenericOrDie(&p.first);
            in->ReadGenericOrDie(&p.second);
            list_.push_back(p);
            ListIterType list_it = list_.end();
            map_[p.first] = --list_it;
        }
        CHECK_EQ(map_.size(), list_.size());
    }
#endif

  private:
    DISALLOW_COPY_AND_ASSIGN(LruMap);
};

}  // namespace cpp_base

#endif  // CPP_BASE_DATA_STRUCT_LRU_MAP_H_
