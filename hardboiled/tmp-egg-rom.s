.globl egg_rom_bundled
.globl egg_rom_bundled_length
egg_rom_bundled: .incbin "out/hardboiled.egg-wasmless"
egg_rom_bundled_length: .int (egg_rom_bundled_length-egg_rom_bundled)
