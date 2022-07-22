from wasmtime import Store, Module, Instance, Func, FuncType

store = Store()

module = Module.from_file(store.engine, './build/mylib.wasm')
instance = Instance(store, module, [])

exports = instance.exports(store)
res = exports['myadd'](store, 40, 2)
print('myadd(40, 2) =>', res)
