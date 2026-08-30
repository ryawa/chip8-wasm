const canvas = document.getElementById("canvas");
const ctx = canvas.getContext("2d");

function draw(display) {
    const width = display.BYTES_PER_ELEMENT * 8;
    const height = display.length;
    const dx = canvas.width / width;
    const dy = canvas.height / height;

    ctx.fillRect(0, 0, canvas.width, canvas.height);
    for (let i = 0; i < height; i++) {
        const row = display[i];
        for (let j = 0; j < width; j++) {
            const mask = 1n << BigInt(width - 1 - j);
            if (row & mask) {
                ctx.clearRect(j * dx, i * dy, dx, dy);
            }
        }
    }
}

export { draw };
