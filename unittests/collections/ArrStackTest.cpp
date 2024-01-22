#include "collections/ArrStack.h"

#include <gtest/gtest.h>

TEST(ArrStackTest, ctor) {
  ArrStack<uint64_t, 12> arrStack{};
  ASSERT_EQ(12, arrStack.capacity);

  ASSERT_TRUE(arrStack.items == arrStack.ending);
  ASSERT_TRUE(arrStack.bottom() == arrStack.top());
}

TEST(ArrStackTest, num) {
  ArrStack<uint64_t, 12> arrStack{};

  arrStack.push(100);
  arrStack.push(200);
  arrStack.push(300);

  ASSERT_EQ(300, arrStack.getByNum(1));
  ASSERT_EQ(200, arrStack.getByNum(2));
  ASSERT_EQ(100, arrStack.getByNum(3));
}
