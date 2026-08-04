CC := arm-none-eabi-gcc
CFLAGS := -mcpu=cortex-m0plus -mthumb -ffreestanding -Os -ffunction-sections -fdata-sections
LDFLAGS := -T linker.ld -Wl,--gc-sections -nostdlib
KLIN ?= klin
.PHONY: all clean
all: spi.elf
LINK_OBJS := $(shell test -f out/spi.link && cat out/spi.link)
out/spi.c out/spi.link: spi.kl $(wildcard ../../machine_rp/*.kl)
	@case "$(KLIN)" in \
	  *.dart) dart run "$(KLIN)" --emit-c spi.kl ;; \
	  *) "$(KLIN)" --emit-c spi.kl ;; \
	esac
spi.elf: linker.ld out/spi.c out/spi.link $(LINK_OBJS)
	$(CC) $(CFLAGS) -I. $(LDFLAGS) $$(cat out/spi.link) out/spi.c -lgcc -o $@
clean:
	rm -rf out spi.elf
