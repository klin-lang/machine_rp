/*
 * MIT License
 *
 * Copyright (c) 2026 Klin contributors
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * Freestanding USB CDC ACM device driver for RP2350 USBCTRL (also conceptual
 * for RP2040). Polling only — call klin_usb_cdc_poll() from the application
 * loop. Caller must enable 48 MHz clk_usb before klin_usb_cdc_init().
 *
 * Inspired by pico-examples / public RP2040–RP2350 USB documentation; original
 * for Klin (no pico-sdk dependency).
 */

#ifndef KLIN_USB_CDC_RP_H
#define KLIN_USB_CDC_RP_H

#ifdef __cplusplus
extern "C" {
#endif

/* Reset USBCTRL, clear DPRAM, device mode, pull-up. Assumes clk_usb @ 48 MHz. */
void klin_usb_cdc_init(void);

/* Handle bus reset, setup packets, and buffer completions. Call in a loop. */
void klin_usb_cdc_poll(void);

/* 1 after SET_CONFIGURATION, else 0. */
int klin_usb_cdc_configured(void);

/* Queue one byte for host. Returns 1 if queued, 0 if TX ring full / not configured. */
int klin_usb_cdc_write_u8(int b);

/* Pop one RX byte: 0..255, or -1 if empty. */
int klin_usb_cdc_try_read_u8(void);

/* 1 if RX ring has data. */
int klin_usb_cdc_any(void);

/* Pull-up off, controller off. */
void klin_usb_cdc_deinit(void);

#ifdef __cplusplus
}
#endif

#endif /* KLIN_USB_CDC_RP_H */
