#include "common.h"

#include <gtest/gtest.h>

TEST(CommonTest, Uint8CountTC) {
  ASSERT_EQ(256, UINT8_COUNT);
}
