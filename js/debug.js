import { instance, emulatorMemory, ptrToStr } from "./instance.js";

function uint16Str(x) {
    return x.toString(16).padStart(4, "0");
}

function isPrintable(x) {
    return 32 <= x && x <= 126;
}

let prevInstructionsHTML = null;
let prevMemoryHTML = null;
function updateMemoryDisplays() {
    let instructionsHTML = "";
    let memoryHTML = "";
    for (let i = 0; i < 4096; i += 16) {
        let line = `<p style="margin: 0;">${uint16Str(i)}: `;
        for (let j = 0; j < 16; j += 2) {
            const word = (emulatorMemory[i + j] << 8) | emulatorMemory[i + j + 1];
            const wordStr = uint16Str(word);

            line += `<span id="mem-${i + j}">${wordStr}</span>`;
            line += " ";

            const disas = ptrToStr(instance.exports.get_disas(word));
            instructionsHTML += `<p id="inst-${i + j}" style="margin: 0;">${uint16Str(i + j)}: ${disas}</p>`;
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
    if (instructionsHTML != prevInstructionsHTML) {
        document.getElementById("instructions").innerHTML = instructionsHTML;
        prevInstructionsHTML = instructionsHTML;
    }
    if (memoryHTML != prevMemoryHTML) {
        document.getElementById("memory").innerHTML = memoryHTML;
        prevMemoryHTML = memoryHTML;
    }
}

function updateRegisters() {
    let registersHTML = "";
    registersHTML += `PC: ${uint16Str(instance.exports.get_pc())}</br>`;
    registersHTML += `I:&nbsp;&nbsp;${uint16Str(instance.exports.get_idx())}</br>`;
    registersHTML += `DT: ${uint16Str(instance.exports.get_delay_timer())}</br>`;
    registersHTML += `ST: ${uint16Str(instance.exports.get_sound_timer())}</br>`;
    registersHTML += "</br>";
    for (let i = 0; i < 16; i++) {
        registersHTML += `V${i.toString(16)}: ${uint16Str(instance.exports.get_register(i))}</br>`;
    }
    document.getElementById("registers").innerHTML = registersHTML;
}

function updateStack() {
    let stackHTML = "";
    const stackCount = instance.exports.get_stack_count();
    console.log(stackCount);
    for (let i = 0; i < 16; i++) {
        stackHTML += `${uint16Str(instance.exports.get_stack(i))}</br>`;
        if (i + 1 == stackCount) {
            stackHTML += "<hr>";
        }
    }
    document.getElementById("stack").innerHTML = stackHTML;
}

let lastInstruction;
function refreshDebug() {
    updateMemoryDisplays();
    updateRegisters();
    updateStack();
    if (lastInstruction) {
        lastInstruction.style.background = "";
    }
    const nextInstruction = document.getElementById(`inst-${instance.exports.get_pc()}`);
    nextInstruction.scrollIntoView({"container": "nearest"});
    nextInstruction.style.background = "yellow";
    lastInstruction = nextInstruction;
}

export { refreshDebug };
