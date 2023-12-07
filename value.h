#ifndef CLOX_VALUE_H
#define CLOX_VALUE_H

#include "common.h"

typedef double Value;

struct ValueArray {
    int count;
    int capacity;
    Value* values;
};

void
initValueArray(ValueArray* array);

void
writeValueArray(ValueArray* array, Value value);

void
freeValueArray(ValueArray* array);

void
printValue(Value value);

#endif
