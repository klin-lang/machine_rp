# machine_rp

RP microcontroller port of a MicroPython-shaped **`machine`** API for [Klin](https://github.com/MrHIDEn/klin).

Not a MicroPython port. No GC, no hidden heap, no hidden clocks.

Decision / catalog: [Klin issue 061](https://github.com/MrHIDEn/klin/blob/main/issues/061-micropython-machine-api.md), targets [062](https://github.com/MrHIDEn/klin/blob/main/issues/062-targets-esp-rp.md).

## Status

| Chip | Status |
|---|---|
| **RP2040** | MVP `Pin` + Pico blink (GPIO 25) |
| **RP2350** | later (same repo, separate example) |

| API | Status |
|---|---|
| `Pin` (`pin_out` / `pin_in`, `high` / `low` / `toggle` / `set` / `value`) | MVP (RP2040 SIO) |
| `Pwm`, `Uart`, … | later |

## Requirements

- [Klin](https://github.com/MrHIDEn/klin) compiler
- `arm-none-eabi-gcc` (Cortex-M0+)
- Flash: `picotool` / UF2 (example builds `blink.elf`; convert as you prefer)

## Layout

```text
machine_rp/              # module machine_rp
  version.kl
  pin.kl
  pin_test.kl
examples/blink_pico/     # RP2040 Pico LED GPIO25 + boot2
```

## Usage

```klin
import "github/mrhiden/machine_rp" machine

@[link("startup.s")]
@[link("boot2_w25q080.S")]
fn main() {
    let led = machine.pin_out(25)
    while true {
        led.toggle()
    }
}
```

```sh
klin get github/mrhiden/machine_rp@main
```

## Blink example

```sh
cd examples/blink_pico
make KLIN=/path/to/klin/bin/klin.dart
# → blink.elf  (flash with picotool / elf2uf2)
```

Vendored `boot2_w25q080.S` is the CRC-padded RP2040 second-stage boot for W25Q080 (Pico flash), from [rp-rs/rp2040-boot2](https://github.com/rp-rs/rp2040-boot2).

## Tests

```sh
dart run /path/to/klin/bin/klin.dart test machine_rp/
```

## License

MIT (Klin package). Boot2 blob: BSD-3-Clause (Raspberry Pi / rp-rs).
