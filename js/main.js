import { instance, emulatorMemory, refreshDisplay } from "./instance.js";
import { updateMemoryDisplays, refreshDebug } from "./debug.js";

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
    updateMemoryDisplays();
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

refreshDisplay();
updateMemoryDisplays();
