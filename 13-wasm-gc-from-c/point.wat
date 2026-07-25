(module
  (type $Point (struct
    (field $x (mut i32))
    (field $y (mut i32))))

  (func (export "alloc_Point") (param $x i32) (param $y i32) (result externref)
    (extern.convert_any
      (struct.new $Point (local.get $x) (local.get $y))))

  (func (export "get_x") (param $p externref) (result i32)
    (struct.get $Point $x
      (ref.cast (ref $Point) (any.convert_extern (local.get $p)))))

  (func (export "get_y") (param $p externref) (result i32)
    (struct.get $Point $y
      (ref.cast (ref $Point) (any.convert_extern (local.get $p)))))

  (func (export "set_x") (param $p externref) (param $v i32)
    (struct.set $Point $x
      (ref.cast (ref $Point) (any.convert_extern (local.get $p)))
      (local.get $v)))

  (func (export "set_y") (param $p externref) (param $v i32)
    (struct.set $Point $y
      (ref.cast (ref $Point) (any.convert_extern (local.get $p)))
      (local.get $v)))
)
