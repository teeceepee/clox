#ifndef CLOX_VALUE_H
#define CLOX_VALUE_H

#include "collection/Vec.h"
#include "common.h"

struct Obj;
struct ObjString;

#ifdef NAN_BOXING

#include <cstring>

// clang-format off
#define SIGN_BIT ((uint64_t)0x8000'0000'0000'0000) // NOTE: 1 << 63
#define QNAN     ((uint64_t)0x7ffc'0000'0000'0000) // NOTE: 0b0'11111111111'1100 << 48

#define TAG_NIL   1 // 01.
#define TAG_FALSE 2 // 10.
#define TAG_TRUE  3 // 11.

typedef uint64_t Value;

#define IS_BOOL(value)    (((value) | 1) == TRUE_VAL)
#define IS_NIL(value)     ((value) == NIL_VAL)
#define IS_NUMBER(value)  (((value) & QNAN) != QNAN)
#define IS_OBJ(value)     (((value) & (QNAN | SIGN_BIT)) == (QNAN | SIGN_BIT))

#define AS_BOOL(value)   ((value) == TRUE_VAL)
#define AS_NUMBER(value) valueToNum(value)
#define AS_OBJ(value)    ((Obj*)(uintptr_t)((value) & ~(SIGN_BIT | QNAN)))

#define BOOL_VAL(b)     ((b) ? TRUE_VAL : FALSE_VAL)
#define FALSE_VAL       ((Value)(uint64_t)(QNAN | TAG_FALSE))
#define TRUE_VAL        ((Value)(uint64_t)(QNAN | TAG_TRUE))
#define NIL_VAL         ((Value)(uint64_t)(QNAN | TAG_NIL))
#define NUMBER_VAL(num) numToValue(num)
#define OBJ_VAL(obj)    (Value)(SIGN_BIT | QNAN | (uint64_t)(uintptr_t)(obj))
// clang-format on

static inline double
valueToNum(Value value) {
  double num;
  memcpy(&num, &value, sizeof(Value));
  return num;
}

static inline Value
numToValue(double num) {
  Value value;
  memcpy(&value, &num, sizeof(double));
  return value;
}

#else

enum class ValueType {
  VAL_BOOL,
  VAL_NIL,
  VAL_NUMBER,
  VAL_OBJ,
};

struct Value {
  ValueType type;
  union {
    bool boolean;
    double number;
    Obj* obj;
  } as;
};

// clang-format off
#define IS_BOOL(value)    ((value).type == ValueType::VAL_BOOL)
#define IS_NIL(value)     ((value).type == ValueType::VAL_NIL)
#define IS_NUMBER(value)  ((value).type == ValueType::VAL_NUMBER)
#define IS_OBJ(value)     ((value).type == ValueType::VAL_OBJ)

#define AS_BOOL(value)    ((value).as.boolean)
#define AS_NUMBER(value)  ((value).as.number)
#define AS_OBJ(value)     ((value).as.obj)

#define BOOL_VAL(value)   ((Value){ValueType::VAL_BOOL, {.boolean = value}})
#define NIL_VAL           ((Value){ValueType::VAL_NIL, {.number = 0}})
#define NUMBER_VAL(value) ((Value){ValueType::VAL_NUMBER, {.number = value}})
#define OBJ_VAL(object)   ((Value){ValueType::VAL_OBJ, {.obj = (Obj*)object}})
// clang-format on

#endif

bool
valuesEqual(Value a, Value b);

void
printValue(Value value);

class ValueArray {
public:
  int
  writeValue(Value value);

  void
  gcMark();

  Vec<Value> values;
};

#endif
