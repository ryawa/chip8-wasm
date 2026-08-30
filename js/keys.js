let keyMap = {
    "KeyX": 0x0,
    "Digit1": 0x1,
    "Digit2": 0x2,
    "Digit3": 0x3,
    "KeyQ": 0x4,
    "KeyW": 0x5,
    "KeyE": 0x6,
    "KeyA": 0x7,
    "KeyS": 0x8,
    "KeyD": 0x9,
    "KeyZ": 0xa,
    "KeyC": 0xb,
    "Digit4": 0xc,
    "KeyR": 0xd,
    "KeyF": 0xe,
    "KeyV": 0xf,
};
let keys = new Array(16).fill(false);
document.addEventListener("keydown", (e) => {
    if (e.code in keyMap) {
        keys[keyMap[e.code]] = true;
        document.getElementById(e.code).style = "background: #aaaaaa;";
    }
});
document.addEventListener("keyup", (e) => {
    if (e.code in keyMap) {
        keys[keyMap[e.code]] = false;
        document.getElementById(e.code).style = "";
    }
});

export { keys };
