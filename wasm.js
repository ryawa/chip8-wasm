"use strict";

const { instance } = await WebAssembly.instantiateStreaming(
    fetch("./chip8.wasm"),
    {
        js: {
            log: log,
            error: error,
            refreshDisplay: refreshDisplay,
        }
    }
);

const buffer = instance.exports.memory.buffer;
const wasmMemory = new Uint8Array(buffer);
const wasmMemoryView = new DataView(buffer);
const emulatorMemory = new Uint8Array(buffer, instance.exports.mem, 4096);
const displayW = wasmMemoryView.getInt32(instance.exports.display_width.value, true);
const displayH = wasmMemoryView.getInt32(instance.exports.display_height.value, true);
const display = new BigUint64Array(buffer, instance.exports.display, displayH);

const canvas = document.getElementById("canvas");
const ctx = canvas.getContext("2d");
const canvasW = canvas.width;
const canvasH = canvas.height;
const dx = canvasW / displayW;
const dy = canvasH / displayH;

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

function refreshDisplay() {
    ctx.fillRect(0, 0, canvasW, canvasH);
    for (let i = 0; i < displayH; i++) {
        const row = display[i];
        for (let j = 0; j < displayW; j++) {
            const mask = 1n << BigInt(displayW - 1 - j);
            if (row & mask) {
                ctx.clearRect(j * dx, i * dy, dx, dy);
            }
        }
    }
}

const INST_PER_SEC = 700;
let animationFrame = null;
let last;
let accumulator = 0;

function start() {
    last = document.timeline.currentTime;
    // Clear screen before starting
    refreshDisplay();
    animationFrame = requestAnimationFrame(step);
}

function stop() {
    cancelAnimationFrame(animationFrame);
    last = undefined;
    accumulator = 0;
    instance.exports.reset();
}

function step(now) {
    accumulator += (now - last) / 1000;
    while (accumulator >= 1 / INST_PER_SEC) {
        if (instance.exports.step() !== 0) {
            return;
        }
        accumulator -= 1 / INST_PER_SEC;
    }

    last = now;
    animationFrame = requestAnimationFrame(step);
}

const fileInput = document.getElementById("file-input");
fileInput.addEventListener("change", async () => {
    stop();
    const bytes = await fileInput.files[0].bytes();
    emulatorMemory.set(bytes, 0x200);
    start();
})
