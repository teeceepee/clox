#pragma once
#include "memory.h"

namespace collections {

template <typename K, typename V>
class Entry {
  K key;
  V value;
};

template <typename K, typename V>
class HashMap {
  typedef uint32_t (*HashFn)(K k);

public:
  HashMap(HashFn hashFn);
  ~
  HashMap();

  bool
  get(K key, /* out */ V* value);

  bool
  insert(K key, V value);

  bool
  remove(K key);

  int count;
  int capacity;
  Entry<K, V>* entries;

private:
  Entry<K, V>
  findEntry(K key);

  HashFn hashFn;
};

inline uint32_t
modulo(uint32_t index, int capacity) {
  return index & (capacity - 1);
}

// impl

template <typename K, typename V>
HashMap<K, V>::HashMap(const HashFn hashFn) : count{0}, capacity{0}, entries{nullptr}, hashFn{hashFn} {}

template <typename K, typename V>
HashMap<K, V>::~HashMap() {
#define COMMA ,
  FREE_ARRAY(Entry<K COMMA V>, this->entries, this->capacity);
#undef COMMA
}

template <typename K, typename V>
bool
HashMap<K, V>::get(K key, V* value) {
  if (this->count == 0) {
    return false;
  }

  Entry<K, V> entry = findEntry(key);
}

template <typename K, typename V>
bool
HashMap<K, V>::insert(K key, V value) {}
template <typename K, typename V>
bool
HashMap<K, V>::remove(K key) {}

template <typename K, typename V>
Entry<K, V>
HashMap<K, V>::findEntry(int capacity, K key) {
  uint32_t index = modulo(this->hashFn(key), capacity);
  Entry<K, V>* tombstone = nullptr;

  for (;;) {
    Entry<K, V>* entry = &(this->entries[index]);
    if (entry->key == nullptr) {
      // if () {
      //
      // }
    }
  }
}

}
