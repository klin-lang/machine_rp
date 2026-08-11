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
 * Freestanding USB CDC ACM device driver for RP2350 USBCTRL (polling only).
 * Does not touch XOSC/PLL/clk_usb — caller must enable 48 MHz clk_usb first.
 *
 * Inspired by pico-examples / public USB docs; original for Klin.
 */

#include "usb_cdc_rp.h"

/* -------------------------------------------------------------------------- */
/* MMIO                                                                       */
/* -------------------------------------------------------------------------- */

#define USBCTRL_DPRAM_BASE  0x50100000u
#define USBCTRL_REGS_BASE   0x50110000u
#define RESETS_BASE         0x40020000u
#define RESETS_SET_BASE     0x40022000u /* atomic set alias */
#define RESETS_CLR_BASE     0x40023000u /* atomic clear alias */
#define RESETS_RESET_DONE   (RESETS_BASE + 0x08u)
#define RESETS_USBCTRL_BITS (1u << 28)

#define USB_ADDR_ENDP   (USBCTRL_REGS_BASE + 0x00u)
#define USB_MAIN_CTRL   (USBCTRL_REGS_BASE + 0x40u)
#define USB_SIE_CTRL    (USBCTRL_REGS_BASE + 0x4cu)
#define USB_SIE_STATUS  (USBCTRL_REGS_BASE + 0x50u)
#define USB_BUFF_STATUS (USBCTRL_REGS_BASE + 0x58u)
#define USB_MUXING      (USBCTRL_REGS_BASE + 0x74u)
#define USB_PWR         (USBCTRL_REGS_BASE + 0x78u)
#define USB_INTE        (USBCTRL_REGS_BASE + 0x90u)

#define REG32(a) (*(volatile unsigned int *)(unsigned long)(a))

#define USB_MAIN_CTRL_CONTROLLER_EN   (1u << 0)
#define USB_MUXING_TO_PHY             (1u << 0)
#define USB_MUXING_SOFTCON            (1u << 3)
#define USB_PWR_VBUS_DETECT           (1u << 2)
#define USB_PWR_VBUS_DETECT_OVERRIDE  (1u << 3)
#define USB_SIE_CTRL_PULLUP_EN        (1u << 16)
#define USB_SIE_CTRL_EP0_INT_1BUF     (1u << 29)

#define USB_SIE_STATUS_SETUP_REC      (1u << 17)
#define USB_SIE_STATUS_BUS_RESET      (1u << 19)

#define USB_BUF_CTRL_LEN_MASK         0x3ffu
#define USB_BUF_CTRL_AVAIL            (1u << 10)
#define USB_BUF_CTRL_STALL            (1u << 11)
#define USB_BUF_CTRL_DATA1            (1u << 13)
#define USB_BUF_CTRL_LAST             (1u << 14)
#define USB_BUF_CTRL_FULL             (1u << 15)

#define USB_EP_CTRL_ENABLE            (1u << 31)
#define USB_EP_CTRL_INTERRUPT_PER_BUF (1u << 29)
#define USB_EP_CTRL_TYPE_BULK         (2u << 26)
#define USB_EP_CTRL_TYPE_INT          (3u << 26)

/* DPRAM layout (device) */
#define DPRAM_SETUP           (USBCTRL_DPRAM_BASE + 0x00u)
#define DPRAM_EP_CTRL(n, in)  (USBCTRL_DPRAM_BASE + 0x08u + ((unsigned)((n) - 1) * 8u) + ((in) ? 0u : 4u))
#define DPRAM_EP_BUF_CTRL(n, in) \
  (USBCTRL_DPRAM_BASE + 0x80u + ((unsigned)(n) * 8u) + ((in) ? 0u : 4u))
#define DPRAM_EP0_BUF_A       (USBCTRL_DPRAM_BASE + 0x100u)
#define DPRAM_EPX_DATA        (USBCTRL_DPRAM_BASE + 0x180u)

/* Endpoint buffer offsets from DPRAM base (low bits of EP_CTRL) */
#define EP1_IN_BUF_OFF   0x180u
#define EP2_IN_BUF_OFF   0x1c0u
#define EP3_OUT_BUF_OFF  0x200u

#define EP1_IN_BUF  (USBCTRL_DPRAM_BASE + EP1_IN_BUF_OFF)
#define EP2_IN_BUF  (USBCTRL_DPRAM_BASE + EP2_IN_BUF_OFF)
#define EP3_OUT_BUF (USBCTRL_DPRAM_BASE + EP3_OUT_BUF_OFF)

#define USB_DPRAM_SIZE  0x1000u

#define RING_SIZE 256u

/* -------------------------------------------------------------------------- */
/* State                                                                      */
/* -------------------------------------------------------------------------- */

static unsigned char s_tx_ring[RING_SIZE];
static unsigned char s_rx_ring[RING_SIZE];
static unsigned char s_tx_head;
static unsigned char s_tx_tail;
static unsigned char s_rx_head;
static unsigned char s_rx_tail;

static unsigned char s_configured;
static unsigned char s_tx_busy;
static unsigned char s_ep2_in_data1;
static unsigned char s_ep3_out_data1;
static unsigned char s_pending_addr;
static unsigned char s_apply_addr;
static unsigned char s_ep0_stall;

/* Multi-packet EP0 IN (config desc is 67 bytes > 64) */
static const unsigned char *s_ep0_in_src;
static unsigned s_ep0_in_remain;
static unsigned char s_ep0_in_data1;
static unsigned char s_ep0_in_active;

/* Line coding: 115200 8N1 (CDC GET/SET_LINE_CODING) */
static unsigned char s_line_coding[7] = {
    0x00u, 0xc2u, 0x01u, 0x00u, /* 115200 LE */
    0x00u,                      /* 1 stop bit */
    0x00u,                      /* no parity */
    0x08u                       /* 8 data bits */
};

/* -------------------------------------------------------------------------- */
/* Tiny memory helpers                                                        */
/* -------------------------------------------------------------------------- */

static void mem_clear(volatile unsigned char *p, unsigned n) {
  while (n--) {
    *p++ = 0;
  }
}

static void mem_copy(unsigned char *dst, const unsigned char *src, unsigned n) {
  while (n--) {
    *dst++ = *src++;
  }
}

static void mem_copy_v(volatile unsigned char *dst, const unsigned char *src, unsigned n) {
  while (n--) {
    *dst++ = *src++;
  }
}

static void busy_wait_short(void) {
  /* RP2040/RP2350: need a few cycles between BUF_CTRL writes before AVAIL. */
  volatile unsigned i;
  for (i = 0; i < 12u; i++) {
  }
}

/* -------------------------------------------------------------------------- */
/* Descriptors                                                                */
/* -------------------------------------------------------------------------- */

static const unsigned char s_device_desc[] = {
    18,    /* bLength */
    0x01,  /* bDescriptorType DEVICE */
    0x00, 0x02, /* bcdUSB 2.00 */
    0x02,  /* bDeviceClass CDC */
    0x00,  /* bDeviceSubClass */
    0x00,  /* bDeviceProtocol */
    64,    /* bMaxPacketSize0 */
    0x09, 0x12, /* idVendor 0x1209 */
    0xE0, 0xC1, /* idProduct 0xC1E0 */
    0x00, 0x01, /* bcdDevice 1.00 */
    1,     /* iManufacturer */
    2,     /* iProduct */
    0,     /* iSerialNumber */
    1      /* bNumConfigurations */
};

/* Config + CDC comm iface + data iface: total length 67 */
static const unsigned char s_config_desc[] = {
    /* Configuration */
    9, 0x02, 67, 0, 2, 1, 0, 0x80, 50,
    /* Interface 0: Communication (CDC ACM) */
    9, 0x04, 0, 0, 1, 0x02, 0x02, 0x01, 0,
    /* Header Functional */
    5, 0x24, 0x00, 0x10, 0x01,
    /* Call Management */
    5, 0x24, 0x01, 0x00, 1,
    /* ACM Functional */
    4, 0x24, 0x02, 0x02,
    /* Union Functional */
    5, 0x24, 0x06, 0, 1,
    /* EP1 IN interrupt (notify) */
    7, 0x05, 0x81, 0x03, 8, 0, 16,
    /* Interface 1: Data */
    9, 0x04, 1, 0, 2, 0x0A, 0x00, 0x00, 0,
    /* EP2 IN bulk */
    7, 0x05, 0x82, 0x02, 64, 0, 0,
    /* EP3 OUT bulk */
    7, 0x05, 0x03, 0x02, 64, 0, 0,
};

static const unsigned char s_string0[] = {
    4, 0x03, 0x09, 0x04 /* English (US) */
};

/* "Klin" UTF-16LE */
static const unsigned char s_string1[] = {
    10, 0x03, 'K', 0, 'l', 0, 'i', 0, 'n', 0
};

/* "USB CDC" UTF-16LE */
static const unsigned char s_string2[] = {
    16, 0x03,
    'U', 0, 'S', 0, 'B', 0, ' ', 0,
    'C', 0, 'D', 0, 'C', 0
};

/* -------------------------------------------------------------------------- */
/* Buffer control                                                             */
/* -------------------------------------------------------------------------- */

static void buf_ctrl_arm(volatile unsigned int *reg, unsigned val_with_avail) {
  *reg = val_with_avail & ~USB_BUF_CTRL_AVAIL;
  busy_wait_short();
  *reg = val_with_avail;
}

static void ep0_in_arm(unsigned len, unsigned data1, unsigned full) {
  unsigned v = (len & USB_BUF_CTRL_LEN_MASK) | USB_BUF_CTRL_AVAIL | USB_BUF_CTRL_LAST;
  if (data1) {
    v |= USB_BUF_CTRL_DATA1;
  }
  if (full) {
    v |= USB_BUF_CTRL_FULL;
  }
  buf_ctrl_arm((volatile unsigned int *)DPRAM_EP_BUF_CTRL(0, 1), v);
}

static void ep0_out_arm(unsigned len, unsigned data1) {
  unsigned v = (len & USB_BUF_CTRL_LEN_MASK) | USB_BUF_CTRL_AVAIL | USB_BUF_CTRL_LAST;
  if (data1) {
    v |= USB_BUF_CTRL_DATA1;
  }
  /* OUT: FULL clear means buffer empty / ready to receive */
  buf_ctrl_arm((volatile unsigned int *)DPRAM_EP_BUF_CTRL(0, 0), v);
}

static void ep0_in_zlp(void) {
  ep0_in_arm(0, 1, 1);
}

static void ep0_stall(void) {
  s_ep0_stall = 1;
  REG32(DPRAM_EP_BUF_CTRL(0, 1)) = USB_BUF_CTRL_STALL;
  REG32(DPRAM_EP_BUF_CTRL(0, 0)) = USB_BUF_CTRL_STALL;
}

/* -------------------------------------------------------------------------- */
/* Non-control endpoints                                                      */
/* -------------------------------------------------------------------------- */

static void configure_data_eps(void) {
  /* EP1 IN interrupt — notify; traffic ignored */
  REG32(DPRAM_EP_CTRL(1, 1)) =
      USB_EP_CTRL_ENABLE | USB_EP_CTRL_INTERRUPT_PER_BUF | USB_EP_CTRL_TYPE_INT | EP1_IN_BUF_OFF;

  /* EP2 IN bulk — device → host */
  REG32(DPRAM_EP_CTRL(2, 1)) =
      USB_EP_CTRL_ENABLE | USB_EP_CTRL_INTERRUPT_PER_BUF | USB_EP_CTRL_TYPE_BULK | EP2_IN_BUF_OFF;

  /* EP3 OUT bulk — host → device */
  REG32(DPRAM_EP_CTRL(3, 0)) =
      USB_EP_CTRL_ENABLE | USB_EP_CTRL_INTERRUPT_PER_BUF | USB_EP_CTRL_TYPE_BULK | EP3_OUT_BUF_OFF;

  s_ep2_in_data1 = 0;
  s_ep3_out_data1 = 0;
  s_tx_busy = 0;
}

static void arm_ep3_out(void) {
  unsigned v = 64u | USB_BUF_CTRL_AVAIL | USB_BUF_CTRL_LAST;
  if (s_ep3_out_data1) {
    v |= USB_BUF_CTRL_DATA1;
  }
  buf_ctrl_arm((volatile unsigned int *)DPRAM_EP_BUF_CTRL(3, 0), v);
}

static void start_tx_if_needed(void) {
  unsigned n;
  unsigned v;
  unsigned char *dst;

  if (!s_configured || s_tx_busy) {
    return;
  }
  if (s_tx_tail == s_tx_head) {
    return;
  }

  dst = (unsigned char *)EP2_IN_BUF;
  n = 0;
  while (s_tx_tail != s_tx_head && n < 64u) {
    dst[n++] = s_tx_ring[s_tx_tail];
    s_tx_tail = (unsigned char)(s_tx_tail + 1u);
  }

  v = (n & USB_BUF_CTRL_LEN_MASK) | USB_BUF_CTRL_AVAIL | USB_BUF_CTRL_FULL | USB_BUF_CTRL_LAST;
  if (s_ep2_in_data1) {
    v |= USB_BUF_CTRL_DATA1;
  }
  s_ep2_in_data1 ^= 1u;
  s_tx_busy = 1;
  buf_ctrl_arm((volatile unsigned int *)DPRAM_EP_BUF_CTRL(2, 1), v);
}

/* -------------------------------------------------------------------------- */
/* Control transfer helpers                                                   */
/* -------------------------------------------------------------------------- */

static void ep0_in_continue(void) {
  unsigned n = s_ep0_in_remain;
  if (n > 64u) {
    n = 64u;
  }
  mem_copy_v((volatile unsigned char *)(unsigned long)DPRAM_EP0_BUF_A, s_ep0_in_src, n);
  ep0_in_arm(n, s_ep0_in_data1, 1);
  s_ep0_in_src += n;
  s_ep0_in_remain -= n;
  s_ep0_in_data1 ^= 1u;
  if (s_ep0_in_remain == 0u) {
    /* Status stage: host OUT ZLP (arm early; OK on RP USBCTRL) */
    ep0_out_arm(0, 1);
    s_ep0_in_active = 0;
  }
}

static void ep0_send_data(const unsigned char *data, unsigned len, unsigned wlength) {
  unsigned n = len;
  if (n > wlength) {
    n = wlength;
  }
  s_ep0_in_src = data;
  s_ep0_in_remain = n;
  s_ep0_in_data1 = 1;
  s_ep0_in_active = 1;
  ep0_in_continue(); /* clears s_ep0_in_active when the last packet is queued */
}

static void handle_get_descriptor(unsigned wvalue, unsigned wlength) {
  unsigned char dtype = (unsigned char)(wvalue >> 8);
  unsigned char dindex = (unsigned char)(wvalue & 0xffu);

  if (dtype == 0x01u && dindex == 0u) {
    ep0_send_data(s_device_desc, sizeof s_device_desc, wlength);
    return;
  }
  if (dtype == 0x02u && dindex == 0u) {
    ep0_send_data(s_config_desc, sizeof s_config_desc, wlength);
    return;
  }
  if (dtype == 0x03u) {
    if (dindex == 0u) {
      ep0_send_data(s_string0, sizeof s_string0, wlength);
      return;
    }
    if (dindex == 1u) {
      ep0_send_data(s_string1, sizeof s_string1, wlength);
      return;
    }
    if (dindex == 2u) {
      ep0_send_data(s_string2, sizeof s_string2, wlength);
      return;
    }
  }
  ep0_stall();
}

static void clear_ep_state_on_reset(void) {
  unsigned i;

  REG32(USB_ADDR_ENDP) = 0;
  s_configured = 0;
  s_tx_busy = 0;
  s_pending_addr = 0;
  s_apply_addr = 0;
  s_ep0_stall = 0;
  s_ep0_in_active = 0;
  s_ep0_in_remain = 0;
  s_ep2_in_data1 = 0;
  s_ep3_out_data1 = 0;
  s_tx_head = s_tx_tail = 0;
  s_rx_head = s_rx_tail = 0;

  for (i = 0; i < 16u; i++) {
    REG32(DPRAM_EP_BUF_CTRL(i, 1)) = 0;
    REG32(DPRAM_EP_BUF_CTRL(i, 0)) = 0;
  }
  for (i = 1; i < 16u; i++) {
    REG32(DPRAM_EP_CTRL(i, 1)) = 0;
    REG32(DPRAM_EP_CTRL(i, 0)) = 0;
  }
}

static void handle_setup(void) {
  unsigned char setup[8];
  unsigned char bmRequestType;
  unsigned char bRequest;
  unsigned wValue;
  unsigned wIndex;
  unsigned wLength;
  unsigned char *p;
  unsigned i;

  p = (unsigned char *)DPRAM_SETUP;
  for (i = 0; i < 8u; i++) {
    setup[i] = p[i];
  }

  /* Clear SETUP_REC (W1C) */
  REG32(USB_SIE_STATUS) = USB_SIE_STATUS_SETUP_REC;

  /* Clear any prior EP0 buf status */
  REG32(USB_BUFF_STATUS) = (1u << 0) | (1u << 1);

  s_ep0_stall = 0;
  s_ep0_in_active = 0;
  s_ep0_in_remain = 0;
  REG32(DPRAM_EP_BUF_CTRL(0, 1)) = 0;
  REG32(DPRAM_EP_BUF_CTRL(0, 0)) = 0;

  bmRequestType = setup[0];
  bRequest = setup[1];
  wValue = (unsigned)setup[2] | ((unsigned)setup[3] << 8);
  wIndex = (unsigned)setup[4] | ((unsigned)setup[5] << 8);
  wLength = (unsigned)setup[6] | ((unsigned)setup[7] << 8);

  /* Standard device/interface/endpoint requests */
  if ((bmRequestType & 0x60u) == 0x00u) {
    switch (bRequest) {
      case 0x05: /* SET_ADDRESS */
        s_pending_addr = (unsigned char)(wValue & 0x7fu);
        s_apply_addr = 1;
        ep0_in_zlp();
        return;

      case 0x06: /* GET_DESCRIPTOR */
        if ((bmRequestType & 0x80u) != 0u) {
          handle_get_descriptor(wValue, wLength);
          return;
        }
        break;

      case 0x09: /* SET_CONFIGURATION */
        if ((bmRequestType & 0x80u) == 0u) {
          if ((wValue & 0xffu) == 1u) {
            configure_data_eps();
            s_configured = 1;
            arm_ep3_out();
          } else if ((wValue & 0xffu) == 0u) {
            s_configured = 0;
            s_tx_busy = 0;
          }
          ep0_in_zlp();
          return;
        }
        break;

      case 0x08: /* GET_CONFIGURATION */
        if ((bmRequestType & 0x80u) != 0u) {
          unsigned char cfg = s_configured ? 1u : 0u;
          ep0_send_data(&cfg, 1, wLength);
          return;
        }
        break;

      case 0x00: /* GET_STATUS */
        if ((bmRequestType & 0x80u) != 0u) {
          unsigned char st[2] = {0, 0};
          ep0_send_data(st, 2, wLength);
          return;
        }
        break;

      case 0x0a: /* GET_INTERFACE */
        if ((bmRequestType & 0x80u) != 0u) {
          unsigned char alt = 0;
          ep0_send_data(&alt, 1, wLength);
          return;
        }
        break;

      case 0x0b: /* SET_INTERFACE */
        if ((bmRequestType & 0x80u) == 0u) {
          ep0_in_zlp();
          return;
        }
        break;

      case 0x01: /* CLEAR_FEATURE */
      case 0x03: /* SET_FEATURE */
        ep0_in_zlp();
        return;

      default:
        break;
    }
  }

  /* CDC class requests (interface) */
  if ((bmRequestType & 0x60u) == 0x20u) {
    switch (bRequest) {
      case 0x20: /* SET_LINE_CODING — 7-byte OUT data stage */
        if ((bmRequestType & 0x80u) == 0u && wLength >= 7u) {
          ep0_out_arm(7, 1);
          return;
        }
        if ((bmRequestType & 0x80u) == 0u) {
          ep0_in_zlp();
          return;
        }
        break;

      case 0x21: /* GET_LINE_CODING */
        if ((bmRequestType & 0x80u) != 0u) {
          ep0_send_data(s_line_coding, 7, wLength);
          return;
        }
        break;

      case 0x22: /* SET_CONTROL_LINE_STATE */
        if ((bmRequestType & 0x80u) == 0u) {
          (void)wValue;
          (void)wIndex;
          ep0_in_zlp();
          return;
        }
        break;

      default:
        /* Ignore other CDC requests with ZLP status when host→device */
        if ((bmRequestType & 0x80u) == 0u) {
          ep0_in_zlp();
          return;
        }
        break;
    }
  }

  (void)wIndex;
  ep0_stall();
}

static void handle_ep0_out_complete(void) {
  unsigned ctrl = REG32(DPRAM_EP_BUF_CTRL(0, 0));
  unsigned len = ctrl & USB_BUF_CTRL_LEN_MASK;
  unsigned char *buf = (unsigned char *)DPRAM_EP0_BUF_A;

  /* Likely SET_LINE_CODING data stage */
  if (len >= 7u && !s_ep0_stall) {
    mem_copy(s_line_coding, buf, 7);
    ep0_in_zlp();
    return;
  }

  /* Status OUT complete (after IN data) — nothing to do */
  (void)buf;
}

static void handle_ep0_in_complete(void) {
  if (s_ep0_in_active && s_ep0_in_remain > 0u) {
    ep0_in_continue();
    return;
  }
  if (s_apply_addr) {
    REG32(USB_ADDR_ENDP) = (unsigned)s_pending_addr;
    s_apply_addr = 0;
  }
}

static void handle_ep2_in_complete(void) {
  s_tx_busy = 0;
  start_tx_if_needed();
}

static void handle_ep3_out_complete(void) {
  unsigned ctrl = REG32(DPRAM_EP_BUF_CTRL(3, 0));
  unsigned len = ctrl & USB_BUF_CTRL_LEN_MASK;
  unsigned char *buf = (unsigned char *)EP3_OUT_BUF;
  unsigned i;
  unsigned char next;

  for (i = 0; i < len; i++) {
    next = (unsigned char)(s_rx_head + 1u);
    if (next == s_rx_tail) {
      break; /* ring full — drop remainder */
    }
    s_rx_ring[s_rx_head] = buf[i];
    s_rx_head = next;
  }

  s_ep3_out_data1 ^= 1u;
  arm_ep3_out();
}

static void handle_buff_status(void) {
  unsigned st = REG32(USB_BUFF_STATUS);
  if (st == 0u) {
    return;
  }

  /* Bit layout: EP0_IN=0, EP0_OUT=1, EP1_IN=2, EP1_OUT=3, EP2_IN=4, ... */
  if (st & (1u << 0)) {
    REG32(USB_BUFF_STATUS) = (1u << 0);
    handle_ep0_in_complete();
  }
  if (st & (1u << 1)) {
    REG32(USB_BUFF_STATUS) = (1u << 1);
    handle_ep0_out_complete();
  }
  if (st & (1u << 2)) {
    /* EP1 IN notify — ignore */
    REG32(USB_BUFF_STATUS) = (1u << 2);
  }
  if (st & (1u << 4)) {
    REG32(USB_BUFF_STATUS) = (1u << 4);
    handle_ep2_in_complete();
  }
  /* EP3 OUT = bit 7 (EP2_OUT=5, EP3_IN=6, EP3_OUT=7) */
  if (st & (1u << 7)) {
    REG32(USB_BUFF_STATUS) = (1u << 7);
    handle_ep3_out_complete();
  }

  /* Clear any other unexpected bits */
  st = REG32(USB_BUFF_STATUS);
  if (st) {
    REG32(USB_BUFF_STATUS) = st;
  }
}

/* -------------------------------------------------------------------------- */
/* Public API                                                                 */
/* -------------------------------------------------------------------------- */

void klin_usb_cdc_init(void) {
  volatile unsigned char *dpram = (volatile unsigned char *)USBCTRL_DPRAM_BASE;

  s_configured = 0;
  s_tx_busy = 0;
  s_tx_head = s_tx_tail = 0;
  s_rx_head = s_rx_tail = 0;
  s_pending_addr = 0;
  s_apply_addr = 0;
  s_ep0_stall = 0;
  s_ep0_in_active = 0;
  s_ep0_in_remain = 0;
  s_ep2_in_data1 = 0;
  s_ep3_out_data1 = 0;

  /* Pulse USBCTRL reset (do not touch clocks). */
  REG32(RESETS_SET_BASE) = RESETS_USBCTRL_BITS;
  REG32(RESETS_CLR_BASE) = RESETS_USBCTRL_BITS;
  while ((REG32(RESETS_RESET_DONE) & RESETS_USBCTRL_BITS) == 0u) {
  }

  mem_clear(dpram, USB_DPRAM_SIZE);

  /* No IRQ — leave INTE clear */
  REG32(USB_INTE) = 0;

  REG32(USB_MUXING) = USB_MUXING_TO_PHY | USB_MUXING_SOFTCON;
  REG32(USB_PWR) = USB_PWR_VBUS_DETECT | USB_PWR_VBUS_DETECT_OVERRIDE;
  REG32(USB_MAIN_CTRL) = USB_MAIN_CTRL_CONTROLLER_EN; /* device mode: HOST_NDEVICE=0 */

  /* EP0 buff_status per buffer + present full-speed pull-up */
  REG32(USB_SIE_CTRL) = USB_SIE_CTRL_EP0_INT_1BUF | USB_SIE_CTRL_PULLUP_EN;
}

void klin_usb_cdc_poll(void) {
  unsigned sie = REG32(USB_SIE_STATUS);

  if (sie & USB_SIE_STATUS_BUS_RESET) {
    REG32(USB_SIE_STATUS) = USB_SIE_STATUS_BUS_RESET;
    clear_ep_state_on_reset();
  }

  sie = REG32(USB_SIE_STATUS);
  if (sie & USB_SIE_STATUS_SETUP_REC) {
    handle_setup();
  }

  handle_buff_status();
  start_tx_if_needed();
}

int klin_usb_cdc_configured(void) {
  return s_configured ? 1 : 0;
}

int klin_usb_cdc_write_u8(int b) {
  unsigned char next;

  if (!s_configured) {
    return 0;
  }
  next = (unsigned char)(s_tx_head + 1u);
  if (next == s_tx_tail) {
    return 0;
  }
  s_tx_ring[s_tx_head] = (unsigned char)b;
  s_tx_head = next;
  return 1;
}

int klin_usb_cdc_try_read_u8(void) {
  int v;

  if (s_rx_tail == s_rx_head) {
    return -1;
  }
  v = (int)s_rx_ring[s_rx_tail];
  s_rx_tail = (unsigned char)(s_rx_tail + 1u);
  return v;
}

int klin_usb_cdc_any(void) {
  return (s_rx_tail != s_rx_head) ? 1 : 0;
}

void klin_usb_cdc_deinit(void) {
  REG32(USB_SIE_CTRL) = 0;
  REG32(USB_MAIN_CTRL) = 0;
  s_configured = 0;
  s_tx_busy = 0;
}
