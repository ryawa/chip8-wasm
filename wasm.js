const { instance } = await WebAssembly.instantiateStreaming(
    fetch("./chip8.wasm"),
    {
        js: {
            log: log,
            logInstruction: logInstruction,
            error: error,
            refreshDisplay: refreshDisplay,
        }
    }
);

const buffer = instance.exports.memory.buffer;
const wasmMemory = new Uint8Array(buffer);
const emulatorMemory = new Uint8Array(buffer, instance.exports.mem, 4096);
const display = new Uint8Array(buffer, instance.exports.display, instance.exports.display_width * instance.exports.display_height);

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

function logInstruction(inst) {
    console.log(`instruction: ${inst.toString(16)}`);
}

function error(ptr) {
    running = false;
    console.error(ptrToStr(ptr));
}

function refreshDisplay() {
    console.log("refresh");
}

const INST_PER_SEC = 700;
let running = false;
let last;
let accumulator = 0;

function start() {
    running = true;
    last = document.timeline.currentTime;
    requestAnimationFrame(step);
}

function stop() {
    running = false;
    accumulator = 0;
}

function step(now) {
    if (!running) {
        return;
    }

    accumulator += (now - last) / 1000;
    while (accumulator >= 1 / INST_PER_SEC) {
        instance.exports.step();
        accumulator -= 1 / INST_PER_SEC;
    }

    last = now;
    requestAnimationFrame(step);
}

const fileInput = document.getElementById("file-input");
fileInput.addEventListener("change", async () => {
    const bytes = await fileInput.files[0].bytes();
    emulatorMemory.set(bytes, 0x200);
    start();
})
