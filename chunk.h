#ifndef CLOX_CHUNK_H
#define CLOX_CHUNK_H

#include "common.h"
#include "value.h"

enum class OpCode : uint8_t {
  OP_CONSTANT,
  OP_NIL,
  OP_TRUE,
  OP_FALSE,
  OP_POP,
  OP_GET_LOCAL,
  OP_SET_LOCAL,
  OP_GET_GLOBAL,
  OP_DEFINE_GLOBAL,
  OP_SET_GLOBAL,
  OP_GET_UPVALUE,
  OP_SET_UPVALUE,
  OP_GET_PROPERTY,
  OP_SET_PROPERTY,
  OP_GET_SUPER,
  OP_EQUAL,
  OP_GREATER,
  OP_LESS,
  OP_ADD,
  OP_SUBTRACT,
  OP_MULTIPLY,
  OP_DIVIDE,
  OP_NOT,
  OP_NEGATE,
  OP_PRINT,
  OP_JUMP,
  OP_JUMP_IF_FALSE,
  OP_LOOP,
  OP_CALL,
  OP_INVOKE,
  OP_SUPER_INVOKE,
  OP_CLOSURE,
  OP_CLOSE_UPVALUE,
  OP_RETURN,
  OP_CLASS,
  OP_INHERIT,
  OP_METHOD,
};

inline OpCode
u8ToOpCode(uint8_t i) {
  return static_cast<OpCode>(i);
}

inline uint8_t
opCodeToU8(OpCode code) {
  return static_cast<uint8_t>(code);
}

struct Chunk {
  int count;
  int capacity;
  uint8_t* code;
  int* lines;
  ValueArray constants;
};

void
initChunk(Chunk* chunk);

void
freeChunk(Chunk* chunk);

void
writeChunk(Chunk* chunk, uint8_t byte, int line);

int
addConstant(Chunk* chunk, Value value);

#endif
