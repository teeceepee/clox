#pragma once
#include "memory.h"

namespace collections {


inline uint32_t
modulo(uint32_t index, int capacity) {
  return index & (capacity - 1);
}


template <typename K, typename V>
class Entry {
  K key;
  V value;
};

template <typename K, typename V, typename KeyHashFn>
class HashMap {

public:
  HashMap() : count{0}, capacity{0}, entries{nullptr} {}
  ~
  HashMap() {
#define COMMA ,
    FREE_ARRAY(Entry<K COMMA V>, this->entries, this->capacity);
#undef COMMA
  }

  void foo() {
    KeyHashFn(1);
  }

  bool
  get(K key, /* out */ V* value) {
    if (this->count == 0) {
      return false;
    }

    Entry<K, V> entry = findEntry(key);
    // todo
  }

  bool
  insert(K key, V value) {
    return false; // todo
  }

  bool
  remove(K key) {
    return false;
  }

  int count;
  int capacity;
  Entry<K, V>* entries;

private:
  Entry<K, V>
  findEntry(K key) {
    uint32_t hashVal = KeyHashFn(key);
    uint32_t index = modulo(hashVal, capacity);
    Entry<K, V>* tombstone = nullptr;

    for (;;) {
      Entry<K, V>* entry = &(this->entries[index]);
      if (entry->key == nullptr) {
        // if () {
        //
        // }
      }

      return *entry;
    }
  }
};

} // namespace collections
