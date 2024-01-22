#pragma once

template <typename T, int Size>
class ArrStack {
public:
  ArrStack();

  void
  push(T val);
  T&
  pop();
  void
  clear();

  /**
   * The num argument is how far down from the top of the stack to look:
   * 1 is the top, 2 is one slot down, etc.
   */
  T*
  getAddressByNum(int num);
  T&
  getByNum(int num);
  void
  setByNum(int num, T val);

  void
  shrinkBySize(int size);

  void
  setTop(T* newTop);

  T*
  bottom();
  T*
  top();

  T&
  first();
  T&
  second();

  int capacity;
  T* ending;
  T items[Size];
};

// impl

template <typename T, int Size>
ArrStack<T, Size>::ArrStack() : capacity{Size}, ending{items} {}

template <typename T, int Size>
void
ArrStack<T, Size>::push(T val) {
  *(this->ending) = val;
  ++(this->ending);
}

template <typename T, int Size>
T&
ArrStack<T, Size>::pop() {
  --(this->ending);
  return *(this->ending);
}

template <typename T, int Size>
T*
ArrStack<T, Size>::getAddressByNum(int num) {
  return this->ending - num;
}

template <typename T, int Size>
T&
ArrStack<T, Size>::getByNum(int num) {
  return *this->getAddressByNum(num);
}

template <typename T, int Size>
void
ArrStack<T, Size>::setByNum(int num, T val) {
  *this->getAddressByNum(num) = val;
}

template <typename T, int Size>
void
ArrStack<T, Size>::shrinkBySize(int size) {
  this->ending -= size;
}

template <typename T, int Size>
void
ArrStack<T, Size>::setTop(T* newTop) {
  this->ending = newTop;
}

template <typename T, int Size>
void
ArrStack<T, Size>::clear() {
  this->ending = this->items;
}

template <typename T, int Size>
T*
ArrStack<T, Size>::bottom() {
  return this->items;
}

template <typename T, int Size>
T*
ArrStack<T, Size>::top() {
  return this->ending;
}

template <typename T, int Size>
T&
ArrStack<T, Size>::first() {
  return this->items[0];
}

template <typename T, int Size>
T&
ArrStack<T, Size>::second() {
  return this->items[1];
}
