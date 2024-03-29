#include "object.h"

#include "memory.h"
#include "table.h"
#include "value.h"
#include "vm.h"

#include <cstdio>
#include <cstring>

// clang-format off
#define ALLOCATE_OBJ(type, objectType) \
    (type*)allocateObject(sizeof(type), objectType)
// clang-format on

static Obj*
allocateObject(size_t size, ObjType type) {
  Obj* object = (Obj*)reallocate(NULL, 0, size);
  object->type = type;
  object->isMarked = false;
  object->next = vm.objects;
  vm.objects = object;

#ifdef DEBUG_LOG_GC
  printf("%p allocate %zu for %d\n", (void*)object, size, type);
#endif

  return object;
}

Obj::
Obj(const ObjType type)
    : type{type}, isMarked{false} {
  this->next = vm.objects;
  vm.objects = this;
}

ObjFunction::
ObjFunction()
    : Obj{ObjType::OBJ_FUNCTION}, arity{0}, upvalueCount{0}, name{nullptr} {}

void
ObjFunction::gcMark() {
  markObject((Obj*)this->name);
  this->chunk.constants.gcMark();
}

void*
ObjFunction::operator new(size_t size) {
  return reallocate(nullptr, 0, size);
}

void
ObjFunction::operator delete(void* ptr) {
  reallocate(ptr, sizeof(ObjFunction), 0);
}

ObjBoundMethod*
newBoundMethod(Value receiver, ObjClosure* method) {
  ObjBoundMethod* bound = ALLOCATE_OBJ(ObjBoundMethod, ObjType::OBJ_BOUND_METHOD);
  bound->receiver = receiver;
  bound->method = method;
  return bound;
}

ObjClass*
newClass(ObjString* name) {
  ObjClass* klass = ALLOCATE_OBJ(ObjClass, ObjType::OBJ_CLASS);
  klass->name = name;
  initTable(&(klass->methods));
  return klass;
}

ObjClosure*
newClosure(ObjFunction* function) {
  ObjUpvalue** upvalues = ALLOCATE(ObjUpvalue*, function->upvalueCount);
  for (int i = 0; i < function->upvalueCount; i++) {
    upvalues[i] = nullptr;
  }

  ObjClosure* closure = ALLOCATE_OBJ(ObjClosure, ObjType::OBJ_CLOSURE);
  closure->function = function;
  closure->upvalues = upvalues;
  closure->upvalueCount = function->upvalueCount;
  return closure;
}

ObjFunction*
newFunction() {
  return new ObjFunction{};
}

ObjInstance*
newInstance(ObjClass* klass) {
  ObjInstance* instance = ALLOCATE_OBJ(ObjInstance, ObjType::OBJ_INSTANCE);
  instance->klass = klass;
  initTable(&(instance->fields));
  return instance;
}

ObjNative*
newNative(NativeFn function) {
  ObjNative* native = ALLOCATE_OBJ(ObjNative, ObjType::OBJ_NATIVE);
  native->function = function;
  return native;
}

static ObjString*
allocateString(char* chars, int length, uint32_t hash) {
  ObjString* string = ALLOCATE_OBJ(ObjString, ObjType::OBJ_STRING);
  string->length = length;
  string->chars = chars;
  string->hash = hash;

  push(OBJ_VAL(string));
  tableSet(&(vm.strings), string, NIL_VAL);
  pop();

  return string;
}

static uint32_t
hashString(const char* key, int length) {
  uint32_t hash = 2166136261u;
  for (int i = 0; i < length; i++) {
    hash ^= (uint8_t)key[i];
    hash *= 16777619;
  }
  return hash;
}

ObjString*
takeString(char* chars, int length) {
  uint32_t hash = hashString(chars, length);
  ObjString* interned = tableFindString(&(vm.strings), chars, length, hash);
  if (interned != nullptr) {
    FREE_ARRAY(char, chars, length + 1);
    return interned;
  }
  return allocateString(chars, length, hash);
}

ObjString*
copyString(const char* chars, int length) {
  uint32_t hash = hashString(chars, length);
  ObjString* interned = tableFindString(&(vm.strings), chars, length, hash);
  if (interned != nullptr) {
    return interned;
  }

  char* heapChars = ALLOCATE(char, length + 1);
  memcpy(heapChars, chars, length);
  heapChars[length] = '\0';
  return allocateString(heapChars, length, hash);
}

static void
printFunction(ObjFunction* function) {
  if (function->name == nullptr) {
    printf("<script>");
    return;
  }
  printf("<fn %s>", function->name->chars);
}

ObjUpvalue*
newUpvalue(Value* slot) {
  ObjUpvalue* upvalue = ALLOCATE_OBJ(ObjUpvalue, ObjType::OBJ_UPVALUE);
  upvalue->closed = NIL_VAL;
  upvalue->location = slot;
  upvalue->next = nullptr;
  return upvalue;
}

void
printObject(Value value) {
  switch (OBJ_TYPE(value)) {
  case ObjType::OBJ_BOUND_METHOD:
    printFunction(AS_BOUND_METHOD(value)->method->function);
    break;
  case ObjType::OBJ_CLASS:
    printf("%s", AS_CLASS(value)->name->chars);
    break;
  case ObjType::OBJ_CLOSURE:
    printFunction(AS_CLOSURE(value)->function);
    break;
  case ObjType::OBJ_FUNCTION:
    printFunction(AS_FUNCTION(value));
    break;
  case ObjType::OBJ_INSTANCE:
    printf("%s instance", AS_INSTANCE(value)->klass->name->chars);
    break;
  case ObjType::OBJ_NATIVE:
    printf("<native fn>");
    break;
  case ObjType::OBJ_STRING:
    printf("%s", AS_CSTRING(value));
    break;
  case ObjType::OBJ_UPVALUE:
    printf("upvalue");
    break;
  }
}
