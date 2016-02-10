#ifndef CPP_BASE_DATA_STRUCT_LRU_SET_H_
#define CPP_BASE_DATA_STRUCT_LRU_SET_H_

#include "cpp-base/data-struct/lru_map.h"
#include "cpp-base/hash/hash.h"
#include "cpp-base/macros.h"
#include "cpp-base/util/map_util.h"

namespace cpp_base {

// This class provides a wrapper around LruMap to simplify its interface when
// only 'set' (not 'map') functionality is needed. It is *not* thread safe.
template <typename T>
class LruSet {
  private:
    // The mapped value (the trivial boolean) wastes some memory so consider
    // rewriting this class with custom internal data structs (like LruMap)
    // if this waste is considerable in your use case.
    LruMap<T, bool /*trivial*/> map_;
  public:
    LruSet() : map_() {}
    explicit LruSet(int capacity) : map_(capacity) {}
    ~LruSet() {}

    // Use this setter only at initialization time. Do not use when the set is full.
    inline void SetCapacity(int capacity) { map_.SetCapacity(capacity); }

    inline bool Empty() const { return map_.Empty(); }
    inline int Size() const { return map_.Size(); }

    // Insert the given key. If it existed, it is reinserted at list head.
    // Returns whether the key already existed.
    inline bool Insert(const T& key) { return map_.Put(key, true); }

    // Returns whether the key exists. Does not re-position the key in the set.
    inline bool Contains(const T& key) const { return map_.Contains(key); }

    // Just like Contains() except that it re-positions the key in the set.
    inline bool Touch(const T& key) { return map_.Touch(key); }

    // Removes the given key. Returns whether it existed.
    inline bool Erase(const T& key) { return map_.Erase(key, nullptr); }

    // An intensive check for internal consistency. Do not call frequently.
    void CheckInternalCorrectness() const { map_.CheckInternalCorrectness(); }

  private:
    DISALLOW_COPY_AND_ASSIGN(LruSet);
};

}  // namespace cpp_base

#endif  // CPP_BASE_DATA_STRUCT_LRU_SET_H_
