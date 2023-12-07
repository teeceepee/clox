#ifndef CLOX_CHUNK_H
#define CLOX_CHUNK_H

#include "common.h"

enum OpCode {
    OP_RETURN,
};

struct Chunk {
    int count;
    int capacity;
    uint8_t* code;
};

void
initChunk(Chunk* chunk);
void
freeChunk(Chunk* chunk);
void
writeChunk(Chunk* chunk, uint8_t byte);

#endif