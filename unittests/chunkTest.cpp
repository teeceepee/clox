#include "chunk.h"

#include <gtest/gtest.h>

TEST(ChunkTest, CtoTC) {
  Chunk chunk;
  ASSERT_EQ(0, chunk.code.count);
  ASSERT_EQ(0, chunk.lines.count);
}
