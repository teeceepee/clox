#include "collections/HashMap.h"

#include <gtest/gtest.h>
#include <unordered_map>

struct Point {
  int x;
  int y;
};

typedef uint32_t (*HashOp)(int);

template <typename K, typename Op>
struct Foo {

  void
  bar(K key) {
    auto op = Op{};
    uint32_t ret = op(key);
    printf("%d\n", ret);
  }
};

uint32_t
incr(Point n) {
  return 1;
}

struct HashFunctor {
  auto
  operator()(Point& p) const -> size_t {
    return 1234;
  }
};

struct IntHashFunctor {
  auto
  operator()(int p) const -> size_t {
    return 1234;
  }
};

TEST(HashMapTest, ctor) {
  // Point p{};
  //
  // Foo<Point, HashFunctor> f;
  // f.bar(p);
  //
  //
  // collections::HashMap<Point, int, incr> m{};
  // m.foo();

  std::unordered_map<int, int, IntHashFunctor> um{};
  um[1] = 2;
  // std::hash<int> h;
  // std::equal_to<int> et;
  // collections::HashMap<int, bool> map{};
  // ASSERT_EQ(12, arr.capacity);
  // ASSERT_EQ(0, arr.count);
}

// template <typename Key>
// struct KeyHasher
// {
//   std::size_t operator()(const Key& k) const
//   {
//     return 1;
//   }
// };
//
//
// typedef int (*FooFn)(int a);
//
// template <typename K, typename V, typename Hasher>
// class MyMap {
// public:
//   void foo() {
//     auto a = Hasher("asdf");
//
//     printf("a: %d\n", a);
//   }
// };
