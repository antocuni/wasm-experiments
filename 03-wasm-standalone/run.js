const binary = require('fs').readFileSync('build/mylib.wasm');

WebAssembly.instantiate(binary).then(({ instance }) => {
    console.log('instance:', instance);
    console.log('instance.exports:', instance.exports);

    console.log();
    console.log('instance.exports.myadd(40, 2) =>', instance.exports.myadd(40, 2));

    // var repl = require("repl");
    // var context = repl.start("$ ").context;
    // context.instance = instance;
});
