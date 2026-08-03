# machine_rp

RP microcontroller port of a MicroPython-shaped **`machine`** API for [Klin](https://github.com/MrHIDEn/klin).

Not a MicroPython port. No GC, no hidden heap, no hidden clocks.

Decision / catalog: [Klin issue 061](https://github.com/MrHIDEn/klin/blob/main/issues/061-micropython-machine-api.md), targets [062](https://github.com/MrHIDEn/klin/blob/main/issues/062-targets-esp-rp.md).

## Status

| Chip | Pin API | Example |
|---|---|---|
| **RP2040** | `pin_out` / `pin_in` | `examples/blink_pico` (GPIO 25) |
| **RP2350** (Arm) | `pin_out_rp2350` / `pin_in_rp2350` | `examples/blink_pico2` (GPIO 25) |

| API | Status |
|---|---|
| `Pin` high/low/toggle/set/value | MVP |
| `Pwm`, `Uart`, … | later |

RP2040 and RP2350 use **different** MMIO maps (RESETS / IO_BANK0 / SIO offsets). Call the matching constructor for your chip.

## Requirements

- [Klin](https://github.com/MrHIDEn/klin) compiler
- `arm-none-eabi-gcc` (M0+ for Pico, M33 for Pico 2)
- Flash: `picotool` / UF2

## Usage

```klin
import "github/mrhiden/machine_rp" machine

// Pico (RP2040)
let led = machine.pin_out(25)

// Pico 2 (RP2350 Arm)
let led2 = machine.pin_out_rp2350(25)
```

```sh
klin get github/mrhiden/machine_rp@main
```

## Examples

```sh
cd examples/blink_pico    # RP2040 + boot2
make KLIN=/path/to/klin/bin/klin.dart

cd examples/blink_pico2   # RP2350 Arm + IMAGE_DEF
make KLIN=/path/to/klin/bin/klin.dart
```

## Tests

```sh
dart run /path/to/klin/bin/klin.dart test machine_rp/
```

## License

MIT (Klin package).  
`examples/blink_pico/boot2_w25q080.S`: BSD-3-Clause (Raspberry Pi / rp-rs).  
`examples/blink_pico2/image_def.S`: minimum IMAGE_DEF per RP2350 datasheet §5.9.5.
