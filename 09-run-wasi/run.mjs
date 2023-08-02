// -*- mode: js -*-

import fs from "fs";
import { init, WASI } from '@wasmer/wasi';

// This is needed to load the WASI library first (since is a Wasm module)
await init();

let hello_wasi = new WASI({
  env: {
      // 'ENVVAR1': '1',
      // 'ENVVAR2': '2'
  },
  args: [
      // 'command', 'arg1', 'arg2'
  ],
});


const buf = fs.readFileSync('hello.wasm');
const module = await WebAssembly.compile(
  new Uint8Array(buf)
);
await hello_wasi.instantiate(module, {});

// Run the start function
let exitCode = hello_wasi.start();
let stdout = hello_wasi.getStdoutString();

console.log(`${stdout}`);
console.log(`exit code: ${exitCode}`);
