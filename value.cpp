#include "value.h"

#include "memory.h"
#include "object.h"

#include <cstdio>
#include <cstring>

void
printValue(Value value) {
#ifdef NAN_BOXING
  if (IS_BOOL(value)) {
    printf(AS_BOOL(value) ? "true" : "false");
  } else if (IS_NIL(value)) {
    printf("nil");
  } else if (IS_NUMBER(value)) {
    printf("%g", AS_NUMBER(value));
  } else if (IS_OBJ(value)) {
    printObject(value);
  }
#else
  switch (value.type) {
  case ValueType::VAL_BOOL: {
    printf(AS_BOOL(value) ? "true" : "false");
    break;
  }
  case ValueType::VAL_NIL: {
    printf("nil");
    break;
  }
  case ValueType::VAL_NUMBER: {
    printf("%g", AS_NUMBER(value));
    break;
  }
  case ValueType::VAL_OBJ: {
    printObject(value);
    break;
  }
  }
#endif
}

bool
valuesEqual(Value a, Value b) {
#ifdef NAN_BOXING
  if (IS_NUMBER(a) && IS_NUMBER(b)) {
    return AS_NUMBER(a) == AS_NUMBER(b);
  }
  return a == b;
#else
  if (a.type != b.type) {
    return false;
  }
  switch (a.type) {
  case ValueType::VAL_BOOL:
    return AS_BOOL(a) == AS_BOOL(b);
  case ValueType::VAL_NIL:
    return true;
  case ValueType::VAL_NUMBER:
    return AS_NUMBER(a) == AS_NUMBER(b);
  case ValueType::VAL_OBJ:
    return AS_OBJ(a) == AS_OBJ(b);
  default:
    return false; // Unreachable.
  }
#endif
}

int
ValueArray::writeValue(Value value) {
  const int idx = this->values.count;
  this->values.push(value);
  return idx;
}

void
ValueArray::gcMark() {
  for (int i = 0; i < this->values.count; i++) {
    markValue(this->values[i]);
  }
}
