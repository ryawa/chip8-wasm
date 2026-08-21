const { instance } = await WebAssembly.instantiateStreaming(
    fetch("./chip8.wasm"),
    {
        js: {
            log: function(ptr) {
                const memoryBuffer = new Uint8Array(instance.exports.memory.buffer);
                let end = ptr;
                while (memoryBuffer[end] !== 0) {
                    end++;
                }
                const bytes = memoryBuffer.subarray(ptr, end);
                const str = new TextDecoder("latin1").decode(bytes);
                console.log(str);
            }
        },
    }
);
console.log(instance.exports.add(4, 2));
instance.exports.print_string();
