// NOTE: this is
var Module = require('./build/hellofs.js');
var repl = require("repl");


factory().then((Module) => {
    // uncomment this is you want to start a REPL
    let c = repl.start("$ ").context;
    c.Module = Module;
});
