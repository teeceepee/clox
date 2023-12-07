#ifndef CLOX_CHUNK_H
#define CLOX_CHUNK_H

#include "common.h"
#include "value.h"

enum OpCode {
    OP_RETURN,
};

struct Chunk {
    int count;
    int capacity;
    uint8_t* code;
    ValueArray constants;
};

void
initChunk(Chunk* chunk);

void
freeChunk(Chunk* chunk);

void
writeChunk(Chunk* chunk, uint8_t byte);

int
addConstant(Chunk* chunk, Value value);

#endif
