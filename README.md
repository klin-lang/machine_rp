# machine_rp

RP microcontroller port of a MicroPython-shaped **`machine`** API for [Klin](https://github.com/klin-lang/klin).

Not a MicroPython port. No GC, no hidden heap, no hidden clocks.

Decision / catalog: [Klin issue 061](https://github.com/klin-lang/klin/blob/main/issues/061-micropython-machine-api.md), targets [062](https://github.com/klin-lang/klin/blob/main/issues/062-targets-esp-rp.md).

## Status

| Chip | Pin | Pwm | Rc | Uart | I2c | Spi | Adc | Pio | Dac |
|---|---|---|---|---|---|---|---|---|---|
| **RP2040** | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | — (no HW) |
| **RP2350** | ✅ `*_rp2350` | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ `*_rp2350` (+ PIO2) | — (no HW) |

Examples: `blink_pico`, `pwm_pico`, `rc_pico`, `uart_pico`, `i2c_pico`, `spi_pico`, `adc_pico`, `pio_blink_pico` (+ Pico 2 blink).

`version()` → `7` (`@v0.7.0`).

**No hardware DAC** on RP2040 / RP2350 — do not expect `dac_out`.

**PIO** is RP silicon only (not on other `machine_*` ports).

## Requirements

- [Klin](https://github.com/klin-lang/klin) compiler
- ARM examples: `arm-none-eabi-gcc`
- RISC-V example: `gcc-riscv64-unknown-elf` (RV32 via `-march=rv32imac -mabi=ilp32`)

## Usage — Pin / Pwm / Rc

```klin
import "github/klin-lang/machine_rp" machine

let led = machine.pin_out(25)
let pwm = machine.pwm_out(25, 6000000)
let servo = machine.rc_out(25, 6000000, 50, 1000, 2000)
// RP2350: pin_out_rp2350 / pwm_out_rp2350 / rc_out_rp2350
```

## Usage — Uart

PL011. `peri_clk_hz` = clk_peri (explicit). FUNCSEL 2.

```klin
let u = machine.uart_out(0, 0, 1, 125000000, 115200)  // UART0 GP0/GP1
u.write_u8(65)
// RP2350: machine.uart_out_rp2350(...)
```

## Usage — I2c

DW_apb_i2c. FUNCSEL 3. 7-bit addresses. Soft pull-ups on pads.

```klin
let bus = machine.i2c_out(0, 4, 5, 125000000, 100000)  // I2C0 GP4/GP5
bus.writeto(0x50, wbuf)
bus.readfrom_into(0x50, rbuf)
```

## Usage — Spi

PL022 master, soft NSS (CS = separate `Pin`). FUNCSEL 1.

```klin
let s = machine.spi_out(0, 18, 19, 16, 125000000, 1000000, 0)
let v = s.write_read_u8(0x9F)
```

## Usage — Adc

Channels 0..=3 → GP26..=29; ch4 = temperature sensor.
Board/startup should enable **clk_adc** (typically 48 MHz) — not hidden here.

```klin
let adc = machine.adc_out(26, 0)
let raw = adc.read_u12()
let u16 = adc.read_u16()
```

## Usage — Pio

Raw instruction words (`[]i32`). Helpers: `pio_encode_set_pins` / `set_pindirs` / `nop` / `jmp`.
RP2040: PIO0/1. RP2350: PIO0/1/2 via `pio_out_rp2350`.

```klin
let sm = machine.pio_out(0, 0)
sm.gpio_init(25)
sm.config_set_pins(25, 1)
let mut prog: [2]i32
prog[0] = machine.pio_encode_set_pindirs(1, 0)
prog[1] = machine.pio_encode_set_pins(1, 0)
sm.load(prog[:], 0)
sm.config_wrap(1, 1)
sm.config_clkdiv(1)
sm.active(1)
sm.put_u32(0)  // TX FIFO (when program PULLs)
```

```sh
klin get github/klin-lang/machine_rp@v0.7.0
```

## Shape (shared with other `machine_*`)

| Piece | Role |
|---|---|
| `*_out` / `*_out_rp2350` | factory — RP2040 vs RP2350 maps |
| `write_u8` / `write` / `read_u8` / `try_read_u8` / `any` | UART |
| `writeto` / `readfrom_into` / `write_readfrom_into` | I2C |
| `write_read_u8` / `write` / `readinto` / `write_readinto` | SPI |
| `read_u12` / `read_u16` | ADC |
| `load` / `config_*` / `active` / `put_u32` / `get_u32` / `exec` | PIO |
| `deinit` | stop peripheral (explicit) |

## Examples

```sh
cd examples/uart_pico   # or i2c_pico / spi_pico / adc_pico / pio_blink_pico / …
make KLIN=/path/to/klin/bin/klin.dart
```

## Tests

```sh
dart run /path/to/klin/bin/klin.dart test machine_rp/
```

## License

MIT
