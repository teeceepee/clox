#include "memory.h"

#include "object.h"
#include "vm.h"

#include <cstdlib>

void*
reallocate(void* pointer, size_t oldSize, size_t newSize) {
    if (newSize == 0) {
        free(pointer);
        return nullptr;
    }

    void* result = realloc(pointer, newSize);
    if (result == nullptr) {
        exit(1);
    }
    return result;
}

static void
freeObject(Obj* object) {
    switch (object->type) {
    case ObjType::OBJ_FUNCTION: {
        ObjFunction* function = (ObjFunction*)object;
        freeChunk(&(function->chunk));
        FREE(ObjFunction, object);
        break;
    }
    case ObjType::OBJ_NATIVE: {
        FREE(ObjNative, object);
        break;
    }
    case ObjType::OBJ_STRING: {
        ObjString* string = (ObjString*)object;
        FREE_ARRAY(char, string->chars, string->length + 1);
        FREE(ObjString, object);
        break;
    }
    }
}

void
freeObjects() {}
