#pragma once

namespace lims {

constexpr int CONSTANT_INDEX_MAX = 255;
constexpr int UINT8_VAL_COUNT = 256; // locals' count, upvalues' count
constexpr int FRAMES_MAX = 64;
constexpr int STACK_MAX = FRAMES_MAX * UINT8_VAL_COUNT;

}
