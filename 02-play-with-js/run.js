var factory = require('./build/mylib.js');
var repl = require("repl");

function chr(n) {
    return String.fromCharCode(n);
}

function ord(s) {
    return s.charCodeAt(0);
}

function dumpString(Module, p) {
    let q = p;
    while(1) {
        let val = Module.HEAPU8[q];
        let sVal = (val + "").padStart(3, " ");
        let ch = chr(val);
        console.log(`      Module.HEAPU8[${q}] == ${sVal}  '${ch}'`)
        if (val == 0)
            break;
        q++;
    }
}


factory().then((Module) => {
    let say_hello = Module.cwrap('say_hello', undefined, []);
    let mypow = Module.cwrap('mypow', 'number', ['number', 'number'])
    let print_str = Module.cwrap('print_str', undefined, ['number']);
    let get_myname = Module.cwrap('get_myname', 'number', []);

    console.log('[ccall say_hello]')
    Module.ccall('say_hello');
    console.log()

    console.log('[cwrap say_hello]')
    say_hello();
    console.log()

    let x = Module.ccall('mypow', 'number', ['number', 'number'], [2, 5]);
    console.log('[ccall mypow]: 2**5 ==', x)
    console.log('[crwap mypow]: 2**6 ==', mypow(2, 6))
    console.log()

    let p = Module.allocateUTF8("Hello JS");
    console.log('[allocateUTF8] p ==', p)
    dumpString(Module, p);
    console.log();
    console.log('[calling print_str(p)]');
    print_str(p);

    console.log();
    let p_myname = get_myname(); // this is a number
    console.log(`[get_myname()] p_myname == ${p_myname}`);
    dumpString(Module, p_myname);
    say_hello();

    console.log();
    console.log('[Modify the memory pointed by a const char* (!!!)]')
    Module.HEAPU8[p_myname] = ord("A");
    Module.HEAPU8[p_myname+1] = ord("N");
    Module.HEAPU8[p_myname+2] = ord("T");
    Module.HEAPU8[p_myname+3] = ord("O");
    dumpString(Module, p_myname)
    say_hello();

    // uncomment this is you want to start a REPL
    // let c = repl.start("$ ").context;
    // c.Module = Module;
});
