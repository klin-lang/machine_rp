# machine_rp

RP microcontroller port of a MicroPython-shaped **`machine`** API for [Klin](https://github.com/klin-lang/klin).

Not a MicroPython port. No GC, no hidden heap, no hidden clocks.

Decision / catalog: [Klin issue 061](https://github.com/klin-lang/klin/blob/main/issues/061-micropython-machine-api.md), targets [062](https://github.com/klin-lang/klin/blob/main/issues/062-targets-esp-rp.md).

## Status

| Chip | Pin | Pwm | Rc | Uart | I2c | Spi | Adc | Pio | Dma | UsbCdc | Dac |
|---|---|---|---|---|---|---|---|---|---|---|
| **RP2040** | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | later | — (no HW) |
| **RP2350** | ✅ `*_rp2350` | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ `*_rp2350` (+ PIO2) | ✅ `*_rp2350` | ✅ CDC poll | — (no HW) |

Examples: `blink_pico`, `pwm_pico`, `rc_pico`, `uart_pico`, `i2c_pico`, `spi_pico`, `adc_pico`, `pio_blink_pico` (+ Pico 2 blink).

`version()` → `10` (`@v0.10.0`).

**No hardware DAC** on RP2040 / RP2350 — do not expect `dac_out`.

**PIO** and **DMA** are RP silicon only (not on other `machine_*` ports).

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
`SSPDMACR.TXDMAE` is enabled at factory; use `write_dma` / `write_dma_repeat2` with a claimed `Dma`.

```klin
let s = machine.spi_out(0, 18, 19, 16, 125000000, 1000000, 0)
let v = s.write_read_u8(0x9F)
let d = machine.dma_out(0)
s.write_dma(d, buf, machine.dma_dreq_spi0_tx())
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

Raw instruction words (`[]i32`). Helpers: `pio_encode_set_*` / `nop` / `jmp` / `jmp_not_x` / `out_x` /
`delay` / `sideset`. Config: wrap, clkdiv(+frac), set/out/sideset pins, out_shift, fifo_join_tx.
RP2040: PIO0/1. RP2350: PIO0/1/2 via `pio_out_rp2350`.

```klin
let sm = machine.pio_out(0, 0)
sm.gpio_init(25)
sm.config_sideset_pins(25, 1)
sm.config_out_shift(0, 1, 24)  // shift left, autopull 24
sm.config_fifo_join_tx()
sm.config_clkdiv_frac(
    machine.pio_clkdiv_int(12000000, 8000000),
    machine.pio_clkdiv_frac(12000000, 8000000)
)
sm.active(1)
sm.put_u32(grb << 8)
```

## Usage — Dma

Claim a channel (`dma_out` / `dma_out_rp2350`). Byte TX to a peripheral FIFO:
`start_to_peri` + `wait`, or `Spi.write_dma` / `write_dma_repeat2` (2-byte ring for solid fills).
DREQ helpers: `dma_dreq_spi*_tx` / `*_rp2350`. No IRQ, no heap, no hidden clocks.

```klin
let d = machine.dma_out_rp2350(0)
s.write_dma_repeat2(d, lo, hi, nbytes, machine.dma_dreq_spi1_tx_rp2350())
```

```sh
klin get github/klin-lang/machine_rp@v0.10.0
```


## Usage — UsbCdc (RP2350)

Polling CDC ACM device. **Caller enables clk_usb @ 48 MHz** (board `enable_usb_clocks`).
C driver: `usb_cdc_rp.c` (linked via `@[link]`).

```klin
let u = machine.usb_cdc_out_rp2350()
while true {
    u.poll()
    if u.configured() {
        let b = u.try_read_u8()
        if b >= 0 { let _ = u.write_u8(b) }
    }
}
```

## Shape (shared with other `machine_*`)

| Piece | Role |
|---|---|
| `*_out` / `*_out_rp2350` | factory — RP2040 vs RP2350 maps |
| `write_u8` / `write` / `read_u8` / `try_read_u8` / `any` | UART |
| `writeto` / `readfrom_into` / `write_readfrom_into` | I2C |
| `write_read_u8` / `write` / `readinto` / `write_readinto` / `write_dma*` | SPI |
| `read_u12` / `read_u16` | ADC |
| `load` / `config_*` / `active` / `put_u32` / `get_u32` / `exec` | PIO |
| `start_to_peri` / `wait` / `busy` | DMA |
| `poll` / `configured` / `write_u8` / `try_read_u8` | UsbCdc |
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
