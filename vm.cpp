#include "vm.h"

#include "compiler.h"
#include "debug.h"

#include <cstdarg>
#include <cstdio>

VM vm;

static void
resetStack() {
    vm.stackTop = vm.stack;
}

static void
runtimeError(const char* format, ...) {
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    fputs("\n", stderr);

    size_t instruction = vm.ip - vm.chunk->code - 1;
    int line = vm.chunk->lines[instruction];
    fprintf(stderr, "[line %d] in script\n", line);
    resetStack();
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

static Value
peek(int distance) {
    return vm.stackTop[-1 - distance];
}

static bool
isFalsey(Value value) {
    return IS_NIL(value) || (IS_BOOL(value) && !AS_BOOL(value));
}

static InterpretResult
run() {
#define READ_BYTE() (*(vm.ip++))
#define READ_CONSTANT() (vm.chunk->constants.values[READ_BYTE()])
    // clang-format off
#define BINARY_OP(valueType, op) \
    do { \
        if (!IS_NUMBER(peek(0)) || !IS_NUMBER(peek(1))) { \
        } \
        double b = AS_NUMBER(pop()); \
        double a = AS_NUMBER(pop()); \
        push(valueType(a op b)); \
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
        case OpCode::OP_NIL: {
            push(NIL_VAL);
            break;
        }
        case OpCode::OP_TRUE: {
            push(BOOL_VAL(true));
            break;
        }
        case OpCode::OP_FALSE: {
            push(BOOL_VAL(false));
            break;
        }
        case OpCode::OP_EQUAL: {
            Value b = pop();
            Value a = pop();
            push(BOOL_VAL(valuesEqual(a, b)));
            break;
        }
        case OpCode::OP_GREATER: {
            BINARY_OP(BOOL_VAL, >);
            break;
        }
        case OpCode::OP_LESS: {
            BINARY_OP(BOOL_VAL, <);
            break;
        }
        case OpCode::OP_ADD: {
            BINARY_OP(NUMBER_VAL, +);
            break;
        }
        case OpCode::OP_SUBTRACT: {
            BINARY_OP(NUMBER_VAL, -);
            break;
        }
        case OpCode::OP_MULTIPLY: {
            BINARY_OP(NUMBER_VAL, *);
            break;
        }
        case OpCode::OP_DIVIDE: {
            BINARY_OP(NUMBER_VAL, /);
            break;
        }
        case OpCode::OP_NOT: {
            push(BOOL_VAL(isFalsey(pop())));
            break;
        }
        case OpCode::OP_NEGATE: {
            if (!IS_NUMBER(peek(0))) {
                runtimeError("Operand must be a number.");
                return InterpretResult::INTERPRET_RUNTIME_ERROR;
            }
            push(NUMBER_VAL(-AS_NUMBER(pop())));
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
