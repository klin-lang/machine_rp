CC := arm-none-eabi-gcc
CFLAGS := -mcpu=cortex-m0plus -mthumb -ffreestanding -Os -ffunction-sections -fdata-sections
LDFLAGS := -T linker.ld -Wl,--gc-sections -nostdlib

# Path to Klin entry (bin/klin.dart) or a `klin` binary on PATH.
KLIN ?= klin

.PHONY: all clean

all: pio_blink.elf

LINK_OBJS := $(shell test -f out/pio_blink.link && cat out/pio_blink.link)

out/pio_blink.c out/pio_blink.link: pio_blink.kl $(wildcard ../../machine_rp/*.kl)
	@case "$(KLIN)" in \
	  *.dart) dart run "$(KLIN)" --emit-c pio_blink.kl ;; \
	  *) "$(KLIN)" --emit-c pio_blink.kl ;; \
	esac

pio_blink.elf: linker.ld out/pio_blink.c out/pio_blink.link $(LINK_OBJS)
	$(CC) $(CFLAGS) -I. $(LDFLAGS) $$(cat out/pio_blink.link) out/pio_blink.c -lgcc -o $@

clean:
	rm -rf out pio_blink.elf
