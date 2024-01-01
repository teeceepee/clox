#include "value.h"

#include <gtest/gtest.h>

TEST(ValueTest, NilTC) {
  ASSERT_TRUE(IS_NIL(NIL_VAL));
  ASSERT_FALSE(IS_BOOL(NIL_VAL));
  ASSERT_FALSE(IS_NUMBER(NIL_VAL));
  ASSERT_FALSE(IS_OBJ(NIL_VAL));
}

TEST(ValueTest, EqualityTC) {
  ASSERT_TRUE(valuesEqual(NIL_VAL, NIL_VAL));
  ASSERT_TRUE(valuesEqual(FALSE_VAL, FALSE_VAL));
  ASSERT_TRUE(valuesEqual(TRUE_VAL, TRUE_VAL));
}

TEST(ValueArrayTest, CtorTC) {
  ValueArray values;
  ASSERT_EQ(0, values.values.count);
}

TEST(ValueArrayTest, WriteValueTC) {
  ValueArray values;
  ASSERT_EQ(0, values.values.count);

  values.writeValue(NIL_VAL);
  ASSERT_EQ(1, values.values.count);

  values.writeValue(NIL_VAL);
  ASSERT_EQ(2, values.values.count);
}
