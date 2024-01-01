#include "collection/Vec.h"

#include <gtest/gtest.h>

TEST(VecTest, ctor) {
  Vec<uint64_t> vec{};
  ASSERT_EQ(0, vec.count);

  vec.push(123);
  ASSERT_EQ(1, vec.count);
}
