from demo import Root, RootImports, imports
from wasmtime import Store

class Host(imports.Python):
    def print(self, s: str):
        print(s)

def main():
    store = Store()
    import pdb;pdb.set_trace()
    demo = Root(store, RootImports(Host()))
    demo.run(store)

if __name__ == '__main__':
    main()
