CC := arm-none-eabi-gcc
CFLAGS := -mcpu=cortex-m0plus -mthumb -ffreestanding -Os -ffunction-sections -fdata-sections
LDFLAGS := -T linker.ld -Wl,--gc-sections -nostdlib

# Path to Klin entry (bin/klin.dart) or a `klin` binary on PATH.
KLIN ?= klin

.PHONY: all clean

all: pwm.elf

LINK_OBJS := $(shell test -f out/pwm.link && cat out/pwm.link)

out/pwm.c out/pwm.link: pwm.kl $(wildcard ../../machine_rp/*.kl)
	@case "$(KLIN)" in \
	  *.dart) dart run "$(KLIN)" --emit-c pwm.kl ;; \
	  *) "$(KLIN)" --emit-c pwm.kl ;; \
	esac

pwm.elf: linker.ld out/pwm.c out/pwm.link $(LINK_OBJS)
	$(CC) $(CFLAGS) -I. $(LDFLAGS) $$(cat out/pwm.link) out/pwm.c -o $@

clean:
	rm -rf out pwm.elf
