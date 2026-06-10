(module
  (import "python" "print" (func $print (param i32 i32)))
  (memory (export "memory") 1)

  (func (export "run")
    i32.const 100   ;; base pointer of string
    i32.const 13    ;; length of string
    call $print)

  (data (i32.const 100) "Hello, world!")
)
