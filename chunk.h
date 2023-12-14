#ifndef CLOX_CHUNK_H
#define CLOX_CHUNK_H

#include "common.h"
#include "value.h"

enum OpCode {
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
    OP_RETURN,
};

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
