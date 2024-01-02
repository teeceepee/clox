#include "chunk.h"
#include "memory.h"
#include "vm.h"

void
Chunk::writeChunk(uint8_t byte, int line) {
  this->code.push(byte);
  this->lines.push(line);
}

int
Chunk::addConstant(Value value) {
  push(value);
  const int idx = this->constants.writeValue(value);
  pop();
  return idx;
}

int
Chunk::getCount() const {
  return this->code.count;
}
