#pragma once

template <typename T, int Size>
class Arr {
public:
  Arr();

  void clear();

  T&
  increaseCount();

  void
  decreaseCount();

  T&
  last();

  T&
  operator[](int index);

  bool
  isEmpty() const;

  bool
  reachedMax() const;

  int capacity;
  int count;
  T items[Size];
};

// impl

template <typename T, int Size>
Arr<T, Size>::Arr() : capacity{Size}, count{0} {}

template <typename T, int Size>
void
Arr<T, Size>::clear() {
  this->count = 0;
}

template <typename T, int Size>
T&
Arr<T, Size>::increaseCount() {
  this->count += 1;
  return this->last();
}

template <typename T, int Size>
void
Arr<T, Size>::decreaseCount() {
  this->count -= 1;
}

template <typename T, int Size>
T&
Arr<T, Size>::last() {
  return this->items[this->count - 1];
}

template <typename T, int Size>
T&
Arr<T, Size>::operator[](int index) {
  return this->items[index];
}

template <typename T, int Size>
bool
Arr<T, Size>::isEmpty() const {
  return this->count == 0;
}

template <typename T, int Size>
bool
Arr<T, Size>::reachedMax() const {
  return this->count == Size;
}
