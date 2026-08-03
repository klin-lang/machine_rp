# machine_rp

RP microcontroller port of a MicroPython-shaped **`machine`** API for [Klin](https://github.com/klin-lang/klin).

Not a MicroPython port. No GC, no hidden heap, no hidden clocks.

Decision / catalog: [Klin issue 061](https://github.com/klin-lang/klin/blob/main/issues/061-micropython-machine-api.md), targets [062](https://github.com/klin-lang/klin/blob/main/issues/062-targets-esp-rp.md).

## Status

| Chip | Pin API | Example | Toolchain |
|---|---|---|---|
| **RP2040** | `pin_out` / `pin_in` | `examples/blink_pico` | `arm-none-eabi-gcc` (M0+) |
| **RP2350 Arm** | `pin_out_rp2350` / `pin_in_rp2350` | `examples/blink_pico2` | `arm-none-eabi-gcc` (M33) |
| **RP2350 RISC-V** | same `pin_out_rp2350` | `examples/blink_pico2_riscv` | `riscv64-unknown-elf-gcc` `-march=rv32imac` |

RP2040 vs RP2350 use **different** MMIO maps. RP2350 Arm and RISC-V share the same peripheral map — only boot (IMAGE_DEF CPU flag) and the compiler differ.

## Requirements

- [Klin](https://github.com/klin-lang/klin) compiler
- ARM examples: `arm-none-eabi-gcc`
- RISC-V example: `gcc-riscv64-unknown-elf` (RV32 via `-march=rv32imac -mabi=ilp32`)

## Usage

```klin
import "github/klin-lang/machine_rp" machine

let a = machine.pin_out(25)           // RP2040
let b = machine.pin_out_rp2350(25)    // RP2350 (Arm or RISC-V)
```

```sh
klin get github/klin-lang/machine_rp@v0.3.0
```

## Examples

```sh
cd examples/blink_pico         # RP2040
cd examples/blink_pico2        # RP2350 Arm + IMAGE_DEF
cd examples/blink_pico2_riscv  # RP2350 RISC-V + IMAGE_DEF
make KLIN=/path/to/klin/bin/klin.dart
```

Pico 2 **W**: onboard LED is not a plain GPIO — use an external LED or a non-W Pico 2 for GPIO25.

## Tests

```sh
dart run /path/to/klin/bin/klin.dart test machine_rp/
```

## License

MIT (Klin package).  
`examples/blink_pico/boot2_w25q080.S`: BSD-3-Clause (Raspberry Pi / rp-rs).  
IMAGE_DEF sources: minimum block per RP2350 datasheet §5.9.5.
