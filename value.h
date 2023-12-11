#ifndef CLOX_VALUE_H
#define CLOX_VALUE_H

#include "common.h"

enum ValueType {
    VAL_BOOL,
    VAL_NIL,
    VAL_NUMBER,
};

struct Value {
    ValueType type;
    union {
        bool boolean;
        double number;
    } as;
};

// clang-format off
#define IS_BOOL(value)    ((value).type == ValueType::VAL_BOOL)
#define IS_NIL(value)     ((value).type == ValueType::VAL_NIL)
#define IS_NUMBER(value)  ((value).type == ValueType::VAL_NUMBER)

#define AS_BOOL(value)    ((value).as.boolean)
#define AS_NUMBER(value)  ((value).as.number)

#define BOOL_VAL(value)   ((Value){ValueType::VAL_BOOL, {.boolean = value}})
#define NIL_VAL           ((Value){ValueType::VAL_NIL, {.number = 0}})
#define NUMBER_VAL(value) ((Value){ValueType::VAL_NUMBER, {.number = value}})
// clang-format on

struct ValueArray {
    int count;
    int capacity;
    Value* values;
};

bool
valuesEqual(Value a, Value b);

void
initValueArray(ValueArray* array);

void
writeValueArray(ValueArray* array, Value value);

void
freeValueArray(ValueArray* array);

void
printValue(Value value);

#endif
