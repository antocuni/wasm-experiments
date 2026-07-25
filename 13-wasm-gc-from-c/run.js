// Host runner for combined.wasm. Provides `host.print_i32` and calls `run`.
const fs = require("fs");

const bytes = fs.readFileSync(process.argv[2] || "combined.wasm");

WebAssembly.instantiate(bytes, {
    host: {
        print_i32: (v) => console.log(v),
    },
}).then(({ instance }) => {
    const result = instance.exports.run();
    console.log("run() =>", result);
});
