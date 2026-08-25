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
const wasmMemoryDV = new DataView(buffer);
const emulatorMemory = new Uint8Array(buffer, instance.exports.mem, 4096);
const displayW = wasmMemoryDV.getInt32(instance.exports.display_width.value, true);
const displayH = wasmMemoryDV.getInt32(instance.exports.display_height.value, true);
const display = new BigUint64Array(buffer, instance.exports.display, displayH);
const stack = new Uint16Array(buffer, instance.exports.stack_base, 16);
const textDecoder = new TextDecoder("latin1");

function uint16Str(x) {
    return x.toString(16).padStart(4, "0");
}

function isPrintable(x) {
    return 32 <= x && x <= 126;
}

function updateMemoryViews() {
    let instructionsHTML = "";
    let memoryHTML = "";
    for (let i = 0; i < 4096; i += 16) {
        let line = `<p style="margin: 0;">${uint16Str(i)}: `;
        for (let j = 0; j < 16; j += 2) {
            const word = uint16Str((emulatorMemory[i + j] << 8) | emulatorMemory[i + j + 1]);
            line += `<span id="mem-${i + j}">${word}</span>`;
            line += " ";

            instructionsHTML += `<p id="inst-${i + j}" style="margin: 0;">${uint16Str(i + j)}: ${word}</p>`;
        }
        for (let j = 0; j < 16; j++) {
            if (isPrintable(emulatorMemory[i + j])) {
                line += String.fromCharCode(emulatorMemory[i + j]);
            } else {
                line += ".";
            }
        }
        line += "</p>";
        memoryHTML += line;
    }
    document.getElementById("instructions").innerHTML = instructionsHTML;
    document.getElementById("memory").innerHTML = memoryHTML;
}

let lastInstruction;
function refreshDebug() {
    if (lastInstruction) {
        lastInstruction.style.background = "";
    }
    console.log(`getting ${instance.exports.get_pc()}`);
    const nextInstruction = document.getElementById(`inst-${instance.exports.get_pc()}`);
    nextInstruction.scrollIntoView({"container": "nearest"});
    nextInstruction.style.background = "yellow";
    lastInstruction = nextInstruction;
}

const canvas = document.getElementById("canvas");
const ctx = canvas.getContext("2d");
const canvasW = canvas.width;
const canvasH = canvas.height;
const dx = canvasW / displayW;
const dy = canvasH / displayH;
ctx.fillRect(0, 0, canvasW, canvasH);

function ptrToStr(ptr) {
    let end = ptr;
    while (wasmMemory[end] !== 0) {
        end++;
    }
    const bytes = wasmMemory.subarray(ptr, end);
    return textDecoder.decode(bytes);
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
    animationFrame = requestAnimationFrame(step);
}

function stop() {
    cancelAnimationFrame(animationFrame);
}

function step(now) {
    accumulator += (now - last) / 1000;
    while (accumulator >= 1 / INST_PER_SEC) {
        if (instance.exports.step() !== 0) {
            return;
        }
        refreshDebug();
        accumulator -= 1 / INST_PER_SEC;
    }

    last = now;
    animationFrame = requestAnimationFrame(step);
}

async function reset() {
    stop();
    instance.exports.reset();
    refreshDisplay();
    last = undefined;
    accumulator = 0;
    const bytes = await fileInput.files[0].bytes();
    emulatorMemory.set(bytes, 0x200);
    updateMemoryViews();
    refreshDebug();
}

const fileInput = document.getElementById("file-input");
fileInput.addEventListener("change", async () => {
    reset();
});

document.getElementById("start").addEventListener("click", () => {
    start();
});
document.getElementById("stop").addEventListener("click", () => {
    stop();
});
document.getElementById("step").addEventListener("click", () => {
    stop();
    instance.exports.step();
    refreshDebug();
});
document.getElementById("reset").addEventListener("click", () => {
    reset();
});
