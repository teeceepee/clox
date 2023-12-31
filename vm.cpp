#include "vm.h"

#include "compiler.h"
#include "debug.h"
#include "memory.h"
#include "object.h"

#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <ctime>

VM vm;

static Value
clockNative(int argCount, Value* args) {
    return NUMBER_VAL((double)clock() / CLOCKS_PER_SEC);
}

static void
resetStack() {
    vm.stackTop = vm.stack;
    vm.frameCount = 0;
    vm.openUpvalues = nullptr;
}

static void
runtimeError(const char* format, ...) {
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    fputs("\n", stderr);

    CallFrame* frame = &(vm.frames[vm.frameCount - 1]);
    ObjFunction* function = frame->closure->function;
    size_t instruction = frame->ip - function->chunk.code - 1;
    int line = function->chunk.lines[instruction];
    fprintf(stderr, "[line %d] in script\n", line);
    resetStack();

    for (int i = vm.frameCount - 1; i >= 0; i--) {
        CallFrame* frame = &(vm.frames[i]);
        size_t instruction = frame->ip - function->chunk.code - 1;
        fprintf(stderr, "[line %d] in ", function->chunk.lines[instruction]);
        if (function->name == nullptr) {
            fprintf(stderr, "script\n");
        } else {
            fprintf(stderr, "%s()\n", function->name->chars);
        }
    }
}

static void
defineNative(const char* name, NativeFn function) {
    push(OBJ_VAL(copyString(name, (int)strlen(name))));
    push(OBJ_VAL(newNative(function)));
    tableSet(&(vm.globals), AS_STRING(vm.stack[0]), vm.stack[1]);
    pop();
    pop();
}

void
initVM() {
    resetStack();
    vm.objects = nullptr;
    vm.bytesAllocated = 0;
    vm.nextGC = 1024 * 1024;

    vm.grayCount = 0;
    vm.grayCapacity = 0;
    vm.grayStack = nullptr;

    initTable(&(vm.globals));
    initTable(&(vm.strings));

    vm.initString = nullptr;
    vm.initString = copyString("init", 4);

    defineNative("clock", clockNative);
}

void
freeVM() {
    freeTable(&(vm.globals));
    freeTable(&(vm.strings));
    vm.initString = nullptr;
    freeObjects();
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
call(ObjClosure* closure, int argCount) {
    if (argCount != closure->function->arity) {
        runtimeError("Expect %d arguments but got %d.", closure->function->arity, argCount);
        return false;
    }

    if (vm.frameCount == FRAMES_MAX) {
        runtimeError("Stack overflow.");
        return false;
    }

    CallFrame* frame = &(vm.frames[vm.frameCount++]);
    frame->closure = closure;
    frame->ip = closure->function->chunk.code;
    frame->slots = vm.stackTop - argCount - 1;
    return true;
}

static bool
callValue(Value callee, int argCount) {
    if (IS_OBJ(callee)) {
        switch (OBJ_TYPE(callee)) {
        case ObjType::OBJ_BOUND_METHOD: {
            ObjBoundMethod* bound = AS_BOUND_METHOD(callee);
            vm.stackTop[-argCount - 1] = bound->receiver;
            return call(bound->method, argCount);
        }
        case ObjType::OBJ_CLASS: {
            ObjClass* klass = AS_CLASS(callee);
            vm.stackTop[-argCount - 1] = OBJ_VAL(newInstance(klass));
            Value initializer;
            if (tableGet(&(klass->methods), vm.initString, &initializer)) {
                return call(AS_CLOSURE(initializer), argCount);
            } else if (argCount != 0) {
                runtimeError("Expect 0 arguments but got %d.", argCount);
            }
            return true;
        }
        case ObjType::OBJ_CLOSURE:
            return call(AS_CLOSURE(callee), argCount);
        case ObjType::OBJ_NATIVE: {
            NativeFn native = AS_NATIVE(callee);
            Value result = native(argCount, vm.stackTop - argCount);
            vm.stackTop -= argCount + 1;
            push(result);
            return true;
        }
        default:
            break;
        }
    }
    runtimeError("Can only call functions and classes.");
    return false;
}

static bool
invokeFromClass(ObjClass* klass, ObjString* name, int argCount) {
    Value method;
    if (!tableGet(&(klass->methods), name, &method)) {
        runtimeError("Undefined property '%s'.", name->chars);
        return false;
    }
    return call(AS_CLOSURE(method), argCount);
}

static bool
invoke(ObjString* name, int argCount) {
    Value receiver = peek(argCount);

    if (!IS_INSTANCE(receiver)) {
        runtimeError("Only instances have methods");
        return false;
    }

    ObjInstance* instance = AS_INSTANCE(receiver);

    Value value;
    if (tableGet(&(instance->fields), name, &value)) {
        vm.stackTop[-argCount - 1] = value;
        return callValue(value, argCount);
    }

    return invokeFromClass(instance->klass, name, argCount);
}

static bool
bindMethod(ObjClass* klass, ObjString* name) {
    Value method;
    if (!tableGet(&(klass->methods), name, &method)) {
        runtimeError("Undefined property '%s'.", name->chars);
        return false;
    }

    ObjBoundMethod* bound = newBoundMethod(peek(0), AS_CLOSURE(method));
    pop();
    push(OBJ_VAL(bound));
    return true;
}

static ObjUpvalue*
captureUpvalue(Value* local) {
    ObjUpvalue* prevUpvalue = nullptr;
    ObjUpvalue* upvalue = vm.openUpvalues;
    while (upvalue != nullptr && upvalue->location > local) { // NOTE: pointer address comparison
        prevUpvalue = upvalue;
        upvalue = upvalue->next;
    }

    if (upvalue != nullptr && upvalue->location == local) { // NOTE: pointer address comparison
        return upvalue;
    }

    ObjUpvalue* createdUpvalue = newUpvalue(local);
    createdUpvalue->next = upvalue;

    if (prevUpvalue == nullptr) {
        vm.openUpvalues = createdUpvalue;
    } else {
        prevUpvalue->next = createdUpvalue;
    }

    return createdUpvalue;
}

static void
closeUpvalues(Value* last) {
    while (vm.openUpvalues != nullptr && vm.openUpvalues->location >= last) { // NOTE: pointer address comparison
        ObjUpvalue* upvalue = vm.openUpvalues;
        upvalue->closed = *(upvalue->location); // NOTE: value copy
        upvalue->location = &(upvalue->closed);
        vm.openUpvalues = upvalue->next;
    }
}

static void
defineMethod(ObjString* name) {
    Value method = peek(0);
    ObjClass* klass = AS_CLASS(peek(1));
    tableSet(&(klass->methods), name, method);
    pop();
}

static bool
isFalsey(Value value) {
    return IS_NIL(value) || (IS_BOOL(value) && !AS_BOOL(value));
}

static void
concatenate() {
    ObjString* b = AS_STRING(peek(0));
    ObjString* a = AS_STRING(peek(1));

    int length = a->length + b->length;
    char* chars = ALLOCATE(char, length + 1);
    memcpy(chars, a->chars, a->length);
    memcpy(chars + a->length, b->chars, b->length);
    chars[length] = '\0';

    ObjString* result = takeString(chars, length);
    pop();
    pop();
    push(OBJ_VAL(result));
}

static InterpretResult
run() {
    CallFrame* frame = &(vm.frames[vm.frameCount - 1]);
#define READ_BYTE() (*(frame->ip++))
#define READ_CONSTANT() (frame->closure->function->chunk.constants.values[READ_BYTE()])
#define READ_SHORT() (frame->ip += 2, (uint16_t)((frame->ip[-2] << 8) | frame->ip[-1]))
#define READ_STRING() AS_STRING(READ_CONSTANT())
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
        disassembleInstruction(&(frame->closure->function->chunk),
                               (int)(frame->ip - frame->closure->function->chunk.code));
#endif

        uint8_t byte = READ_BYTE();
        OpCode instruction{u8ToOpCode(byte)};
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
        case OpCode::OP_POP: {
            pop();
            break;
        }
        case OpCode::OP_GET_LOCAL: {
            uint8_t slot = READ_BYTE();
            push(frame->slots[slot]);
            break;
        }
        case OpCode::OP_SET_LOCAL: {
            uint8_t slot = READ_BYTE();
            frame->slots[slot] = peek(0);
            break;
        }
        case OpCode::OP_GET_GLOBAL: {
            ObjString* name = READ_STRING();
            Value value;
            if (!tableGet(&(vm.globals), name, &value)) {
                runtimeError("Undefined variable '%s'.", name->chars);
                return InterpretResult::INTERPRET_RUNTIME_ERROR;
            }
            push(value);
            break;
        }
        case OpCode::OP_DEFINE_GLOBAL: {
            ObjString* name = READ_STRING();
            tableSet(&(vm.globals), name, peek(0));
            pop();
            break;
        }
        case OpCode::OP_SET_GLOBAL: {
            ObjString* name = READ_STRING();
            if (tableSet(&(vm.globals), name, peek(0))) {
                tableDelete(&(vm.globals), name);
                runtimeError("Undefined variable '%s'.", name->chars);
                return InterpretResult::INTERPRET_RUNTIME_ERROR;
            }
            break;
        }
        case OpCode::OP_GET_UPVALUE: {
            uint8_t slot = READ_BYTE();
            push(*(frame->closure->upvalues[slot]->location));
            break;
        }
        case OpCode::OP_SET_UPVALUE: {
            uint8_t slot = READ_BYTE();
            *(frame->closure->upvalues[slot]->location) = peek(0);
            break;
        }
        case OpCode::OP_GET_PROPERTY: {
            if (!IS_INSTANCE(peek(0))) {
                runtimeError("Only instances have properties.");
                return InterpretResult::INTERPRET_RUNTIME_ERROR;
            }

            ObjInstance* instance = AS_INSTANCE(peek(0));
            ObjString* name = READ_STRING();

            Value value;
            if (tableGet(&(instance->fields), name, &value)) {
                pop(); // Instance.
                push(value);
                break;
            }

            if (!bindMethod(instance->klass, name)) {
                return InterpretResult::INTERPRET_RUNTIME_ERROR;
            }
            break;
        }
        case OpCode::OP_SET_PROPERTY: {
            if (!IS_INSTANCE(peek(1))) {
                runtimeError("Only instances have properties.");
                return InterpretResult::INTERPRET_RUNTIME_ERROR;
            }

            ObjInstance* instance = AS_INSTANCE(peek(1));
            tableSet(&(instance->fields), READ_STRING(), peek(0));
            Value value = pop();
            pop();
            push(value);
            break;
        }
        case OpCode::OP_GET_SUPER: {
            ObjString* name = READ_STRING();
            ObjClass* superclass = AS_CLASS(pop());

            if (!bindMethod(superclass, name)) {
                return InterpretResult::INTERPRET_RUNTIME_ERROR;
            }
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
            if (IS_STRING(peek(0)) && IS_STRING(peek(1))) {
                concatenate();
            } else if (IS_NUMBER(peek(0)) && IS_NUMBER(peek(1))) {
                double b = AS_NUMBER(pop());
                double a = AS_NUMBER(pop());
                push(NUMBER_VAL(a + b));
            } else {
                runtimeError("Operands must be two numbers or two strings.");
                return InterpretResult::INTERPRET_RUNTIME_ERROR;
            }
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
        case OpCode::OP_PRINT: {
            printValue(pop());
            printf("\n");
            break;
        }
        case OpCode::OP_JUMP: {
            uint16_t offset = READ_SHORT();
            frame->ip += offset;
            break;
        }
        case OpCode::OP_JUMP_IF_FALSE: {
            uint16_t offset = READ_SHORT();
            if (isFalsey(peek(0))) {
                frame->ip += offset;
            }
            break;
        }
        case OpCode::OP_LOOP: {
            uint16_t offset = READ_SHORT();
            frame->ip -= offset;
            break;
        }
        case OpCode::OP_CALL: {
            int argCount = READ_BYTE();
            if (!callValue(peek(argCount), argCount)) {
                return InterpretResult::INTERPRET_RUNTIME_ERROR;
            }
            frame = &(vm.frames[vm.frameCount - 1]);
            break;
        }
        case OpCode::OP_INVOKE: {
            ObjString* method = READ_STRING();
            int argCount = READ_BYTE();
            if (!invoke(method, argCount)) {
                return InterpretResult::INTERPRET_RUNTIME_ERROR;
            }
            frame = &(vm.frames[vm.frameCount - 1]);
            break;
        }
        case OpCode::OP_SUPER_INVOKE: {
            ObjString* method = READ_STRING();
            int argCount = READ_BYTE();
            ObjClass* superclass = AS_CLASS(pop());
            if (!invokeFromClass(superclass, method, argCount)) {
                return InterpretResult::INTERPRET_RUNTIME_ERROR;
            }
            frame = &(vm.frames[vm.frameCount - 1]);
            break;
        }
        case OpCode::OP_CLOSURE: {
            ObjFunction* function = AS_FUNCTION(READ_CONSTANT());
            ObjClosure* closure = newClosure(function);
            push(OBJ_VAL(closure));
            for (int i = 0; i < closure->upvalueCount; i++) {
                uint8_t isLocal = READ_BYTE();
                uint8_t index = READ_BYTE();
                if (isLocal) {
                    closure->upvalues[i] = captureUpvalue(frame->slots + index);
                } else {
                    closure->upvalues[i] = frame->closure->upvalues[index];
                }
            }

            break;
        }
        case OpCode::OP_CLOSE_UPVALUE: {
            closeUpvalues(vm.stackTop - 1);
            pop();
            break;
        }
        case OpCode::OP_RETURN: {
            Value result = pop();
            closeUpvalues(frame->slots);
            vm.frameCount -= 1;
            if (vm.frameCount == 0) {
                pop();
                return InterpretResult::INTERPRET_OK;
            }

            vm.stackTop = frame->slots;
            push(result);
            frame = &(vm.frames[vm.frameCount - 1]);
            break;
        }
        case OpCode::OP_CLASS: {
            push(OBJ_VAL(newClass(READ_STRING())));
            break;
        }
        case OpCode::OP_INHERIT: {
            Value superclass = peek(1);
            if (!IS_CLASS(superclass)) {
                runtimeError("Superclass must be a class.");
                return InterpretResult::INTERPRET_RUNTIME_ERROR;
            }

            ObjClass* subclass = AS_CLASS(peek(0));
            tableAddAll(&(AS_CLASS(superclass)->methods), &(subclass->methods));
            pop(); // Subclass.
            break;
        }
        case OpCode::OP_METHOD: {
            defineMethod(READ_STRING());
            break;
        }
        }
    }

#undef READ_BYTE
#undef READ_SHORT
#undef READ_CONSTANT
#undef READ_STRING
#undef BINARY_OP
}

InterpretResult
interpret(const char* source) {
    ObjFunction* function = compile(source);
    if (function == nullptr) {
        return InterpretResult::INTERPRET_COMPILE_ERROR;
    }

    push(OBJ_VAL(function));
    ObjClosure* closure = newClosure(function);
    pop();
    push(OBJ_VAL(closure));
    call(closure, 0);

    return run();
}
