#include "debug.h"

#include "object.h"

#include <cstdio>

void
disassembleChunk(Chunk* chunk, const char* name) {
  printf("== %s ==\n", name);

  for (int offset = 0; offset < chunk->getCount();) {
    offset = disassembleInstruction(chunk, offset);
  }
}

static int
constantInstruction(const char* name, Chunk* chunk, int offset) {
  uint8_t constantIdx = chunk->code[offset + 1];
  printf("%-16s %4d '", name, constantIdx);
  printValue(chunk->constants.values[constantIdx]);
  printf("\n");
  return offset + 2;
}

static int
invokeInstruction(const char* name, Chunk* chunk, int offset) {
  uint8_t constant = chunk->code[offset + 1];
  uint8_t argCount = chunk->code[offset + 2];
  printf("%-16s (%d args) %4d '", name, argCount, constant);
  printValue(chunk->constants.values[constant]);
  printf("'\n");
  return offset + 3;
}

static int
simpleInstruction(const char* name, int offset) {
  printf("%s\n", name);
  return offset + 1;
}

static int
byteInstruction(const char* name, Chunk* chunk, int offset) {
  uint8_t slot = chunk->code[offset + 1];
  printf("%-16s %4d\n", name, slot);
  return offset + 2;
}

static int
jumpInstruction(const char* name, int sign, Chunk* chunk, int offset) {
  uint16_t jump = (uint16_t)(chunk->code[offset + 1] << 8);
  jump |= chunk->code[offset + 2];
  printf("%-16s %4d -> %d\n", name, offset, offset + 3 + sign * jump);
  return offset + 3;
}

int
disassembleInstruction(Chunk* chunk, int offset) {
  printf("%04d ", offset);

  if (offset > 0 && chunk->lines[offset] == chunk->lines[offset - 1]) {
    printf("   | ");
  } else {
    printf("%4d ", chunk->lines[offset]);
  }

  uint8_t byte = chunk->code[offset];
  OpCode instruction{u8ToOpCode(byte)};
  switch (instruction) {
  case OpCode::OP_CONSTANT:
    return constantInstruction("OP_CONSTANT", chunk, offset);
  case OpCode::OP_NIL:
    return simpleInstruction("OP_NIL", offset);
  case OpCode::OP_TRUE:
    return simpleInstruction("OP_TRUE", offset);
  case OpCode::OP_FALSE:
    return simpleInstruction("OP_FALSE", offset);
  case OpCode::OP_POP:
    return simpleInstruction("OP_POP", offset);
  case OpCode::OP_GET_LOCAL:
    return byteInstruction("OP_GET_LOCAL", chunk, offset);
  case OpCode::OP_SET_LOCAL:
    return byteInstruction("OP_SET_LOCAL", chunk, offset);
  case OpCode::OP_GET_GLOBAL:
    return constantInstruction("OP_GET_GLOBAL", chunk, offset);
  case OpCode::OP_DEFINE_GLOBAL:
    return constantInstruction("OP_DEFINE_GLOBAL", chunk, offset);
  case OpCode::OP_SET_GLOBAL:
    return constantInstruction("OP_SET_GLOBAL", chunk, offset);
  case OpCode::OP_GET_UPVALUE:
    return byteInstruction("OP_GET_UPVALUE", chunk, offset);
  case OpCode::OP_SET_UPVALUE:
    return byteInstruction("OP_SET_UPVALUE", chunk, offset);
  case OpCode::OP_GET_PROPERTY:
    return constantInstruction("OP_GET_PROPERTY", chunk, offset);
  case OpCode::OP_SET_PROPERTY:
    return constantInstruction("OP_SET_PROPERTY", chunk, offset);
  case OpCode::OP_GET_SUPER:
    return constantInstruction("OP_GET_SUPER", chunk, offset);
  case OpCode::OP_EQUAL:
    return simpleInstruction("OP_EQUAL", offset);
  case OpCode::OP_GREATER:
    return simpleInstruction("OP_GREATER", offset);
  case OpCode::OP_LESS:
    return simpleInstruction("OP_LESS", offset);
  case OpCode::OP_ADD:
    return simpleInstruction("OP_ADD", offset);
  case OpCode::OP_SUBTRACT:
    return simpleInstruction("OP_SUBTRACT", offset);
  case OpCode::OP_MULTIPLY:
    return simpleInstruction("OP_MULTIPLY", offset);
  case OpCode::OP_DIVIDE:
    return simpleInstruction("OP_DIVIDE", offset);
  case OpCode::OP_NOT:
    return simpleInstruction("OP_NOT", offset);
  case OpCode::OP_NEGATE:
    return simpleInstruction("OP_NEGATE", offset);
  case OpCode::OP_PRINT:
    return simpleInstruction("OP_PRINT", offset);
  case OpCode::OP_JUMP:
    return jumpInstruction("OP_JUMP", 1, chunk, offset);
  case OpCode::OP_JUMP_IF_FALSE:
    return jumpInstruction("OP_JUMP_IF_FALSE", 1, chunk, offset);
  case OpCode::OP_LOOP:
    return jumpInstruction("OP_LOOP", -1, chunk, offset);
  case OpCode::OP_CALL:
    return byteInstruction("OP_CALL", chunk, offset);
  case OpCode::OP_INVOKE:
    return invokeInstruction("OP_INVOKE", chunk, offset);
  case OpCode::OP_CLOSURE: {
    offset++;
    uint8_t constant = chunk->code[offset++];
    printf("%-16s %4d ", "OP_CLOSURE", constant);
    printValue(chunk->constants.values[constant]);
    printf("\n");

    ObjFunction* function = AS_FUNCTION(chunk->constants.values[constant]);
    for (int j = 0; j < function->upvalueCount; j++) {
      int isLocal = chunk->code[offset++];
      int index = chunk->code[offset++];
      printf("%04d      |                     %s %d\n", offset - 2, isLocal ? "local" : "upvalue", index);
    }

    return offset;
  }
  case OpCode::OP_CLOSE_UPVALUE:
    return simpleInstruction("OP_CLOSE_UPVALUE", offset);
  case OpCode::OP_RETURN:
    return simpleInstruction("OP_RETURN", offset);
  case OpCode::OP_CLASS:
    return constantInstruction("OP_CLASS", chunk, offset);
  case OpCode::OP_INHERIT:
    return simpleInstruction("OP_INHERIT", offset);
  case OpCode::OP_METHOD:
    return constantInstruction("OP_METHOD", chunk, offset);
  default:
    printf("Unknown opcode %d\n", opCodeToU8(instruction));
    return offset + 1;
  }
}
