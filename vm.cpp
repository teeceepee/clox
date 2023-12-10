#include "vm.h"

#include "compiler.h"
#include "debug.h"

#include <cstdio>

VM vm;

static void
resetStack() {
    vm.stackTop = vm.stack;
}

void
initVM() {
    resetStack();
}

void
freeVM() {

}

void
push(Value value) {
    *(vm.stackTop) = value;
    (vm.stackTop)++;
}

Value
pop() {
    (vm.stackTop)--;
    return *(vm.stackTop);
}

static InterpretResult
run() {
#define READ_BYTE() (*(vm.ip++))
#define READ_CONSTANT() (vm.chunk->constants.values[READ_BYTE()])
    // clang-format off
#define BINARY_OP(op) \
    do { \
        double b = pop(); \
        double a = pop(); \
        push(a op b); \
    } while (false)
    // clang-format on

    for (;;) {
#ifdef DEBUG_TRACE_EXECUTION
        printf("          ");
        for (Value* slot = vm.stack; slot < vm.stackTop; slot++) {
            printf("[ ");
            printValue(*slot);
            printf(" ]");
        }
        printf("\n");
        disassembleInstruction(vm.chunk, (int)(vm.ip - vm.chunk->code));
#endif

        uint8_t instruction = READ_BYTE();
        switch (instruction) {
        case OpCode::OP_CONSTANT: {
            Value constant = READ_CONSTANT();
            push(constant);
            break;
        }
        case OpCode::OP_ADD: {
            BINARY_OP(+);
            break;
        }
        case OpCode::OP_SUBTRACT: {
            BINARY_OP(-);
            break;
        }
        case OpCode::OP_MULTIPLY: {
            BINARY_OP(*);
            break;
        }
        case OpCode::OP_DIVIDE: {
            BINARY_OP(/);
            break;
        }
        case OpCode::OP_NEGATE: {
            push(-pop());
            break;
        }
        case OpCode::OP_RETURN:
            printValue(pop());
            printf("\n");
            return InterpretResult::INTERPRET_OK;
        }
    }

#undef READ_BYTE
#undef READ_CONSTANT
#undef BINARY_OP
}

InterpretResult
interpret(const char* source) {
    Chunk chunk;
    initChunk(&chunk);

    if (!compile(source, &chunk)) {
        freeChunk(&chunk);
        return InterpretResult::INTERPRET_COMPILE_ERROR;
    }

    vm.chunk = &chunk;
    vm.ip = vm.chunk->code;

    InterpretResult result = run();

    freeChunk(&chunk);
    return result;
}
