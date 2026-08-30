import { instance, emulatorMemory } from "./instance.js";

function uint16Str(x) {
    return x.toString(16).padStart(4, "0");
}

function isPrintable(x) {
    return 32 <= x && x <= 126;
}

function updateMemoryDisplays() {
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
    const nextInstruction = document.getElementById(`inst-${instance.exports.get_pc()}`);
    nextInstruction.scrollIntoView({"container": "nearest"});
    nextInstruction.style.background = "yellow";
    lastInstruction = nextInstruction;
}

export { updateMemoryDisplays, refreshDebug };
