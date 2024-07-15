import wasmtime as wt

linker = wt.Linker(wt.Engine())
linker.define_wasi()

module = wt.Module.from_file(linker.engine, 'hello.wasm')

store = wt.Store(linker.engine)

wasi_config = wt.WasiConfig()
wasi_config.inherit_stdin()
wasi_config.inherit_stdout()
wasi_config.inherit_stderr()

store.set_wasi(wasi_config)
instance = linker.instantiate(store, module)

exports = instance.exports(store)
_start = exports['_start']
_start(store)
