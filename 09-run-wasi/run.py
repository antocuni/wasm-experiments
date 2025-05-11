import wasmtime as wt

ENGINE = wt.Engine()
module = wt.Module.from_file(ENGINE, 'hello.wasm')

store = wt.Store(ENGINE)

wasi_config = wt.WasiConfig()
wasi_config.inherit_stdin()
wasi_config.inherit_stdout()
wasi_config.inherit_stderr()
store.set_wasi(wasi_config)

linker = wt.Linker(ENGINE)
linker.define_wasi()
instance = linker.instantiate(store, module)

import pdb;pdb.set_trace()

exports = instance.exports(store)
_start = exports['_start']
_start(store)
