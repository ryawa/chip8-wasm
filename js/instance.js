import { draw } from "./canvas.js";
import { keys } from "./keys.js";

const { instance } = await WebAssembly.instantiateStreaming(
    fetch("./c/chip8.wasm"),
    {
        js: {
            log: log,
            error: error,
            refreshDisplay: refreshDisplay,
            rand: () => Math.floor(Math.random() * Math.pow(2, 31) - 1),
            isPressed: (key) => keys[key],
        }
    }
);

const buffer = instance.exports.memory.buffer;
const wasmMemory = new Uint8Array(buffer);
const emulatorMemory = new Uint8Array(buffer, instance.exports.mem, 4096);
const display = new BigUint64Array(buffer, instance.exports.display, 32);
const stack = new Uint16Array(buffer, instance.exports.stack, 16);

function refreshDisplay() {
    draw(display);
}

function ptrToStr(ptr) {
    let end = ptr;
    while (wasmMemory[end] !== 0) {
        end++;
    }
    const bytes = wasmMemory.subarray(ptr, end);
    return new TextDecoder("latin1").decode(bytes);
}

function log(ptr) {
    console.log(ptrToStr(ptr));
}

function error(ptr) {
    console.error(ptrToStr(ptr));
}

export { instance, emulatorMemory, refreshDisplay, stack };
