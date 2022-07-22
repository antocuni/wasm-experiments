var repl = require("repl");
var context = repl.start("$ ").context;

// Configure what’s available in the REPL
context.util = require("util");
