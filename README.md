# machine_rp

RP microcontroller port of a MicroPython-shaped **`machine`** API for [Klin](https://github.com/klin-lang/klin).

Not a MicroPython port. No GC, no hidden heap, no hidden clocks.

Decision / catalog: [Klin issue 061](https://github.com/klin-lang/klin/blob/main/issues/061-micropython-machine-api.md), targets [062](https://github.com/klin-lang/klin/blob/main/issues/062-targets-esp-rp.md).

## Status

| Chip | Pin | Pwm | Rc | Example | Toolchain |
|---|---|---|---|---|---|
| **RP2040** | `pin_out` / `pin_in` | `pwm_out` | `rc_out` | `blink_pico`, `pwm_pico`, `rc_pico` | `arm-none-eabi-gcc` (M0+) |
| **RP2350 Arm** | `pin_out_rp2350` / `pin_in_rp2350` | `pwm_out_rp2350` | `rc_out_rp2350` | `blink_pico2` | `arm-none-eabi-gcc` (M33) |
| **RP2350 RISC-V** | same `pin_out_rp2350` | same `pwm_out_rp2350` | same `rc_out_rp2350` | `blink_pico2_riscv` | `riscv64-unknown-elf-gcc` `-march=rv32imac` |

RP2040 vs RP2350 use **different** MMIO maps. RP2350 Arm and RISC-V share the same peripheral map — only boot (IMAGE_DEF CPU flag) and the compiler differ.

`version()` → `5` (`@v0.5.0`).

## Requirements

- [Klin](https://github.com/klin-lang/klin) compiler
- ARM examples: `arm-none-eabi-gcc`
- RISC-V example: `gcc-riscv64-unknown-elf` (RV32 via `-march=rv32imac -mabi=ilp32`)

## Usage — Pin

```klin
import "github/klin-lang/machine_rp" machine

let a = machine.pin_out(25)           // RP2040
let b = machine.pin_out_rp2350(25)    // RP2350 (Arm or RISC-V)
```

## Usage — Pwm

Same shape as [`machine_stm32`](https://github.com/klin-lang/machine_stm32): `freq` / `duty_u16` / `deinit`.
Slice and A/B channel are derived from the GPIO number (explicit `sys_clk_hz` — no clock-tree read).

```klin
import "github/klin-lang/machine_rp" machine

let led = machine.pwm_out(25, 6000000)
led.freq(1000)
led.duty_u16(32768)
```

RP2350:

```klin
let led = machine.pwm_out_rp2350(25, 150000000)
led.freq(1000)
led.duty_u16(32768)
```

## Usage — Rc (servo / RC pulse)

```klin
import "github/klin-lang/machine_rp" machine

let servo = machine.rc_out(25, 6000000, 50, 1000, 2000)
servo.out(50000, 0)
servo.out_f32(0.25, 0)
servo.pulse_us(1500)
// RP2350: machine.rc_out_rp2350(25, 150000000, 50, 1000, 2000)
```

```sh
klin get github/klin-lang/machine_rp@v0.5.0
```

## Pwm / Rc shape (shared with other `machine_*`)

| Piece | Role |
|---|---|
| `pwm_out` / `pwm_out_rp2350` | factory — chip map differs |
| `freq` / `duty_u16` / `deinit` | PWM |
| `rc_out` / `rc_out_rp2350` | servo/RC on same HW args + `freq_hz, us_min, us_max` |
| `out` / `out_f32` / `pulse_us` / `deinit` | position + trim / raw µs |

## Examples

```sh
cd examples/blink_pico         # RP2040 Pin
cd examples/pwm_pico           # RP2040 Pwm fade on GPIO 25
cd examples/rc_pico            # RP2040 Rc servo sweep on GPIO 25
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
`examples/blink_pico/boot2_w25q080.S` / `examples/pwm_pico/boot2_w25q080.S` / `examples/rc_pico/boot2_w25q080.S`: BSD-3-Clause (Raspberry Pi / rp-rs).  
IMAGE_DEF sources: minimum block per RP2350 datasheet §5.9.5.
