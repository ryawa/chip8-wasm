#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "printf.h"
void _putchar(char c) { (void)c; }

#define MAX(a, b) (((a) > (b)) ? (a) : (b))

#define DISPLAY_WIDTH 64
#define DISPLAY_HEIGHT 32

#define LOAD_ADDR 0x200
#define FONT_ADDR 0x50

extern char __heap_base;
char *bump_ptr = &__heap_base;
void *malloc(size_t sz) {
  char *p = bump_ptr;
  bump_ptr += sz;
  return p;
}

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
uint64_t display[DISPLAY_HEIGHT];
uint16_t pc;
uint16_t idx;
uint16_t stack[16];
uint8_t stack_count;
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

uint8_t get_stack_count() {
  return stack_count;
}

uint8_t get_delay_timer() {
  return delay_timer;
}

uint8_t get_sound_timer() {
  return sound_timer;
}

// Should be called every time a new ROM is loaded
void reset() {
  __builtin_memset(mem, 0, sizeof(mem));
  for (size_t i = 0; i < sizeof(FONT); i++) {
    mem[FONT_ADDR + i] = FONT[i];
  }
  __builtin_memset(display, 0, sizeof(display));
  pc = LOAD_ADDR;
  idx = 0;
  __builtin_memset(stack, 0, sizeof(stack));
  stack_count = 0;
  delay_timer = 0;
  sound_timer = 0;
  __builtin_memset(registers, 0, sizeof(registers));
}

void clear_display() {
  for (int i = 0; i < DISPLAY_HEIGHT; i++) {
    display[i] = 0;
  }
}

void draw_sprite(int x, int y, int height) {
  *flag = 0;
  for (int i = 0; i < height; i++) {
    if (y + i >= DISPLAY_HEIGHT) {
      break;
    }

    uint64_t mask = (uint64_t)mem[idx + i];
    if (x <= DISPLAY_WIDTH - 8) {
      mask <<= ((DISPLAY_WIDTH - 8) - x);
    } else {
      mask >>= (x - (DISPLAY_WIDTH - 8));
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

enum op {
  CLEAR_SCREEN,
  JUMP,
  RETURN,
  CALL,
  SKIP_EQ_CONST,
  SKIP_NEQ_CONST,
  SKIP_EQ,
  SKIP_NEQ,
  MOVE_CONST,
  ADD_CONST,
  MOVE,
  OR,
  AND,
  XOR,
  ADD,
  SUBTRACT,
  SUBTRACT_REVERSE,
  SHIFT_RIGHT,
  SHIFT_LEFT,
  MOVE_INDEX,
  JUMP_INDEX,
  RANDOM,
  DRAW,
  SKIP_KEY_DOWN,
  SKIP_KEY_UP,
  GET_DELAY,
  SET_DELAY,
  SET_SOUND,
  ADD_INDEX,
  WAIT_KEY,
  FONT_INDEX,
  BCD,
  STORE,
  LOAD
};

// Packing and order is platform-dependent but it doesn't matter since we only
// care about compiling to WASM.
// Type 1: oxyn
// Type 2: oxnn
// Type 3: onnn
union encoded {
  uint16_t raw;
  struct { uint16_t opcode: 4, x: 4, y: 4, n: 4; } xyn;
  struct { uint16_t opcode: 4, x: 4, nn: 8; } xnn;
  struct { uint16_t opcode: 4, nnn: 12; } nnn;
};

struct insn {
  char disas[16];
  enum op op;
  // Arguments
  uint16_t a;
  uint16_t b;
  uint16_t c;
};

int decode(struct insn *i, uint16_t raw) {
  union encoded enc = { .raw = raw };
  sprintf(i->disas, "???");
  switch (enc.xyn.opcode) {
    case 0x0:
      switch (enc.raw) {
        case 0x00E0:
          i->op = CLEAR_SCREEN;
          sprintf(i->disas, "cls");
          break;
        case 0x00EE:
          i->op = RETURN;
          sprintf(i->disas, "rts");
          break;
        default:
          print_error("Expected instruction 0x00E0 or 0x00EE");
          return 1;
      }
      break;
    case 0x1:
      i->op = JUMP;
      i->a = enc.nnn.nnn;
      sprintf(i->disas, "jmp %x", enc.nnn.nnn);
      break;
    case 0x2:
      i->op = CALL;
      i->a = enc.nnn.nnn;
      sprintf(i->disas, "jsr %x", i->a);
      break;
    case 0x3:
      i->op = SKIP_EQ_CONST;
      i->a = enc.xnn.x;
      i->b = enc.xnn.nn;
      sprintf(i->disas, "skeq v%x, %x", i->a, i->b);
      break;
    case 0x4:
      i->op = SKIP_NEQ_CONST;
      i->a = enc.xnn.x;
      i->b = enc.xnn.nn;
      sprintf(i->disas, "skne v%x, %x", i->a, i->b);
      break;
    case 0x5:
      if (enc.xyn.n != 0) {
        print_error("Expected last nibble of opcode 0x5 to be 0x0");
        return 1;
      }
      i->op = SKIP_EQ;
      i->a = enc.xyn.x;
      i->b = enc.xyn.y;
      sprintf(i->disas, "skeq v%x, v%x", i->a, i->b);
      break;
    case 0x6:
      i->op = MOVE_CONST;
      i->a = enc.xnn.x;
      i->b = enc.xnn.nn;
      sprintf(i->disas, "mov v%x, %x", i->a, i->b);
      break;
    case 0x7:
      i->op = ADD_CONST;
      i->a = enc.xnn.x;
      i->b = enc.xnn.nn;
      sprintf(i->disas, "add v%x, %x", i->a, i->b);
      break;
    case 0x8: {
      i->a = enc.xyn.x;
      i->b = enc.xyn.y;
      char *op_str;
      switch (enc.xyn.n) {
        case 0x0:
          i->op = MOVE;
          op_str = "mov";
          break;
        case 0x1:
          i->op = OR;
          op_str = "or";
          break;
        case 0x2:
          i->op = AND;
          op_str = "and";
          break;
        case 0x3:
          i->op = XOR;
          op_str = "xor";
          break;
        case 0x4:
          i->op = ADD;
          op_str = "add";
          break;
        case 0x5:
          i->op = SUBTRACT;
          op_str = "sub";
          break;
        case 0x6:
          i->op = SHIFT_RIGHT;
          op_str = "shr";
          break;
        case 0x7:
          i->op = SUBTRACT_REVERSE;
          op_str = "rsb";
          break;
        case 0xE:
          i->op = SHIFT_LEFT;
          op_str = "shl";
          break;
        default:
          print_error("Unknown arithmetic operation");
          return 1;
      }
      sprintf(i->disas, "%s v%x, v%x", op_str, i->a, i->b);
      break;
      }
    case 0x9:
      if (enc.xyn.n != 0) {
        print_error("Expected last nibble of opcode 0x9 to be 0x0");
        return 1;
      }
      i->op = SKIP_NEQ;
      i->a = enc.xyn.x;
      i->b = enc.xyn.y;
      sprintf(i->disas, "skne v%x, v%x", i->a, i->b);
      break;
    case 0xA:
      i->op = MOVE_INDEX;
      i->a = enc.nnn.nnn;
      sprintf(i->disas, "mvi %x", i->a);
      break;
    case 0xB:
      i->op = JUMP_INDEX;
      i->a = enc.nnn.nnn;
      i->b = enc.xnn.x;
      sprintf(i->disas, "jmi %x", i->a);
      break;
    case 0xC:
      i->op = RANDOM;
      i->a = enc.xnn.x;
      i->b = enc.xnn.nn;
      sprintf(i->disas, "rand v%x, %x", i->a, i->b);
      break;
    case 0xD:
      i->op = DRAW;
      i->a = enc.xyn.x;
      i->b = enc.xyn.y;
      i->c = enc.xyn.n;
      sprintf(i->disas, "sprite v%x, v%x, %x", i->a, i->b, i->c);
      break;
    case 0xE:
      switch (enc.xnn.nn) {
        case 0x9E:
          i->op = SKIP_KEY_DOWN;
          i->a = enc.xnn.x;
          sprintf(i->disas, "skpr %x", i->a);
          break;
        case 0xA1:
          i->op = SKIP_KEY_UP;
          i->a = enc.xnn.x;
          sprintf(i->disas, "skup %x", i->a);
          break;
        default:
          print_error("Expected instruction 0xE_9E or 0xE_A1");
          return 1;
      }
      break;
    case 0xF: {
      char *op_str;
      switch (enc.xnn.nn) {
        i->a = enc.xnn.x;
        case 0x07:
          i->op = GET_DELAY;
          op_str = "gdelay";
          break;
        case 0x0A:
          i->op = WAIT_KEY;
          op_str = "key";
          break;
        case 0x15:
          i->op = SET_DELAY;
          op_str = "sdelay";
          break;
        case 0x18:
          i->op = SET_SOUND;
          op_str = "ssound";
          break;
        case 0x1E:
          i->op = ADD_INDEX;
          op_str = "adi";
          break;
        case 0x29:
          i->op = FONT_INDEX;
          op_str = "font";
          break;
        case 0x33:
          i->op = BCD;
          op_str = "bcd";
          break;
        case 0x55:
          i->op = STORE;
          op_str = "str v0-";
          break;
        case 0x65:
          i->op = LOAD;
          op_str = "ldr v0-";
          break;
        default:
          print_error("Unknown opcode 0xF instruction");
          return 1;
      }
      sprintf(i->disas, "%s %sv%x", op_str, enc.xnn.nn <= 0x33 ? " " : "", i->a);
      break;
      }
  }
  return 0;
}

const char *get_disas(uint16_t raw) {
  struct insn *i = malloc(sizeof(struct insn));
  decode(i, raw);
  return i->disas;
  // Intentional memory leak
}

int step() {
  /* uint16_t inst = mem[pc] << 8 | mem[pc + 1]; */
  /* pc += 2; */

  /* uint8_t *X = &registers[NIBBLE(inst, 1)]; */
  /* uint8_t Y = registers[NIBBLE(inst, 2)]; */
  /* uint8_t N  = NIBBLE(inst, 3); */
  /* uint8_t NN = BYTE(inst, 1); */
  /* uint16_t NNN = ADDR(inst); */

  /* switch (NIBBLE(inst, 0)) { */
  /* case 0x0: */
  /*   switch (inst) { */
  /*     // Clear screen */
  /*     case 0x00E0: */
  /*       clear_display(); */
  /*       js_refresh_display(); */
  /*       break; */
  /*     // Return */
  /*     case 0x00EE: */
  /*       stack_count--; */
  /*       pc = stack[stack_count]; */
  /*       break; */
  /*     default: */
  /*       print_error("Expected instruction 0x00E0 or 0x00EE"); */
  /*       return 1; */
  /*   } */
  /*   break; */
  /* // Jump */
  /* case 0x1: */
  /*   pc = NNN; */
  /*   break; */
  /* // Call */
  /* case 0x2: */
  /*   stack[stack_count] = pc; */
  /*   stack_count++; */
  /*   pc = NNN; */
  /*   break; */
  /* // Jump if equal to value */
  /* case 0x3: */
  /*   if (*X == NN) { */
  /*     pc += 2; */
  /*   } */
  /*   break; */
  /* // Jump if not equal to value */
  /* case 0x4: */
  /*   if (*X != NN) { */
  /*     pc += 2; */
  /*   } */
  /*   break; */
  /* // Jump if registers equal */
  /* case 0x5: */
  /*   if (N != 0) { */
  /*     print_error("Expected last nibble of opcode 0x5 to be 0x0"); */
  /*     return 1; */
  /*   } */
  /*   if (*X == Y) { */
  /*       pc += 2; */
  /*   } */
  /*   break; */
  /* // Move value to register */
  /* case 0x6: */
  /*   *X = NN; */
  /*   break; */
  /* // Add value to register */
  /* case 0x7: */
  /*   *X += NN; */
  /*   break; */
  /* // Arithmetic */
  /* case 0x8: */
  /*   arithmetic(N, X, Y); */
  /*   break; */
  /* // Jump if registers not equal */
  /* case 0x9: */
  /*   if (N != 0) { */
  /*     print_error("Expected last nibble of opcode 0x9 to be 0x0"); */
  /*     return 1; */
  /*   } */
  /*   if (*X != Y) { */
  /*       pc += 2; */
  /*   } */
  /*   break; */
  /* // Set index */
  /* case 0xA: */
  /*   idx = NNN; */
  /*   break; */
  /* // Jump with offset */
  /* case 0xB: */
/* #ifdef JUMP_OFFSET_REG */
  /*   uint8_t offset = *X; */
/* #else */
  /*   uint8_t offset = registers[0]; */
/* #endif */
  /*   pc = NNN + offset; */
  /*   break; */
  /* // Random */
  /* case 0xC: */
  /*   *X = js_rand() & NN; */
  /*   break; */
  /* // Draw */
  /* case 0xD: */
  /*   uint8_t x = *X % DISPLAY_WIDTH; */
  /*   uint8_t y = Y % DISPLAY_HEIGHT; */
  /*   draw_sprite(x, y, N); */
  /*   js_refresh_display(); */
  /*   break; */
  /* case 0xE: */
  /*   switch (BYTE(inst, 1)) { */
  /*     // Skip if key */
  /*     case 0x9E: */
  /*       if (js_is_pressed(*X)) { */
  /*         pc += 2; */
  /*       } */
  /*       break; */
  /*     // Skip if not key */
  /*     case 0xA1: */
  /*       if (!js_is_pressed(*X)) { */
  /*         pc += 2; */
  /*       } */
  /*       break; */
  /*     default: */
  /*       print_error("Expected instruction 0xE_9E or 0xE_A1"); */
  /*   } */
  /*   break; */
  /* default: */
  /*   print_error("Unknown instruction 0x%x", inst); */
  /*   return 1; */
  /* } */
  /* return 0; */
}
