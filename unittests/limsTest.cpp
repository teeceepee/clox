#include "lims.h"

#include <gtest/gtest.h>

TEST(LimsTest, Uint8CountTC) { ASSERT_EQ(16384, lims::STACK_MAX); }
