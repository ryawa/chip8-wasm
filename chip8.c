#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>

#include "printf.h"
void _putchar(char c) { (void)c; }

#define JS_IMPORT(x) __attribute__((import_module("js"), import_name(x)))
JS_IMPORT("log") void js_log(const char *str);
JS_IMPORT("error") void js_error(const char *str);
JS_IMPORT("refreshDisplay") void js_refresh_display();
JS_IMPORT("updateMemoryDisplays") void js_update_memory_displays();
JS_IMPORT("rand") int js_rand();
JS_IMPORT("isPressed") bool js_is_pressed(uint8_t key);

void print(const char *format, ...) {
  char buffer[256];
  va_list args;
  va_start(args, format);
  vsnprintf(buffer, sizeof(buffer), format, args);
  va_end(args);

  js_log(buffer);
}

void print_error(const char *format, ...) {
  char buffer[256];
  va_list args;
  va_start(args, format);
  vsnprintf(buffer, sizeof(buffer), format, args);
  va_end(args);

  js_error(buffer);
}

#define NIBBLE(inst, n) (((inst) >> (12 - 4 * (n))) & 0x0F)
#define BYTE(inst, n) (((inst) >> (8 - 8 * (n))) & 0xFF)
#define ADDR(inst) ((inst) & 0x0FFF)

#define MAX(a, b) (((a) > (b)) ? (a) : (b))

#define WIDTH 64
#define HEIGHT 32
// Exported to JS
const int32_t display_width = WIDTH;   // NOLINT
const int32_t display_height = HEIGHT; // NOLINT

#define LOAD_ADDR 0x200
#define FONT_ADDR 0x50
uint8_t FONT[] = {
    0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
    0x20, 0x60, 0x20, 0x20, 0x70, // 1
    0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
    0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
    0x90, 0x90, 0xF0, 0x10, 0x10, // 4
    0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
    0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
    0xF0, 0x10, 0x20, 0x40, 0x40, // 7
    0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
    0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
    0xF0, 0x90, 0xF0, 0x90, 0x90, // A
    0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
    0xF0, 0x80, 0x80, 0x80, 0xF0, // C
    0xE0, 0x90, 0x90, 0x90, 0xE0, // D
    0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
    0xF0, 0x80, 0xF0, 0x80, 0x80  // F
};

uint8_t mem[4096];
uint64_t display[HEIGHT];
uint16_t pc;
uint16_t idx;
uint16_t stack_base[16];
uint16_t *stack_ptr = stack_base;
uint8_t delay_timer;
uint8_t sound_timer;
uint8_t registers[16];
uint8_t *flag = &registers[0xF];

uint16_t get_pc() {
  return pc;
}

uint16_t get_idx() {
  return idx;
}

uint8_t get_stack_offset() {
  return stack_ptr - stack_base;
}

uint8_t get_delay_timer() {
  return delay_timer;
}

uint8_t get_sound_timer() {
  return sound_timer;
}

// Should be called every time a new ROM is loaded
void reset() {
  // Because I'm too lazy to implement memset myself
  __builtin_memset(mem, 0, sizeof(mem));
  for (size_t i = 0; i < sizeof(FONT); i++) {
    mem[FONT_ADDR + i] = FONT[i];
  }
  __builtin_memset(display, 0, sizeof(display));
  pc = LOAD_ADDR;
  idx = 0;
  __builtin_memset(stack_base, 0, sizeof(stack_base));
  stack_ptr = stack_base;
  delay_timer = 0;
  sound_timer = 0;
  __builtin_memset(registers, 0, sizeof(registers));
}

void clear_display() {
  for (int i = 0; i < HEIGHT; i++) {
    display[i] = 0;
  }
}

void draw_sprite(int x, int y, int height) {
  *flag = 0;
  for (int i = 0; i < height; i++) {
    if (y + i >= HEIGHT) {
      break;
    }

    uint64_t mask = (uint64_t)mem[idx + i];
    if (x <= WIDTH - 8) {
      mask <<= ((WIDTH - 8) - x);
    } else {
      mask >>= (x - (WIDTH - 8));
    }

    if (display[y + i] & mask) {
      *flag = 1;
    }
    display[y + i] ^= mask;
  }
}

void arithmetic(uint8_t N, uint8_t *X, uint8_t Y) {
  switch (N) {
    case 0x0:
      *X = Y;
      break;
    case 0x1:
      *X |= Y;
      break;
    case 0x2:
      *X &= Y;
      break;
    case 0x3:
      *X ^= Y;
      break;
    case 0x4:
      *X += Y;
      break;
    case 0x5:
      *X -= Y;
      break;
    case 0x7:
      *X = Y - *X;
      break;
    case 0x6:
#ifdef SHIFT_SET
      *X = Y;
#endif
      *flag = *X & 1;
      *X >>= 1;
      break;
    case 0xE:
#ifdef SHIFT_SET
      *X = Y;
#endif
      *flag = *X & (1 << 7);
      *X <<= 1;
      break;
  }
}

int step() {
  uint16_t inst = mem[pc] << 8 | mem[pc + 1];
  pc += 2;

  uint8_t *X = &registers[NIBBLE(inst, 1)];
  uint8_t Y = registers[NIBBLE(inst, 2)];
  uint8_t N  = NIBBLE(inst, 3);
  uint8_t NN = BYTE(inst, 1);
  uint16_t NNN = ADDR(inst);

  switch (NIBBLE(inst, 0)) {
  case 0x0:
    switch (inst) {
      // Clear screen
      case 0x00E0:
        clear_display();
        js_refresh_display();
        break;
      // Return
      case 0x00EE:
        pc = *stack_ptr;
        stack_ptr--;
        break;
      default:
        print_error("Expected instruction 0x00E0 or 0x00EE");
    }
    break;
  // Jump
  case 0x1:
    pc = NNN;
    break;
  // Call
  case 0x2:
    stack_ptr++;
    *stack_ptr = pc;
    pc = NNN;
    break;
  // Jump if equal to value
  case 0x3:
    if (*X == NN) {
      pc += 2;
    }
    break;
  // Jump if not equal to value
  case 0x4:
    if (*X != NN) {
      pc += 2;
    }
    break;
  // Jump if registers equal
  case 0x5:
    if (N != 0) {
      print_error("Expected last nibble of opcode 0x5 to be 0x0");
      return 1;
    }
    if (*X == Y) {
        pc += 2;
    }
    break;
  // Move value to register
  case 0x6:
    *X = NN;
    break;
  // Add value to register
  case 0x7:
    *X += NN;
    break;
  // Arithmetic
  case 0x8:
    arithmetic(N, X, Y);
    break;
  // Jump if registers not equal
  case 0x9:
    if (N != 0) {
      print_error("Expected last nibble of opcode 0x9 to be 0x0");
      return 1;
    }
    if (*X != Y) {
        pc += 2;
    }
    break;
  // Set index
  case 0xA:
    idx = NNN;
    break;
  // Jump with offset
  case 0xB:
#ifdef JUMP_OFFSET_REG
    uint8_t offset = *X;
#else
    uint8_t offset = registers[0];
#endif
    idx = NNN + offset;
    break;
  // Random
  case 0xC:
    *X = js_rand() & NN;
    break;
  // Draw
  case 0xD:
    uint8_t x = *X % WIDTH;
    uint8_t y = Y % HEIGHT;
    draw_sprite(x, y, N);
    js_refresh_display();
    break;
  case 0xE:
    switch (BYTE(inst, 1)) {
      // Skip if key
      case 0x9E:
        if (js_is_pressed(*X)) {
          pc += 2;
        }
        break;
      // Skip if not key
      case 0xA1:
        if (!js_is_pressed(*X)) {
          pc += 2;
        }
        break;
      default:
        print_error("Expected instruction 0xE_9E or 0xE_A1");
    }
    break;
  default:
    print_error("Unknown instruction 0x%x", inst);
    return 1;
  }
  return 0;
}
