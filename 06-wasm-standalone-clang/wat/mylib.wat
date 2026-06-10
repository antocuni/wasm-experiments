(module
  (type (;0;) (func (param i32)))
  (type (;1;) (func))
  (type (;2;) (func (param i32 i32) (result i32)))
  (import "env" "some_external_function" (func (;0;) (type 0)))
  (func (;1;) (type 1)
    nop)
  (func (;2;) (type 2) (param i32 i32) (result i32)
    block  ;; label = @1
      local.get 0
      i32.const 0
      i32.gt_s
      if  ;; label = @2
        i32.const 123
        return
      end
      i32.const 456
      return
    end
    i32.const 789
    return)
  (memory (;0;) 2)
  (global (;0;) i32 (i32.const 1024))
  (global (;1;) i32 (i32.const 1024))
  (global (;2;) i32 (i32.const 1024))
  (global (;3;) i32 (i32.const 66560))
  (global (;4;) i32 (i32.const 0))
  (global (;5;) i32 (i32.const 1))
  (export "memory" (memory 0))
  (export "__wasm_call_ctors" (func 1))
  (export "foo" (func 2))
  (export "__dso_handle" (global 0))
  (export "__data_end" (global 1))
  (export "__global_base" (global 2))
  (export "__heap_base" (global 3))
  (export "__memory_base" (global 4))
  (export "__table_base" (global 5)))
