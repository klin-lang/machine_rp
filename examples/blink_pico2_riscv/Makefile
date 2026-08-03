CC := riscv64-unknown-elf-gcc
CFLAGS := -march=rv32imac -mabi=ilp32 -ffreestanding -Os -ffunction-sections -fdata-sections
LDFLAGS := -T linker.ld -Wl,--gc-sections -nostdlib -march=rv32imac -mabi=ilp32

KLIN ?= klin

.PHONY: all clean

all: blink.elf

LINK_OBJS := $(shell test -f out/blink.link && cat out/blink.link)

out/blink.c out/blink.link: blink.kl $(wildcard ../../machine_rp/*.kl)
	@case "$(KLIN)" in \
	  *.dart) dart run "$(KLIN)" --emit-c blink.kl ;; \
	  *) "$(KLIN)" --emit-c blink.kl ;; \
	esac

blink.elf: linker.ld out/blink.c out/blink.link $(LINK_OBJS)
	$(CC) $(CFLAGS) -I. $(LDFLAGS) $$(cat out/blink.link) out/blink.c -o $@

clean:
	rm -rf out blink.elf
