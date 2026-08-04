CC := arm-none-eabi-gcc
CFLAGS := -mcpu=cortex-m0plus -mthumb -ffreestanding -Os -ffunction-sections -fdata-sections
LDFLAGS := -T linker.ld -Wl,--gc-sections -nostdlib
KLIN ?= klin
.PHONY: all clean
all: uart.elf
LINK_OBJS := $(shell test -f out/uart.link && cat out/uart.link)
out/uart.c out/uart.link: uart.kl $(wildcard ../../machine_rp/*.kl)
	@case "$(KLIN)" in \
	  *.dart) dart run "$(KLIN)" --emit-c uart.kl ;; \
	  *) "$(KLIN)" --emit-c uart.kl ;; \
	esac
uart.elf: linker.ld out/uart.c out/uart.link $(LINK_OBJS)
	$(CC) $(CFLAGS) -I. $(LDFLAGS) $$(cat out/uart.link) out/uart.c -lgcc -o $@
clean:
	rm -rf out uart.elf
