#pragma once

template <typename T>
class Vec {
public:
  Vec();
  ~
  Vec();

  void
  push(T item);

  T*
  beginning();

  T&
  operator[](int index);

  int capacity;
  int count;
  T* items;
};

// impl

#include "memory.h"

template <typename T>
Vec<T>::Vec() : capacity{0}, count{0}, items{nullptr} {}

template <typename T>
Vec<T>::~Vec() {
  FREE_ARRAY(T, this->items, this->capacity);
}

template <typename T>
void
Vec<T>::push(T item) {
  if (this->capacity < this->count + 1) {
    const int oldCapacity = this->capacity;
    this->capacity = GROW_CAPACITY(oldCapacity);
    this->items = GROW_ARRAY(T, this->items, oldCapacity, this->capacity);
  }

  this->items[this->count] = item;
  this->count += 1;
}

template <typename T>
T*
Vec<T>::beginning() {
  return this->items;
}

template <typename T>
T&
Vec<T>::operator[](int index) {
  return this->items[index];
}
