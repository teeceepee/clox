#include "collections/Arr.h"

#include <gtest/gtest.h>

TEST(ArrTest, ctor) {
  Arr<uint64_t, 12> arr{};
  ASSERT_EQ(12, arr.capacity);
  ASSERT_EQ(0, arr.count);
}
