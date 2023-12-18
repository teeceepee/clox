#ifndef CLOX_OBJECT_H
#define CLOX_OBJECT_H

#include "chunk.h"
#include "common.h"
#include "value.h"

#define OBJ_TYPE(value)        (AS_OBJ(value)->type)

#define IS_CLOSURE(value)      isObjType(value, ObjType::ObjClosure)
#define IS_FUNCTION(value)     isObjType(value, ObjType::ObjFunction)
#define IS_NATIVE(value)       isObjType(value, ObjType::OBJ_NATIVE)
#define IS_STRING(value)       isObjType(value, ObjType::OBJ_STRING)

#define AS_CLOSURE(value)      ((ObjClosure*)AS_OBJ(value))
#define AS_FUNCTION(value)     ((ObjFunction*)AS_OBJ(value))
#define AS_NATIVE(value)       (((ObjNative*)AS_OBJ(value))->function)
#define AS_STRING(value)       ((ObjString*)AS_OBJ(value))
#define AS_CSTRING(value)      (((ObjString*)AS_OBJ(value))->chars)

enum ObjType {
    OBJ_CLOSURE,
    OBJ_FUNCTION,
    OBJ_NATIVE,
    OBJ_STRING,
    OBJ_UPVALUE,
};

struct Obj {
    ObjType type;
    Obj* next;
};

struct ObjFunction {
    Obj obj;
    int arity;
    int upvalueCount;
    Chunk chunk;
    ObjString* name;
};

typedef Value (*NativeFn)(int argCount, Value* args);

struct ObjNative {
    Obj obj;
    NativeFn function;
};

struct ObjString {
    Obj obj;
    int length;
    char* chars;
    uint32_t hash;
};

struct ObjUpvalue {
    Obj obj;
    Value* location;
    Value closed;
    ObjUpvalue* next;
};

struct ObjClosure {
    Obj obj;
    ObjFunction* function;
    ObjUpvalue** upvalues;
    int upvalueCount;
};

ObjClosure*
newClosure(ObjFunction* function);

ObjFunction*
newFunction();

ObjNative*
newNative(NativeFn function);

ObjString*
takeString(char* chars, int length);

ObjString*
copyString(const char* chars, int length);

ObjUpvalue*
newUpvalue(Value* slot);

void
printObject(Value value);

static inline bool
isObjType(Value value, ObjType type) {
    return IS_OBJ(value) && AS_OBJ(value)->type == type;
}

#endif
