// #include "common.h"
// #include "chunk.h"
// #include "debug.h"
#include "vm.h"

#include <cstdio>
#include <cstdlib>

static void
repl() {
    char line[1024];
    for (;;) {
        printf("> ");

        if (!fgets(line, sizeof(line), stdin)) {
            printf("\n");
            break;
        }

        interpret(line);
    }
}

static char*
readFile(const char* path) {
    FILE* file = fopen(path, "rb");
    if (file == nullptr) {
        fprintf(stderr, "Could not open file \"%s\".\n", path);
        exit(74);
    }

    fseek(file, 0L, SEEK_END);
    size_t fileSize = ftell(file);
    rewind(file);

    char* buffer = (char*)malloc(fileSize + 1);
    if (buffer == nullptr) {
        fprintf(stderr, "Not enough memory to read \"%s\".\n", path);
        exit(74);
    }

    size_t bytesRead = fread(buffer, sizeof(char), fileSize, file);
    if (bytesRead < fileSize) {
        fprintf(stderr, "Could not read file \"%s\".\n", path);
        exit(74);
    }

    buffer[bytesRead] = '\0';

    fclose(file);
    return buffer;
}

static void
runFile(const char* path) {
    char* source = readFile(path);
    InterpretResult result = interpret(source);
    free(source);

    if (result == InterpretResult::INTERPRET_COMPILE_ERROR) {
        exit(65);
    }
    if (result == InterpretResult::INTERPRET_RUNTIME_ERROR) {
        exit(70);
    }
}

int
main(int argc, const char* argv[]) {
    initVM();

    if (argc == 1) {
        repl();
    } else if (argc == 2) {
        runFile(argv[1]);
    } else {
        fprintf(stderr, "Usage: clox [path]\n");
        exit(64);
    }

    freeVM();

    // Chunk chunk{};
    // initChunk(&chunk);
    //
    // int constantIdx = addConstant(&chunk, 1.2);
    // writeChunk(&chunk, OpCode::OP_CONSTANT, 123);
    // writeChunk(&chunk, constantIdx, 123);
    //
    // constantIdx = addConstant(&chunk, 3.4);
    // writeChunk(&chunk, OpCode::OP_CONSTANT, 123);
    // writeChunk(&chunk, constantIdx, 123);
    //
    // writeChunk(&chunk, OpCode::OP_ADD, 123);
    //
    // constantIdx = addConstant(&chunk, 5.6);
    // writeChunk(&chunk, OpCode::OP_CONSTANT, 123);
    // writeChunk(&chunk, constantIdx, 123);
    //
    // writeChunk(&chunk, OpCode::OP_DIVIDE, 123);
    // writeChunk(&chunk, OpCode::OP_NEGATE, 123);
    //
    // writeChunk(&chunk, OpCode::OP_RETURN, 123);
    //
    // disassembleChunk(&chunk, "test chunk");
    // interpret(&chunk);
    // freeVM();
    // freeChunk(&chunk);

    return 0;
}
