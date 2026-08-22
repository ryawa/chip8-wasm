#include <stddef.h>
#include <stdint.h>

#include "printf.h"
void _putchar(char c) {
  (void) c;
}

#define JS_IMPORT(x) __attribute__((import_module("js"), import_name(x)))
JS_IMPORT("log") void js_log(const char *str);
JS_IMPORT("error") void js_error(const char *str);
JS_IMPORT("refreshDisplay") void js_refresh_display();

void print(const char *format, ...) {
  char buffer[256];
  va_list args;
  va_start(args, format);
  vsnprintf(buffer, sizeof(buffer), format, args);
  va_end(args);

  js_log(buffer);
}

void error(const char *format, ...) {
  char buffer[256];
  va_list args;
  va_start(args, format);
  vsnprintf(buffer, sizeof(buffer), format, args);
  va_end(args);

  js_error(buffer);
}


#define WIDTH 64
#define HEIGHT 32
static const int display_width = WIDTH;   // NOLINT
static const int display_height = HEIGHT; // NOLINT

#define NIBBLE(op, n) (((op) >> (12 - 4 * (n))) & 0x0F)
#define BYTE(op, n) (((op) >> (8 - 8 * (n))) & 0xFF)
#define ADDR(op) ((op) & 0x0FFF)

#define MAX(a, b) (((a) > (b)) ? (a) : (b))

uint8_t mem[4096] = {
    [0x50] = 0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
    0x20,          0x60, 0x20, 0x20, 0x70, // 1
    0xF0,          0x10, 0xF0, 0x80, 0xF0, // 2
    0xF0,          0x10, 0xF0, 0x10, 0xF0, // 3
    0x90,          0x90, 0xF0, 0x10, 0x10, // 4
    0xF0,          0x80, 0xF0, 0x10, 0xF0, // 5
    0xF0,          0x80, 0xF0, 0x90, 0xF0, // 6
    0xF0,          0x10, 0x20, 0x40, 0x40, // 7
    0xF0,          0x90, 0xF0, 0x90, 0xF0, // 8
    0xF0,          0x90, 0xF0, 0x10, 0xF0, // 9
    0xF0,          0x90, 0xF0, 0x90, 0x90, // A
    0xE0,          0x90, 0xE0, 0x90, 0xE0, // B
    0xF0,          0x80, 0x80, 0x80, 0xF0, // C
    0xE0,          0x90, 0x90, 0x90, 0xE0, // D
    0xF0,          0x80, 0xF0, 0x80, 0xF0, // E
    0xF0,          0x80, 0xF0, 0x80, 0x80  // F
};
uint64_t display[HEIGHT];
uint16_t pc = 0x200;
uint16_t idx;
uint16_t stack_base[16];
uint16_t *stack_ptr = stack_base;
uint8_t delay_timer;
uint8_t sound_timer;
uint8_t registers[16];
uint8_t *flag = &registers[15];

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

    uint64_t mask = mem[idx];
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

void step() {
  print("PC = %x", pc);
  uint16_t inst = mem[pc] << 8 | mem[pc + 1];
  pc += 2;

  print("instruction = %x", inst);
  switch (NIBBLE(inst, 0)) {
  case 0x0:
    if (inst != 0x00E0) {
      error("Expected instruction 0x00E0");
    } else {
      clear_display();
      js_refresh_display();
    }
    break;
  case 0x1:
    pc = ADDR(inst);
    break;
  case 0x6:
    registers[NIBBLE(inst, 1)] = BYTE(inst, 1);
    break;
  case 0x7:
    registers[NIBBLE(inst, 1)] += BYTE(inst, 1);
    break;
  case 0xA:
    idx = ADDR(inst);
    break;
  case 0xD:
    uint8_t x = registers[NIBBLE(inst, 1)] % 64;
    uint8_t y = registers[NIBBLE(inst, 2)] % 32;
    draw_sprite(x, y, NIBBLE(inst, 3));
    break;
  }
}
