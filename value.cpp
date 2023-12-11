#include <cstdio>

#include "memory.h"
#include "value.h"

void
initValueArray(ValueArray* array) {
    array->count = 0;
    array->capacity = 0;
    array->values = nullptr;
}

void
writeValueArray(ValueArray* array, Value value) {
    if (array->capacity < array->count + 1) {
        int oldCapacity = array->capacity;
        array->capacity = GROW_CAPACITY(oldCapacity);
        array->values = GROW_ARRAY(Value, array->values, oldCapacity, array->capacity);
    }

    array->values[array->count] = value;
    array->count += 1;
}

void
freeValueArray(ValueArray* array) {
    FREE_ARRAY(Value, array->values, array->capacity);
}

void
printValue(Value value) {
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
    }
}

bool
valuesEqual(Value a, Value b) {
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
    default:
        return false; // Unreachable.
    }
}
