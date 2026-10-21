import { instance, emulatorMemory, refreshDisplay } from "./instance.js";
import { updateMemoryDisplays, refreshDebug } from "./debug.js";

const INSN_HZ = 700;
const TIMER_HZ = 60;
let animationFrame = null;
let last;
let insnAccumulator = 0;
let timerAccumulator = 0;
let running = false;

const audioCtx = new AudioContext();
let oscillator = null;
let prevSoundTimer = 0;

function playSound() {
    oscillator = audioCtx.createOscillator();
    oscillator.type = "square";
    oscillator.frequency.setValueAtTime(440, audioCtx.currentTime);
    oscillator.connect(audioCtx.destination);
    oscillator.start();
}

function stopSound() {
    if (oscillator) oscillator.stop();
}

function start() {
    if (running) return;
    running = true;
    last = document.timeline.currentTime;
    animationFrame = requestAnimationFrame(step);
}

function stop() {
    running = false;
    stopSound();
    cancelAnimationFrame(animationFrame);
}

function step(now) {
    insnAccumulator += (now - last) / 1000;
    while (insnAccumulator >= 1 / INSN_HZ) {
        if (instance.exports.step() !== 0) {
            return;
        }
        refreshDebug();
        insnAccumulator -= 1 / INSN_HZ;
    }
    timerAccumulator += (now - last) / 1000;
    while (timerAccumulator >= 1 / TIMER_HZ) {
        instance.exports.update_timers();
        const soundTimer = instance.exports.get_sound_timer();
        if (soundTimer > 0 && prevSoundTimer == 0) {
            playSound();
        } else if (soundTimer == 0 && prevSoundTimer > 0) {
            stopSound();
        }
        prevSoundTimer = soundTimer;
        timerAccumulator -= 1 / TIMER_HZ;
    }

    last = now;
    animationFrame = requestAnimationFrame(step);
}

async function reset() {
    stop();
    instance.exports.reset();
    refreshDisplay();
    last = undefined;
    insnAccumulator = 0;
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
