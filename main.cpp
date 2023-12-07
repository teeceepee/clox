#include "common.h"
#include "chunk.h"
#include "debug.h"

int
main(int argc, const char* argv[]) {
    Chunk chunk{};
    initChunk(&chunk);

    int constantIdx = addConstant(&chunk, 1.2);
    writeChunk(&chunk, OpCode::OP_CONSTANT, 123);
    writeChunk(&chunk, constantIdx, 123);

    writeChunk(&chunk, OpCode::OP_RETURN, 123);

    disassembleChunk(&chunk, "test chunk");
    freeChunk(&chunk);

    return 0;
}
