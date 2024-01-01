#include "value.h"

#include <gtest/gtest.h>

TEST(runner, foo) {
  ASSERT_TRUE(true);
  ASSERT_FALSE(false);
}

int
main(int argc, char** argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
