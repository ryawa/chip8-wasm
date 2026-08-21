#define JS_IMPORT(x) __attribute__((import_module("js"), import_name(x)))

JS_IMPORT("log") void js_log(const char* str);

int add(int a, int b) {
    return a + b;
}

void print_string() {
    js_log("Hello, world!");
}
