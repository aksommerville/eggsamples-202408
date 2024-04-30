(module
  (import "env" "egg_log" (func $egg_log (param i32) (param i32)))
  (import "env" "egg_texture_new" (func $egg_texture_new (result i32)))
  (import "env" "egg_texture_load_image" (func $egg_texture_load_image (param i32) (param i32) (param i32) (result i32)))
  (import "env" "egg_draw_rect" (func $egg_draw_rect (param i32) (param i32) (param i32) (param i32) (param i32) (param i32)))
  (import "env" "egg_draw_tile" (func $egg_draw_tile (param i32) (param i32) (param i32) (param i32)))
  
  (memory $memory 1)
  (export "memory" (memory $memory))
  ;; 4: texid
  ;; 8..255: reserve for tile list
  ;; 256: f32 monster x
  ;; 260: f32 monster y
  ;; 264: u8 monster tileid
  ;; 265..267: pad
  ;; 268: f32 monster dx
  ;; 272: f32 monster dy

  (func $egg_client_init (result i32)
    (i32.store (i32.const 4) (call $egg_texture_new))
    (call $egg_texture_load_image (i32.load (i32.const 4)) (i32.const 0) (i32.const 1))
    drop
    
    (f32.store (i32.const 256) (f32.const 50))
    (f32.store (i32.const 260) (f32.const 30))
    (i32.store8 (i32.const 264) (i32.const 0x10))
    (f32.store (i32.const 268) (f32.const 60.0))
    (f32.store (i32.const 272) (f32.const 20.0))
    
    i32.const 0
  )
  
  (func $egg_client_update (param $elapsed f64)
    (f32.store (i32.const 256) (f32.add
      (f32.load (i32.const 256))
      (f32.mul (f32.load (i32.const 268)) (f32.demote_f64 (local.get $elapsed)))
    ))
    (f32.gt (f32.load (i32.const 256)) (f32.const 128.0))
    (if (then (f32.store (i32.const 256) (f32.const 0))))
    (f32.store (i32.const 260) (f32.add
      (f32.load (i32.const 260))
      (f32.mul (f32.load (i32.const 272)) (f32.demote_f64 (local.get $elapsed)))
    ))
    (f32.gt (f32.load (i32.const 260)) (f32.const 96.0))
    (if (then (f32.store (i32.const 260) (f32.const 0))))
  )
  
  (func $egg_client_render
    (call $egg_draw_rect (i32.const 1) (i32.const 0) (i32.const 0) (i32.const 128) (i32.const 96) (i32.const 0x202030ff))
    
    (i32.store16 (i32.const 8) (i32.trunc_f32_s (f32.load (i32.const 256)))) ;; x
    (i32.store16 (i32.const 10) (i32.trunc_f32_s (f32.load (i32.const 260)))) ;; y
    (i32.store8 (i32.const 12) (i32.load8_u (i32.const 264))) ;; tileid
    (i32.store8 (i32.const 13) (i32.const 0)) ;; xform
    (call $egg_draw_tile (i32.const 1) (i32.load (i32.const 4)) (i32.const 8) (i32.const 1))
  )
  
  (export "egg_client_init" (func $egg_client_init))
  (export "egg_client_update" (func $egg_client_update))
  (export "egg_client_render" (func $egg_client_render))
)
