#ifndef CLOX_VALUE_H
#define CLOX_VALUE_H

#include "common.h"

struct Obj;
struct ObjString;

enum ValueType {
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
