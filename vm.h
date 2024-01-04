#ifndef CLOX_VM_H
#define CLOX_VM_H

#include "chunk.h"
#include "lims.h"
#include "object.h"
#include "table.h"
#include "value.h"

struct CallFrame {
  ObjClosure* closure;
  uint8_t* ip;
  Value* slots;
};

struct VM {
  CallFrame frames[lims::FRAMES_MAX];
  int frameCount;

  Value stack[lims::STACK_MAX];
  Value* stackTop;
  Table globals;
  Table strings;
  ObjString* initString;
  ObjUpvalue* openUpvalues;

  size_t bytesAllocated;
  size_t nextGC;

  Obj* objects;
  int grayCount;
  int grayCapacity;
  Obj** grayStack;
};

enum class InterpretResult {
  INTERPRET_OK,
  INTERPRET_COMPILE_ERROR,
  INTERPRET_RUNTIME_ERROR,
};

extern VM vm;

void
initVM();

void
freeVM();

InterpretResult
interpret(const char* source);

void
push(Value value);

Value
pop();

#endif
